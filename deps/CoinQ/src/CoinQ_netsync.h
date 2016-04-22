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

namespace Coin
{
    class PartialMerkleTree;
}

namespace CoinQ
{
    namespace Network
    {

typedef std::function<void(const ChainMerkleBlock&, const Coin::Transaction&, unsigned int /*txindex*/, unsigned int /*txcount*/)> merkle_tx_slot_t;
typedef std::function<void(const ChainMerkleBlock&, const bytes_t& /*txhash*/ , unsigned int /*txindex*/, unsigned int /*txcount*/)> tx_confirmed_slot_t;

class NetworkSync
{
public:
    NetworkSync(const CoinQ::CoinParams& coinParams = CoinQ::getBitcoinParams(), bool bCheckProofOfWork = false);
    ~NetworkSync();

    void setCoinParams(const CoinQ::CoinParams& coinParams);
    const CoinQ::CoinParams& getCoinParams() const { return m_coinParams; }

    void enableCheckProofOfWork(bool bCheckProofOfWork = true) { m_bCheckProofOfWork = bCheckProofOfWork; }

    void loadHeaders(const std::string& blockTreeFile, bool bCheckProofOfWork = true, CoinQBlockTreeMem::callback_t callback = nullptr);
    bool headersSynched() const { return m_bHeadersSynched; }
    int getBestHeight() const;
    const bytes_t& getBestHash() const;
    const ChainHeader& getBestHeader() const { return m_blockTree.getHeader(-1); }
    const ChainHeader& getHeader(const bytes_t& hash) const { return m_blockTree.getHeader(hash); }
    const ChainHeader& getHeader(int height) const { return m_blockTree.getHeader(height); }
    const ChainHeader& getHeaderBefore(uint32_t timestamp) const { return m_blockTree.getHeaderBefore(timestamp); }

/*
    void start();
    void stop();
    bool isStarted() const { return m_bPeerStarted; }
*/
    void start(const std::string& host, const std::string& port = "", unsigned int keepalive_timeout = 0);
    void start(const std::string& host, int port, unsigned int keepalive_timeout = 0);
    void stop();
    bool connected() const { return m_bConnected; }

    void setBloomFilter(const Coin::BloomFilter& bloomFilter);
    void clearBloomFilter();

    void syncBlocks(const std::vector<bytes_t>& locatorHashes, uint32_t startTime);
    void syncBlocks(int startHeight);
    void stopSynchingBlocks(bool bClearFilter = true);

    // TRANSACTIONS PUSHED OFF CHAIN MUST BE ADDED BACK TO MEMPOOL
    void addToMempool(const uchar_vector& txHash);

    // FOR TESTING
    void insertTx(const Coin::Transaction& tx);
    void insertMerkleBlock(const Coin::MerkleBlock& merkleBlock, const std::vector<Coin::Transaction>& txs);

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

    void subscribeConnectionError(error_slot_t slot) { notifyConnectionError.connect(slot); }
    void subscribeProtocolError(error_slot_t slot) { notifyProtocolError.connect(slot); }
    void subscribeBlockTreeError(error_slot_t slot) { notifyBlockTreeError.connect(slot); }

    void subscribeSynchingHeaders(void_slot_t slot) { notifySynchingHeaders.connect(slot); }
    void subscribeHeadersSynched(void_slot_t slot) { notifyHeadersSynched.connect(slot); }

    void subscribeSynchingBlocks(void_slot_t slot) { notifySynchingBlocks.connect(slot); }
    void subscribeBlocksSynched(void_slot_t slot) { notifyBlocksSynched.connect(slot); }

    void subscribeStatus(string_slot_t slot) { notifyStatus.connect(slot); }

    // PEER EVENT SUBSCRIPTIONS
    void subscribeNewTx(tx_slot_t slot) { notifyNewTx.connect(slot); }
    void subscribeMerkleTx(merkle_tx_slot_t slot) { notifyMerkleTx.connect(slot); }
    void subscribeTxConfirmed(tx_confirmed_slot_t slot) { notifyTxConfirmed.connect(slot); }

    void subscribeBlock(chain_block_slot_t slot) { notifyBlock.connect(slot); }
    void subscribeMerkleBlock(chain_merkle_block_slot_t slot) { notifyMerkleBlock.connect(slot); }
    void subscribeAddBestChain(chain_header_slot_t slot) { notifyAddBestChain.connect(slot); }
    void subscribeRemoveBestChain(chain_header_slot_t slot) { notifyRemoveBestChain.connect(slot); }
    void subscribeBlockTreeChanged(void_slot_t slot) { notifyBlockTreeChanged.connect(slot); }

private:
    CoinQ::CoinParams m_coinParams;
    bool m_bCheckProofOfWork;

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

    bool m_bFlushingToFile;
    boost::mutex m_fileFlushMutex;
    boost::condition_variable m_fileFlushCond;
    boost::thread m_fileFlushThread;
    void startFileFlushThread();
    void stopFileFlushThread();
    void fileFlushLoop();

    mutable boost::mutex m_syncMutex;
    std::string m_blockTreeFile;
    CoinQBlockTreeMem m_blockTree;
    bool m_blockTreeLoaded;
    bool m_bHeadersSynched;

    uchar_vector m_lastRequestedBlockHash;
    uchar_vector m_lastRequestedMerkleBlockHash;
    uchar_vector m_lastSynchedMerkleBlockHash;

    void do_syncBlocks(int startHeight);

    Coin::BloomFilter m_bloomFilter;

    void initBlockFilter();

    // Merkle block state
    mutable boost::mutex m_mempoolMutex;
    std::set<bytes_t> m_mempoolTxs;
    ChainMerkleBlock m_currentMerkleBlock;
    std::queue<bytes_t> m_currentMerkleTxHashes;
    unsigned int m_currentMerkleTxIndex;
    unsigned int m_currentMerkleTxCount;
    bool m_bMissingTxs;

    void syncMerkleBlock(const ChainMerkleBlock& merkleBlock, const Coin::PartialMerkleTree& merkleTree);
    void processBlockTx(const Coin::Transaction& tx);
    void processMempoolConfirmations();

    // Sync signals
    CoinQSignal<void> notifyStarted;
    CoinQSignal<void> notifyStopped;
    CoinQSignal<void> notifyOpen;
    CoinQSignal<void> notifyClose;
    CoinQSignal<void> notifyTimeout;

    CoinQSignal<const std::string&, int> notifyConnectionError;
    CoinQSignal<const std::string&, int> notifyProtocolError;
    CoinQSignal<const std::string&, int> notifyBlockTreeError;

    CoinQSignal<void> notifySynchingHeaders;
    CoinQSignal<void> notifyHeadersSynched;

    CoinQSignal<void> notifySynchingBlocks;
    CoinQSignal<void> notifyBlocksSynched;

    CoinQSignal<const std::string&> notifyStatus;

    // Peer signals
    CoinQSignal<const Coin::Transaction&> notifyNewTx;
    CoinQSignal<const ChainMerkleBlock&, const Coin::Transaction&, unsigned int, unsigned int> notifyMerkleTx;
    CoinQSignal<const ChainMerkleBlock&, const bytes_t&, unsigned int, unsigned int> notifyTxConfirmed;

    CoinQSignal<const ChainBlock&> notifyBlock;
    CoinQSignal<const ChainMerkleBlock&> notifyMerkleBlock;
    CoinQSignal<const ChainHeader&> notifyAddBestChain;
    CoinQSignal<const ChainHeader&> notifyRemoveBestChain;
    CoinQSignal<void> notifyBlockTreeChanged;
};

    }
}

