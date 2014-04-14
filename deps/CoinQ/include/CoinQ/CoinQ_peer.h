///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_peer.h 
//
// Copyright (c) 2012-2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef _COINQ_PEER_H_
#define _COINQ_PEER_H_

#include "CoinQ_signals.h"
#include "CoinQ_slots.h"

#include <CoinNodeAbstractListener.h>

#include <iostream>

#include <boost/thread.hpp>



namespace CoinQ {

typedef boost::asio::ip::tcp::endpoint endpoint_t;

class Peer;

typedef std::function<void(Peer&)>                                 peer_slot_t;
typedef std::function<void(Peer&, int)>                            peer_closed_t;

typedef std::function<void(Peer&, const Coin::HeadersMessage&)>    peer_headers_slot_t;
typedef std::function<void(Peer&, const Coin::CoinBlock&)>         peer_block_slot_t;
typedef std::function<void(Peer&, const Coin::Transaction&)>       peer_tx_slot_t;
typedef std::function<void(Peer&, const Coin::AddrMessage&)>       peer_addr_slot_t;


class Peer : public Coin::CoinNodeAbstractListener
{
private:
    bool bRunning;
    boost::shared_mutex mutex;

    CoinQSignal<Peer&, const Coin::HeadersMessage&>    notifyHeaders;
    CoinQSignal<Peer&, const Coin::CoinBlock&>         notifyBlock;
    CoinQSignal<Peer&, const Coin::Transaction&>       notifyTx;
    CoinQSignal<Peer&, const Coin::AddrMessage&>       notifyAddr;

    CoinQSignal<Peer&>                                 notifyStart;
    CoinQSignal<Peer&>                                 notifyStop;
    CoinQSignal<Peer&, int>                            notifyClosed;

public:
    Peer(uint32_t magic_bytes, uint32_t protocol_version, const std::string& hostname, uint16_t port)
        : CoinNodeAbstractListener(magic_bytes, protocol_version, hostname, port), bRunning(false) { }

    void subscribeHeaders(peer_headers_slot_t slot) { notifyHeaders.connect(slot); }
    void subscribeBlock(peer_block_slot_t slot) { notifyBlock.connect(slot); }
    void subscribeTx(peer_tx_slot_t slot) { notifyTx.connect(slot); }
    void subscribeAddr(peer_addr_slot_t slot) { notifyAddr.connect(slot); }

    void subscribeStart(std::function<void(Peer&)> slot) { notifyStart.connect(slot); }
    void subscribeStop(std::function<void(Peer&)> slot) { notifyStop.connect(slot); }
    void subscribeClosed(std::function<void(Peer&, int)> slot) { notifyClosed.connect(slot); }

    virtual void start()
    {
        if (bRunning) {
            throw std::runtime_error("Peer already started.");
        }

        boost::unique_lock<boost::shared_mutex> lock(mutex);
        try {
            CoinNodeAbstractListener::start();
            bRunning = true;
            notifyStart(*this);
        }
        catch (const std::exception& e) {
            throw std::runtime_error(e.what());
        }
    }

    virtual void stop()
    {
        if (!bRunning) return;
        boost::unique_lock<boost::shared_mutex> lock(mutex);
        CoinNodeAbstractListener::stop();
        bRunning = false;
        notifyStop(*this);
    }

    bool isRunning() const { return bRunning; }
    
    virtual void onBlock(Coin::CoinBlock& block);
    virtual void onTx(Coin::Transaction& tx);
    virtual void onAddr(Coin::AddrMessage& addr);
    virtual void onHeaders(Coin::HeadersMessage& headers);
    
    virtual void onSocketClosed(int code);
};

inline void Peer::onBlock(Coin::CoinBlock& block)
{
    boost::shared_lock<boost::shared_mutex> lock(mutex);
    notifyBlock(*this, block);
}

inline void Peer::onTx(Coin::Transaction& tx)
{
    boost::shared_lock<boost::shared_mutex> lock(mutex);
    notifyTx(*this, tx);
}

inline void Peer::onAddr(Coin::AddrMessage& addr)
{
    boost::shared_lock<boost::shared_mutex> lock(mutex);
    notifyAddr(*this, addr);
}

inline void Peer::onHeaders(Coin::HeadersMessage& headers)
{
    boost::shared_lock<boost::shared_mutex> lock(mutex);
    notifyHeaders(*this, headers);
}

inline void Peer::onSocketClosed(int code)
{
    std::cout << "-------------------------------" << std::endl
              << "--Socket closed with code: " << code << std::endl
              << "-------------------------------" << std::endl
              << std::endl;
    bRunning = false;
    notifyClosed(*this, code);
}

}

#endif // _COINQ_PEER_H_
