///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_peermanager.h
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#ifndef COINVAULT_PEERMANAGER_H
#define COINVAULT_PEERMANAGER_H

/*
#ifdef __WIN32__
#include <winsock2.h>
#endif
*/

#include "CoinQ_peer_io.h"

#include <map>

#include <boost/thread/mutex.hpp>

namespace CoinQ {

class PeerManager
{
public:
    PeerManager() : work_(io_service_), running_(false) { }
    ~PeerManager() { stop(); }

    void subscribeMessage(peer_message_slot_t slot) { notifyMessage.connect(slot); }
    void subscribeHeaders(peer_headers_slot_t slot) { notifyHeaders.connect(slot); }
    void subscribeBlock(peer_block_slot_t slot) { notifyBlock.connect(slot); }
    void subscribeMerkleBlock(peer_merkle_block_slot_t slot) { notifyMerkleBlock.connect(slot); }
    void subscribeTx(peer_tx_slot_t slot) { notifyTx.connect(slot); }
    void subscribeAddr(peer_addr_slot_t slot) { notifyAddr.connect(slot); }
    void subscribeInv(peer_inv_slot_t slot) { notifyInv.connect(slot); }

    void subscribeStart(peer_slot_t slot) { notifyStart.connect(slot); }
    void subscribeStop(peer_slot_t slot) { notifyStop.connect(slot); }
    void subscribeOpen(peer_slot_t slot) { notifyOpen.connect(slot); }
    void subscribeTimeout(peer_slot_t slot) { notifyTimeout.connect(slot); }
    void subscribeClose(peer_closed_slot_t slot) { notifyClose.connect(slot); }

    void createPeer(
        const std::string& host,
        const std::string& port,
        uint32_t magic_bytes,
        uint32_t protocol_version,
        const std::string& user_agent = std::string(),
        uint32_t start_height = 0,
        bool relay = true
    );

    bool deletePeer(const std::string& peername);

    bool hasPeer(const std::string& peername) const;

    std::size_t peerCount() const;

    void start();
    void stop();
    bool isRunning() const { return running_; }

private:
    io_service_t io_service_;
    io_service_t::work work_;
    bool running_;
    mutable boost::mutex running_mutex_;

    typedef std::vector<std::shared_ptr<boost::thread>> threads_t;
    threads_t threads_;
    mutable boost::mutex threads_mutex_;

    typedef std::map<std::string, std::shared_ptr<Peer>> peermap_t;
    peermap_t peermap_;
    mutable boost::mutex peermap_mutex_;

    CoinQSignal<Peer&, const Coin::CoinNodeMessage&>    notifyMessage;
    CoinQSignal<Peer&, const Coin::HeadersMessage&>     notifyHeaders;
    CoinQSignal<Peer&, const Coin::CoinBlock&>          notifyBlock;
    CoinQSignal<Peer&, const Coin::MerkleBlock&>        notifyMerkleBlock;
    CoinQSignal<Peer&, const Coin::Transaction&>        notifyTx;
    CoinQSignal<Peer&, const Coin::AddrMessage&>        notifyAddr;
    CoinQSignal<Peer&, const Coin::Inventory&>          notifyInv;

    CoinQSignal<Peer&>                                  notifyStart;
    CoinQSignal<Peer&>                                  notifyStop;
    CoinQSignal<Peer&>                                  notifyOpen;
    CoinQSignal<Peer&>                                  notifyTimeout;
    CoinQSignal<Peer&, int, const std::string&>         notifyClose;
};

}

#endif // COINVAULT_PEERMANAGER_H
