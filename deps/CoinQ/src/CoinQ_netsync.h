///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_netsync.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef COINQ_NETSYNC_H
#define COINQ_NETSYNC_H

#ifdef __WIN32__
#include <winsock2.h>
#endif

#include "CoinQ_peer_io.h"
#include "CoinQ_blocks.h"
#include "CoinQ_filter.h"

#include "CoinQ_signals.h"
#include "CoinQ_slots.h"

#include <boost/thread.hpp>

#include "CoinQ_typedefs.h"
#include "CoinQ_coinparams.h"

#include <CoinCore/BloomFilter.h>

typedef Coin::Transaction coin_tx_t;
typedef ChainHeader chain_header_t;
typedef ChainBlock chain_block_t;
typedef ChainMerkleBlock chain_merkle_block_t;

namespace CoinQ {
    namespace Network {

class NetworkSync
{
public:
    NetworkSync(const CoinQ::CoinParams& coinParams = CoinQ::getBitcoinParams());
    ~NetworkSync();

    void loadHeaders(const std::string& blockTreeFile, bool bCheckProofOfWork = true);
    bool headersSynched() const { return m_bHeadersSynched; }
    int getBestHeight();

/*
    void start();
    void stop();
    bool isStarted() const { return m_bStarted; }
*/
    void start(const std::string& host, const std::string& port = "");
    void start(const std::string& host, int port);
    void stop();
    bool connected() const { return m_bConnected; }

    void setBloomFilter(const Coin::BloomFilter& bloomFilter);

    void syncBlocks(const std::vector<bytes_t>& locatorHashes, uint32_t startTime);
    //void syncBlocks(int resyncHeight);
    void stopSyncBlocks();
    bool blocksSynched() const { return m_bBlocksSynched; }

    // MESSAGES TO PEER
    void sendTx(Coin::Transaction& tx);
    void getTx(uchar_vector& hash);
    void getMempool();

    // SYNC EVENT SUBSCRIPTIONS
    void subscribeStarted(void_slot_t slot) { notifyStarted.connect(slot); }
    void subscribeStopped(void_slot_t slot) { notifyStopped.connect(slot); }
    void subscribeOpen(void_slot_t slot) { notifyOpen.connect(slot); }
    void subscribeTimeout(void_slot_t slot) { notifyTimeout.connect(slot); }
    void subscribeClose(void_slot_t slot) { notifyClose.connect(slot); }

    void subscribeFetchingHeaders(void_slot_t slot) { notifyFetchingHeaders.connect(slot); }
    void subscribeHeadersSynched(void_slot_t slot) { notifyHeadersSynched.connect(slot); }

    void subscribeFetchingBlocks(void_slot_t slot) { notifyFetchingBlocks.connect(slot); }
    void subscribeBlocksSynched(void_slot_t slot) { notifyBlocksSynched.connect(slot); }

    void subscribeStatus(string_slot_t slot) { notifyStatus.connect(slot); }
    void subscribeError(string_slot_t slot) { notifyError.connect(slot); }

    // PEER EVENT SUBSCRIPTIONS
    void subscribeTx(tx_slot_t slot) { notifyTx.connect(slot); }
    void subscribeBlock(chain_block_slot_t slot) { notifyBlock.connect(slot); }
    void subscribeMerkleBlock(chain_merkle_block_slot_t slot) { notifyMerkleBlock.connect(slot); }
    void subscribeAddBestChain(chain_header_slot_t slot) { notifyAddBestChain.connect(slot); }
    void subscribeRemoveBestChain(chain_header_slot_t slot) { notifyRemoveBestChain.connect(slot); }
    void subscribeBlockTreeChanged(void_slot_t slot) { notifyBlockTreeChanged.connect(slot); }

private:
    CoinQ::CoinParams m_coinParams;

    mutable boost::mutex m_startMutex;
    bool m_bStarted;
    CoinQ::io_service_t m_ioService;
    CoinQ::io_service_t::work m_work;
    boost::thread m_ioServiceThread;
    CoinQ::Peer m_peer;

    mutable boost::mutex m_connectionMutex;
    bool m_bConnected;

    mutable boost::mutex m_syncMutex;
    std::string m_blockTreeFile;
    CoinQBlockTreeMem m_blockTree;
    bool m_blockTreeLoaded;
    bool m_bFetchingHeaders;
    bool m_bHeadersSynched;

    bool m_bFetchingBlocks;
    bool m_bBlocksSynched;

    uint32_t m_lastRequestedBlockHeight;

    Coin::BloomFilter m_bloomFilter;

    void initBlockFilter();

    // Sync signals
    CoinQSignal<void> notifyStarted;
    CoinQSignal<void> notifyStopped;
    CoinQSignal<void> notifyOpen;
    CoinQSignal<void> notifyTimeout;
    CoinQSignal<void> notifyClose;

    CoinQSignal<void> notifyFetchingHeaders;
    CoinQSignal<void> notifyHeadersSynched;

    CoinQSignal<void> notifyFetchingBlocks;
    CoinQSignal<void> notifyBlocksSynched;

    CoinQSignal<const std::string&> notifyStatus;
    CoinQSignal<const std::string&> notifyError;

    // Peer signals
    CoinQSignal<const Coin::Transaction&> notifyTx;
    CoinQSignal<const ChainBlock&> notifyBlock;
    CoinQSignal<const ChainMerkleBlock&> notifyMerkleBlock;
    CoinQSignal<const ChainHeader&> notifyAddBestChain;
    CoinQSignal<const ChainHeader&> notifyRemoveBestChain;
    CoinQSignal<void> notifyBlockTreeChanged;
};

}
}

#endif // COINVAULT_NETWORKSYNC_H
