///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_peerasync.h 
//
// Copyright (c) 2012-2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef _COINQ_PEERASYNC_H_
#define _COINQ_PEERASYNC_H_

#include "CoinQ_signals.h"
#include "CoinQ_slots.h"

#include <numericdata.h>

#include <iostream>

#include <queue>

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>


inline uint64_t getRandomNonce64()
{
    // TODO: Use better RNG
    srand(time(NULL));
    uint64_t nonce = 0;
    for (uint i = 0; i < 4; i++) {
        nonce <<= 8;
        nonce |= rand() % 0xff;
    }
    return nonce;
}

namespace CoinQ {

typedef boost::asio::io_service io_service_t;
typedef boost::asio::ip::tcp::endpoint endpoint_t;
typedef boost::asio::ip::tcp tcp;

class Peer;

typedef std::function<void(Peer&)>                                  peer_slot_t;
typedef std::function<void(Peer&, int, const std::string&)>         peer_closed_slot_t;

typedef std::function<void(Peer&, const Coin::CoinNodeMessage&)>    peer_message_slot_t;
typedef std::function<void(Peer&, const Coin::HeadersMessage&)>     peer_headers_slot_t;
typedef std::function<void(Peer&, const Coin::CoinBlock&)>          peer_block_slot_t;
typedef std::function<void(Peer&, const Coin::MerkleBlock&)>        peer_merkle_block_slot_t;
typedef std::function<void(Peer&, const Coin::Transaction&)>        peer_tx_slot_t;
typedef std::function<void(Peer&, const Coin::AddrMessage&)>        peer_addr_slot_t;
typedef std::function<void(Peer&, const Coin::Inventory&)>          peer_inv_slot_t; 


class Peer
{
private:
    io_service_t& io_service;
    io_service_t::strand strand;
    tcp::socket socket;
    tcp::endpoint endpoint;
    boost::asio::deadline_timer timer; // for handshakes
 
    uint32_t magic_bytes;
    uchar_vector magicBytes;
    uint32_t protocol_version;

    std::string hostname;
    uint16_t port;

    boost::shared_mutex mutex;
    bool bRunning;

    bool bWriteReady;

    boost::mutex handshakeMutex;
    boost::condition_variable handshakeCond;
    bool bHandshakeComplete;

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
    CoinQSignal<Peer&, int, const std::string&>         notifyClose;

    CoinQSignal<Peer&>                                  notifyTimeout;

    static const unsigned int READ_BUFFER_SIZE = 1024;
    unsigned char read_buffer[READ_BUFFER_SIZE];

    uchar_vector read_message;
    uchar_vector write_message;
    std::queue<boost::shared_ptr<uchar_vector>> sendQueue;
    boost::mutex sendMutex;

    void do_connect(tcp::resolver::iterator iter);
    void do_read();
    void do_write(boost::shared_ptr<uchar_vector> data);
    void do_send(const Coin::CoinNodeMessage& message); // calls do_write from the strand thread 
    void do_handshake();

public:
    Peer(io_service_t& _io_service, const endpoint_t& _endpoint, uint32_t _magic_bytes, uint32_t _protocol_version)
        : io_service(_io_service), strand(io_service), socket(_io_service), endpoint(_endpoint), timer(_io_service), magic_bytes(_magic_bytes), protocol_version(_protocol_version), bRunning(false), bHandshakeComplete(false)
    {
        magicBytes = uint_to_vch(magic_bytes, _BIG_ENDIAN);
    }

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

    static const unsigned char DEFAULT_Ipv6[];

    void start();
    void stop();
    bool send(const Coin::CoinNodeMessage& message);

    bool isRunning() const { return bRunning; }

    uint32_t getMagic() const { return magic_bytes; }
    const endpoint_t& getEndpoint() const { return endpoint; }
    std::string getName() const { std::stringstream ss; ss << endpoint.address().to_string() << ":" << endpoint.port(); return ss.str(); }
    
    void getBlock(const uchar_vector& hash)
    {
        Coin::InventoryItem block(MSG_BLOCK, hash);
        Coin::Inventory inv;
        inv.addItem(block);
        Coin::GetDataMessage getData(inv);
        Coin::CoinNodeMessage msg(magic_bytes, &getData);
        send(msg); 
    }

    void getFilteredBlock(const uchar_vector& hash)
    {
        Coin::InventoryItem block(MSG_FILTERED_BLOCK, hash);
        Coin::Inventory inv;
        inv.addItem(block);
        Coin::GetDataMessage getData(inv);
        Coin::CoinNodeMessage msg(magic_bytes, &getData);
        send(msg);
    }

    void getHeaders(const std::vector<uchar_vector>& locatorHashes, const uchar_vector& hashStop = g_zero32bytes)
    {
        Coin::GetHeadersMessage getHeaders(protocol_version, locatorHashes, hashStop);
        Coin::CoinNodeMessage msg(magic_bytes, &getHeaders);
        send(msg);
    }

    void getBlocks(const std::vector<uchar_vector>& locatorHashes, const uchar_vector& hashStop = g_zero32bytes)
    {
        Coin::GetBlocksMessage getBlocks(protocol_version, locatorHashes, hashStop);
        Coin::CoinNodeMessage msg(magic_bytes, &getBlocks);
        send(msg);
    }

    void getMempool()
    {
        Coin::BlankMessage mempool("mempool");
        Coin::CoinNodeMessage msg(magic_bytes, &mempool);
        send(msg);
    }

    void getAddr()
    {
        Coin::BlankMessage getaddr("getaddr");
        Coin::CoinNodeMessage msg(magic_bytes, &getaddr);
        send(msg);
    }
};

// TODO: use a thread group and run one thread per CPU
class PeerManager
{
private:
    io_service_t io_service;
    io_service_t::work work;
    boost::thread io_service_thread;
    boost::mutex startMutex;
    bool bStarted;

    uint32_t magic;
    uint32_t protocol_version;

    typedef std::map<endpoint_t, boost::shared_ptr<Peer>> peer_map_t;
    peer_map_t peers;

public:
    PeerManager(uint32_t _magic, uint32_t _protocol_version) : work(io_service), bStarted(false), magic(_magic), protocol_version(_protocol_version) { }
    ~PeerManager() { stop(); }

    void start()
    {
        if (bStarted) throw std::runtime_error("Already started.");
        boost::unique_lock<boost::mutex> lock(startMutex);
        if (bStarted) throw std::runtime_error("Already started.");

        io_service_thread = boost::thread([this]() { io_service.run(); });
        for (auto& peer: peers) {
            peer.second->start();
        }
        bStarted = true;
    }

    void stop()
    {
        if (!bStarted) return;
        boost::unique_lock<boost::mutex> lock(startMutex);
        if (!bStarted) return;

        for (auto& peer: peers) {
            peer.second->stop();
        }
        io_service.stop();
        io_service_thread.join();
    }

    void addPeer(const std::string& hostname, const std::string& port)
    {
        tcp::resolver resolver(io_service);
        tcp::resolver::query query(hostname, port);
        auto iter = resolver.resolve(query);
        endpoint_t ep = *iter;
        boost::shared_ptr<Peer> pPeer(new Peer(io_service, ep, magic, protocol_version));
        peers[ep] = pPeer;
        pPeer->start();
    }
};

}

#endif // _COINQ_PEERASYNC_H_
