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
    // select default coin parameters if desired
    NetworkSync(const CoinQ::CoinParams& coin_params = CoinQ::getBitcoinParams());
    ~NetworkSync();

    bool isConnected() const { return isConnected_; }

    void initBlockTree(const std::string& blockTreeFile, bool bCheckProofOfWork = true);
    int getBestHeight();

    void start(const std::string& host, const std::string& port);
    void start(const std::string& host, int port);
    void stop();

    void setBloomFilter(const Coin::BloomFilter& bloomFilter_);

    void resync(const std::vector<bytes_t>& locatorHashes, uint32_t startTime);
    void resync(int resyncHeight);
    void stopResync();

    // MESSAGES TO PEER
    void sendTx(Coin::Transaction& tx);
    void getTx(uchar_vector& hash);

    // SYNC EVENT SUBSCRIPTIONS
    void subscribeStarted(void_slot_t slot) { notifyStarted.connect(slot); }
    void subscribeStopped(void_slot_t slot) { notifyStopped.connect(slot); }
    void subscribeOpen(void_slot_t slot) { notifyOpen.connect(slot); }
    void subscribeTimeout(void_slot_t slot) { notifyTimeout.connect(slot); }
    void subscribeClose(void_slot_t slot) { notifyClose.connect(slot); }

    void subscribeDoneSync(void_slot_t slot) { notifyDoneSync.connect(slot); }
    void subscribeDoneResync(void_slot_t slot) { notifyDoneResync.connect(slot); }

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
    CoinQ::CoinParams coin_params_;

    CoinQ::io_service_t io_service;
    CoinQ::io_service_t::work work;
    boost::thread* io_service_thread;
    CoinQ::Peer peer;

    std::string blockTreeFile;
    CoinQBlockTreeMem blockTree;
    bool blockTreeFlushed;

    CoinQBestChainBlockFilter blockFilter;

    boost::mutex mutex;
    bool resynching;

    bool isConnected_;

    Coin::BloomFilter bloomFilter;

    void initBlockFilter();

    // Sync signals
    CoinQSignal<void> notifyStarted;
    CoinQSignal<void> notifyStopped;
    CoinQSignal<void> notifyOpen;
    CoinQSignal<void> notifyTimeout;
    CoinQSignal<void> notifyClose;

    CoinQSignal<void> notifyDoneSync;
    CoinQSignal<void> notifyDoneResync;

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
