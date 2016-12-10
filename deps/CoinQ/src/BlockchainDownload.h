///////////////////////////////////////////////////////////////////////////////
//
// BlockchainDownload.h
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#ifdef __WIN32__
#include <winsock2.h>
#endif

#include "CoinQ_peer_io.h"

#include <boost/thread.hpp>

#include "CoinQ_coinparams.h"
#include "CoinQ_blocks.h"

#include <CoinCore/typedefs.h>
#include <CoinCore/CoinNodeData.h>

#include <Signals/Signals.h>

#include <queue>


namespace CoinQ
{
    namespace Network
    {

class BlockchainDownload
{
public:
    BlockchainDownload(const CoinQ::CoinParams& coinParams = CoinQ::getBitcoinParams(), bool bCheckProofOfWork = false);
    ~BlockchainDownload();

    void setCoinParams(const CoinQ::CoinParams& coinParams);
    const CoinQ::CoinParams& getCoinParams() const { return m_coinParams; }

    void enableCheckProofOfWork(bool bCheckProofOfWork = true) { m_bCheckProofOfWork = bCheckProofOfWork; }

    int getBestHeight() const { return m_blockTree.getBestHeight(); }
    const bytes_t& getBestHash() const { return m_blockTree.getBestHash(); }

    void start(const std::string& host, const std::string& port = std::string(), const std::vector<uchar_vector>& locatorHashes = std::vector<uchar_vector>(), const uchar_vector& hashStop = uchar_vector(32, 0));
    void start(const std::string& host, int port, const std::vector<uchar_vector>& locatorHashes = std::vector<uchar_vector>(), const uchar_vector& hashStop = uchar_vector(32, 0));
    void stop();
    bool connected() const { return m_bConnected; }

    typedef Signals::Signal<>                           VoidSignal;
    typedef Signals::Signal<const Coin::CoinBlock&>     BlockSignal;
    typedef Signals::Signal<const std::string&, int>    ErrorSignal;

    // SYNC EVENT SUBSCRIPTIONS
    Signals::Connection subscribeStarted(VoidSignal::Slot slot)             { return notifyStarted.connect(slot); }
    Signals::Connection subscribeStopped(VoidSignal::Slot slot)             { return notifyStopped.connect(slot); }
    Signals::Connection subscribeOpen(VoidSignal::Slot slot)                { return notifyOpen.connect(slot); }
    Signals::Connection subscribeClose(VoidSignal::Slot slot)               { return notifyClose.connect(slot); }
    Signals::Connection subscribeTimeout(VoidSignal::Slot slot)             { return notifyTimeout.connect(slot); }

    Signals::Connection subscribeConnectionError(ErrorSignal::Slot slot)    { return notifyConnectionError.connect(slot); }
    Signals::Connection subscribeProtocolError(ErrorSignal::Slot slot)      { return notifyProtocolError.connect(slot); }
    Signals::Connection subscribeBlockTreeError(ErrorSignal::Slot slot)     { return notifyBlockTreeError.connect(slot); }

    Signals::Connection subscribeBlocksSynched(VoidSignal::Slot slot)       { return notifyBlocksSynched.connect(slot); }

    // PEER EVENT SUBSCRIPTIONS
    Signals::Connection subscribeBlock(BlockSignal::Slot slot)              { return notifyBlock.connect(slot); }

private:
    CoinQ::CoinParams m_coinParams;
    bool m_bCheckProofOfWork;

    std::vector<uchar_vector> m_locatorHashes;
    uchar_vector m_hashStop;

    uchar_vector m_lastRequestedBlockHash;
    uchar_vector m_lastReceivedBlockHash;
    CoinQBlockTreeMem m_blockTree;
 
    bool m_bStarted;
    boost::mutex m_startMutex;

    bool m_bIOServiceStarted;
    boost::mutex m_ioServiceMutex;
    void startIOServiceThread();
    void stopIOServiceThread();
    CoinQ::io_service_t m_ioService;
    boost::thread m_ioServiceThread;
    CoinQ::io_service_t::work m_work;

    bool m_bConnected;
    CoinQ::Peer m_peer;

    // Sync signals
    VoidSignal          notifyStarted;
    VoidSignal          notifyStopped;
    VoidSignal          notifyOpen;
    VoidSignal          notifyClose;
    VoidSignal          notifyTimeout;

    ErrorSignal         notifyConnectionError;
    ErrorSignal         notifyProtocolError;
    ErrorSignal         notifyBlockTreeError;

    VoidSignal          notifyBlocksSynched;

    // Peer signals
    BlockSignal         notifyBlock;
};

    }
}

