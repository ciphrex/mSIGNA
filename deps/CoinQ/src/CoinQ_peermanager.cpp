///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_peermanager.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "CoinQ_peermanager.h"

using namespace CoinQ;

void PeerManager::createPeer(
    const std::string& host,
    const std::string& port,
    uint32_t magic_bytes,
    uint32_t protocol_version,
    const std::string& user_agent,
    uint32_t start_height,
    bool relay
)
{
    std::shared_ptr<Peer> peer(new Peer(io_service_,
        host,
        port,
        magic_bytes,
        protocol_version,
        user_agent,
        start_height,
        relay));

    // TODO: use a separate thread with an event queue
    peer->subscribeMessage([&](Peer& peer, const Coin::CoinNodeMessage& message) { notifyMessage(peer, message); });
    peer->subscribeHeaders([&](Peer& peer, const Coin::HeadersMessage& headers) { notifyHeaders(peer, headers); });
    peer->subscribeBlock([&](Peer& peer, const Coin::CoinBlock& block) { notifyBlock(peer, block); });
    peer->subscribeMerkleBlock([&](Peer& peer, const Coin::MerkleBlock& merkleblock) { notifyMerkleBlock(peer, merkleblock); });
    peer->subscribeTx([&](Peer& peer, const Coin::Transaction& tx) { notifyTx(peer, tx); });
    peer->subscribeAddr([&](Peer& peer, const Coin::AddrMessage& addr) { notifyAddr(peer, addr); });
    peer->subscribeInv([&](Peer& peer, const Coin::Inventory& inv) { notifyInv(peer, inv); });

    peer->subscribeStart([&](Peer& peer) { notifyStart(peer); });
    peer->subscribeStop([&](Peer& peer) { notifyStop(peer); });
    peer->subscribeOpen([&](Peer& peer) { notifyOpen(peer); });
    peer->subscribeTimeout([&](Peer& peer) { notifyTimeout(peer); deletePeer(peer.name()); });
    peer->subscribeClose([&](Peer& peer, int code, const std::string& message) { notifyClose(peer, code, message); deletePeer(peer.name()); });

    {
        boost::lock_guard<boost::mutex> peermap_lock(peermap_mutex_);
        peermap_[host + ":" + port] = peer; // TODO: Resolve the endpoint before adding to peermap (perhaps on notifyOpen).
    }

    peer->start();
}

bool PeerManager::deletePeer(const std::string& peername)
{
    boost::lock_guard<boost::mutex> peermap_lock(peermap_mutex_);
    return (peermap_.erase(peername) == 1);
}

bool PeerManager::hasPeer(const std::string& peername) const
{
    boost::lock_guard<boost::mutex> peermap_lock(peermap_mutex_);
    return (peermap_.count(peername) != 0);
}

size_t PeerManager::peerCount() const
{
    boost::lock_guard<boost::mutex> peermap_lock(peermap_mutex_);
    return peermap_.size();
}

void PeerManager::start()
{
    if (running_) throw std::runtime_error("PeerManager is already started.");
    boost::lock_guard<boost::mutex> running_lock(running_mutex_);
    if (running_) throw std::runtime_error("PeerManager is already started.");

    running_ = true;

    std::shared_ptr<boost::thread> thread(new boost::thread(boost::bind(&io_service_t::run, &io_service_)));

    boost::lock_guard<boost::mutex> threads_lock(threads_mutex_);
    threads_.push_back(thread);
}

void PeerManager::stop()
{
    if (!running_) return;
    boost::lock_guard<boost::mutex> running_lock(running_mutex_);
    if (!running_) return;

    running_ = false;

    io_service_.stop();
    for (auto& thread: threads_) { thread->join(); }

    boost::lock_guard<boost::mutex> threads_lock(threads_mutex_);
    threads_.clear();

    boost::lock_guard<boost::mutex> peermap_lock(peermap_mutex_);
    peermap_.clear();
}
