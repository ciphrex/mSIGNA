///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_netsync.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

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

#include <CoinCore/typedefs.h>
#include <CoinCore/BloomFilter.h>

#include <queue>

typedef Coin::Transaction coin_tx_t;
typedef ChainHeader chain_header_t;
typedef ChainBlock chain_block_t;
typedef ChainMerkleBlock chain_merkle_block_t;

namespace CoinQ {
    namespace Network {

typedef std::function<void(const ChainMerkleBlock&, const Coin::Transaction&, unsigned int /*txIndex*/, unsigned int /*txCount*/)> merkle_tx_slot_t;

class NetworkSync
{
public:
    NetworkSync(const CoinQ::CoinParams& coinParams = CoinQ::getBitcoinParams());
    ~NetworkSync();

    void setCoinParams(const CoinQ::CoinParams& coinParams);
    const CoinQ::CoinParams& getCoinParams() const { return m_coinParams; }

    void loadHeaders(const std::string& blockTreeFile, bool bCheckProofOfWork = true, std::function<void(const CoinQBlockTreeMem&)> callback = nullptr);
    bool headersSynched() const { return m_bHeadersSynched; }
    int getBestHeight();
    const ChainHeader& getBestHeader() const { return m_blockTree.getHeader(-1); }
    const ChainHeader& getHeader(const bytes_t& hash) const { return m_blockTree.getHeader(hash); }
    const ChainHeader& getHeader(int height) const { return m_blockTree.getHeader(height); }
    const ChainHeader& getHeaderBefore(uint32_t timestamp) const { return m_blockTree.getHeaderBefore(timestamp); }

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
    void getTx(const bytes_t& hash);
    void getTxs(const hashvector_t& hashes);
    void getMempool();
    void getFilteredBlock(const bytes_t& hash);

    // SYNC EVENT SUBSCRIPTIONS
    void subscribeStarted(void_slot_t slot) { notifyStarted.connect(slot); }
    void subscribeStopped(void_slot_t slot) { notifyStopped.connect(slot); }
    void subscribeOpen(void_slot_t slot) { notifyOpen.connect(slot); }
    void subscribeClose(void_slot_t slot) { notifyClose.connect(slot); }
    void subscribeTimeout(void_slot_t slot) { notifyTimeout.connect(slot); }

    void subscribeConnectionError(string_slot_t slot) { notifyConnectionError.connect(slot); }
    void subscribeProtocolError(string_slot_t slot) { notifyProtocolError.connect(slot); }
    void subscribeBlockTreeError(string_slot_t slot) { notifyBlockTreeError.connect(slot); }

    void subscribeFetchingHeaders(void_slot_t slot) { notifyFetchingHeaders.connect(slot); }
    void subscribeHeadersSynched(void_slot_t slot) { notifyHeadersSynched.connect(slot); }

    void subscribeFetchingBlocks(void_slot_t slot) { notifyFetchingBlocks.connect(slot); }
    void subscribeBlocksSynched(void_slot_t slot) { notifyBlocksSynched.connect(slot); }

    void subscribeStatus(string_slot_t slot) { notifyStatus.connect(slot); }

    // PEER EVENT SUBSCRIPTIONS
    void subscribeNewTx(tx_slot_t slot) { notifyNewTx.connect(slot); }
    void subscribeMerkleTx(merkle_tx_slot_t slot) { notifyMerkleTx.connect(slot); }

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
    boost::thread* m_ioServiceThread;
    CoinQ::io_service_t::work m_work;
    CoinQ::Peer m_peer;

    mutable boost::mutex m_connectionMutex;
    bool m_bConnected;

    mutable boost::mutex m_syncMutex;
    std::string m_blockTreeFile;
    CoinQBlockTreeMem m_blockTree;
    bool m_blockTreeLoaded;
    bool m_bHeadersSynched;

    bool m_bFetchingBlocks;
    bool m_bBlocksSynched;

    uint32_t m_lastRequestedBlockHeight;

    Coin::BloomFilter m_bloomFilter;

    void initBlockFilter();

    // Merkle block state
    ChainMerkleBlock m_currentMerkleBlock;
    std::queue<bytes_t> m_currentMerkleTxHashes;
    unsigned int m_currentMerkleTxIndex;
    unsigned int m_currentMerkleTxCount;

    // Sync signals
    CoinQSignal<void> notifyStarted;
    CoinQSignal<void> notifyStopped;
    CoinQSignal<void> notifyOpen;
    CoinQSignal<void> notifyClose;
    CoinQSignal<void> notifyTimeout;

    CoinQSignal<const std::string&> notifyConnectionError;
    CoinQSignal<const std::string&> notifyProtocolError;
    CoinQSignal<const std::string&> notifyBlockTreeError;

    CoinQSignal<void> notifyFetchingHeaders;
    CoinQSignal<void> notifyHeadersSynched;

    CoinQSignal<void> notifyFetchingBlocks;
    CoinQSignal<void> notifyBlocksSynched;

    CoinQSignal<const std::string&> notifyStatus;

    // Peer signals
    CoinQSignal<const Coin::Transaction&> notifyNewTx;
    CoinQSignal<const ChainMerkleBlock&, const Coin::Transaction&, unsigned int, unsigned int> notifyMerkleTx;

    CoinQSignal<const ChainBlock&> notifyBlock;
    CoinQSignal<const ChainMerkleBlock&> notifyMerkleBlock;
    CoinQSignal<const ChainHeader&> notifyAddBestChain;
    CoinQSignal<const ChainHeader&> notifyRemoveBestChain;
    CoinQSignal<void> notifyBlockTreeChanged;
};

    }
}

