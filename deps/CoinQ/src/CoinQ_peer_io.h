///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_peer_io.h 
//
// Copyright (c) 2012-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include "CoinQ_signals.h"
#include "CoinQ_slots.h"

#include <CoinCore/typedefs.h>
#include <CoinCore/numericdata.h>

#include <logger/logger.h>

#include <queue>

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>

#include <CoinCore/CoinNodeData.h> // TODO: Move flags into separate include file

// TODO: Support flags for segregated witness
const uint32_t DEFAULT_INV_FLAGS = 0; // MSG_WITNESS_FLAG;

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
typedef std::function<void(Peer&, const std::string&, int)>         peer_error_slot_t;

typedef std::function<void(Peer&, const Coin::CoinNodeMessage&)>    peer_message_slot_t;
typedef std::function<void(Peer&, const Coin::HeadersMessage&)>     peer_headers_slot_t;
typedef std::function<void(Peer&, const Coin::CoinBlock&)>          peer_block_slot_t;
typedef std::function<void(Peer&, const Coin::MerkleBlock&)>        peer_merkle_block_slot_t;
typedef std::function<void(Peer&, const Coin::Transaction&)>        peer_tx_slot_t;
typedef std::function<void(Peer&, const Coin::AddrMessage&)>        peer_addr_slot_t;
typedef std::function<void(Peer&, const Coin::Inventory&)>          peer_inv_slot_t; 


class Peer
{
public:
    Peer(io_service_t& io_service, const std::string& host = "", const std::string& port = "", uint32_t magic_bytes = 0, uint32_t protocol_version = 0, const std::string& user_agent = std::string(), uint32_t start_height = 0, bool relay = true, uint32_t invFlags = DEFAULT_INV_FLAGS) :
        //io_service_(io_service),
        strand_(io_service),
        resolver_(io_service),
        socket_(io_service),
        timer_(io_service),
        host_(host),
        port_(port),
        magic_bytes_(magic_bytes),
        protocol_version_(protocol_version),
        user_agent_(user_agent),
        start_height_(start_height),
        relay_(relay),
        invFlags_(invFlags),
        bRunning(false)
    {
        magic_bytes_vector_ = uint_to_vch(magic_bytes_, LITTLE_ENDIAN_);
    }

    ~Peer() { stop(); }

    void set(const std::string& host, const std::string& port, uint32_t magic_bytes, uint32_t protocol_version, const std::string& user_agent = std::string(), uint32_t start_height = 0, bool relay = true)
    {
        stop();
        host_ = host;
        port_ = port;
        magic_bytes_ = magic_bytes;
        protocol_version_ = protocol_version;
        user_agent_ = user_agent;
        start_height_ = start_height;
        relay_ = relay;

        magic_bytes_vector_ = uint_to_vch(magic_bytes_, LITTLE_ENDIAN_);
    }

    void setInvFlags(uint32_t invFlags) { invFlags_ = invFlags; }

    void subscribeMessage(peer_message_slot_t slot) { notifyMessage.connect(slot); }
    void subscribeHeaders(peer_headers_slot_t slot) { notifyHeaders.connect(slot); }
    void subscribeBlock(peer_block_slot_t slot) { notifyBlock.connect(slot); }
    void subscribeMerkleBlock(peer_merkle_block_slot_t slot) { notifyMerkleBlock.connect(slot); }
    void subscribeTx(peer_tx_slot_t slot) { notifyTx.connect(slot); }
    void subscribeAddr(peer_addr_slot_t slot) { notifyAddr.connect(slot); }
    void subscribeInv(peer_inv_slot_t slot) { notifyInv.connect(slot); }
    void subscribeProtocolError(peer_error_slot_t slot) { notifyProtocolError.connect(slot); }

    void subscribeStart(peer_slot_t slot) { notifyStart.connect(slot); }
    void subscribeStop(peer_slot_t slot) { notifyStop.connect(slot); }
    void subscribeOpen(peer_slot_t slot) { notifyOpen.connect(slot); }
    void subscribeTimeout(peer_slot_t slot) { notifyTimeout.connect(slot); }
    void subscribeClose(peer_slot_t slot) { notifyClose.connect(slot); }
    void subscribeConnectionError(peer_error_slot_t slot) { notifyConnectionError.connect(slot); }

    static const unsigned char DEFAULT_Ipv6[];

    void start();
    void stop();
    bool send(Coin::CoinNodeStructure& message);

    bool isRunning() const { return bRunning; }

    uint32_t magic_bytes() const { return magic_bytes_; }
    const endpoint_t& endpoint() const { return endpoint_; }
    std::string resolved_name() const { std::stringstream ss; ss << endpoint_.address().to_string() << ":" << endpoint_.port(); return ss.str(); }
    std::string name() const { std::stringstream ss; ss << host_ << ":" << port_; return ss.str(); }

    uint32_t inv_flags() const { return invFlags_; }

    void getTx(const bytes_t& hash)
    {
        if (hash.size() != 32)
        {
            std::stringstream err;
            err << "Invalid transaction hash requested: " << uchar_vector(hash).getHex();
            LOGGER(error) << "Peer::getTx() - " << err.str() << std::endl;
            notifyProtocolError(*this, err.str(), -1);
            return;
        }

        Coin::InventoryItem tx(MSG_TX | invFlags_, hash);
        Coin::Inventory inv;
        inv.addItem(tx);
        Coin::GetDataMessage getData(inv);
        send(getData);
    }

    void getTxs(const hashvector_t& txhashes)
    {
        using namespace Coin;

        if (txhashes.empty()) return;
        Inventory inv;
        for (auto& hash: txhashes)
        {
            if (hash.size() != 32)
            {
                std::stringstream err;
                err << "Invalid transaction hash requested: " << uchar_vector(hash).getHex();
                LOGGER(error) << "Peer::getTxs() - " << err.str() << std::endl;
                notifyProtocolError(*this, err.str(), -1);
                return;
            }

            inv.addItem(InventoryItem(MSG_TX | invFlags_, hash));
        }
        GetDataMessage getData(inv);
        send(getData);
    }
 
    void getBlock(const bytes_t& hash)
    {
        if (hash.size() != 32)
        {
            std::stringstream err;
            err << "Invalid block hash requested: " << uchar_vector(hash).getHex();
            LOGGER(error) << "Peer::getBlock() - " << err.str() << std::endl;
            notifyProtocolError(*this, err.str(), -1);
            return;
        }

        Coin::InventoryItem block(MSG_BLOCK | invFlags_, hash);
        Coin::Inventory inv;
        inv.addItem(block);
        Coin::GetDataMessage getData(inv);
        send(getData); 
    }

    void getFilteredBlock(const bytes_t& hash)
    {
        if (hash.size() != 32)
        {
            std::stringstream err;
            err << "Invalid block hash requested: " << uchar_vector(hash).getHex();
            LOGGER(error) << "Peer::getFilteredBlock() - " << err.str() << std::endl;
            notifyProtocolError(*this, err.str(), -1);
            return;
        }

        Coin::InventoryItem block(MSG_FILTERED_BLOCK | invFlags_, hash);
        Coin::Inventory inv;
        inv.addItem(block);
        Coin::GetDataMessage getData(inv);
        send(getData);
    }

    void getHeaders(const std::vector<uchar_vector>& locatorHashes, const uchar_vector& hashStop = g_zero32bytes)
    {
        for (auto& hash: locatorHashes)
        {
            if (hash.size() != 32)
            {
                std::stringstream err;
                err << "Invalid locator hash: " << uchar_vector(hash).getHex();
                LOGGER(error) << "Peer::getHeaders() - " << err.str() << std::endl;
                notifyProtocolError(*this, err.str(), -1);
                return;
            }
        }

        if (hashStop.size() != 32)
        {
            std::stringstream err;
            err << "Invalid hash stop: " << uchar_vector(hashStop).getHex();
            LOGGER(error) << "Peer::getHeaders() - " << err.str() << std::endl;
            notifyProtocolError(*this, err.str(), -1);
            return;
        }

        Coin::GetHeadersMessage getHeaders(protocol_version_, locatorHashes, hashStop);
        send(getHeaders);
    }

    void getHeaders()
    {
        std::vector<uchar_vector> locatorHashes;
        locatorHashes.push_back(g_zero32bytes);
        getHeaders(locatorHashes);
    }

    void getBlocks(const std::vector<uchar_vector>& locatorHashes, const uchar_vector& hashStop = g_zero32bytes)
    {
        for (auto& hash: locatorHashes)
        {
            if (hash.size() != 32)
            {
                std::stringstream err;
                err << "Invalid locator hash: " << uchar_vector(hash).getHex();
                LOGGER(error) << "Peer::getBlocks() - " << err.str() << std::endl;
                notifyProtocolError(*this, err.str(), -1);
                return;
            }
        }

        if (hashStop.size() != 32)
        {
            std::stringstream err;
            err << "Invalid hash stop: " << uchar_vector(hashStop).getHex();
            LOGGER(error) << "Peer::getBlocks() - " << err.str() << std::endl;
            notifyProtocolError(*this, err.str(), -1);
            return;
        }

        Coin::GetBlocksMessage getBlocks(protocol_version_, locatorHashes, hashStop);
        send(getBlocks);
    }

    void getBlocks()
    {
        std::vector<uchar_vector> locatorHashes;
        locatorHashes.push_back(g_zero32bytes);
        getBlocks(locatorHashes);
    }

    void getMempool()
    {
        Coin::BlankMessage mempool("mempool");
        send(mempool);
    }

    void getAddr()
    {
        Coin::BlankMessage getAddr("getaddr");
        send(getAddr);
    }

private:
    // ASIO environment
    //io_service_t& io_service_;
    io_service_t::strand strand_;
    tcp::resolver resolver_;
    tcp::socket socket_;
    tcp::endpoint endpoint_;
    boost::asio::deadline_timer timer_; // for handshakes
 
    // Peer attributes
    std::string host_;
    std::string port_;

    uint32_t magic_bytes_;
    uchar_vector magic_bytes_vector_;
    uint32_t protocol_version_;

    std::string user_agent_;
    uint32_t start_height_;
    bool relay_;

    // Protocol flags
    uint32_t invFlags_;

    // State members
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
    CoinQSignal<Peer&, const std::string&, int>         notifyProtocolError;

    CoinQSignal<Peer&>                                  notifyStart;
    CoinQSignal<Peer&>                                  notifyStop;
    CoinQSignal<Peer&>                                  notifyOpen;
    CoinQSignal<Peer&>                                  notifyClose;
    CoinQSignal<Peer&, const std::string&, int>         notifyConnectionError;

    CoinQSignal<Peer&>                                  notifyTimeout;

    static const unsigned int READ_BUFFER_SIZE = 262144;
    unsigned char read_buffer[READ_BUFFER_SIZE];
    std::size_t min_read_bytes;

    uchar_vector read_message;
    uchar_vector write_message;
    std::queue<boost::shared_ptr<uchar_vector>> sendQueue;
    boost::mutex sendMutex;

    void do_connect(tcp::resolver::iterator iter);
    void do_read();
    void do_write(boost::shared_ptr<uchar_vector> data);
    void do_send(const Coin::CoinNodeMessage& message); // calls do_write from the strand thread 
    void do_handshake();
    void do_stop();
    void do_clearSendQueue();
};

}

