///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_netsync.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include "CoinQ_netsync.h"

#include "CoinQ_typedefs.h"

#include <CoinCore/MerkleTree.h>

#include <stdint.h>

#include <logger/logger.h>

using namespace CoinQ::Network;
using namespace std;

NetworkSync::NetworkSync(const CoinQ::CoinParams& coinParams, bool bCheckProofOfWork) :
    m_coinParams(coinParams),
    m_bCheckProofOfWork(bCheckProofOfWork),
    m_bStarted(false),
    m_bIOServiceStarted(false),
    m_ioServiceThread(nullptr),
    m_work(m_ioService),
    m_bConnected(false),
    m_peer(m_ioService),
    m_bFlushingToFile(false),
    m_fileFlushThread(nullptr),
    m_bHeadersSynched(false)
{
    // Select hash functions
    Coin::CoinBlockHeader::setHashFunc(m_coinParams.block_header_hash_function());
    Coin::CoinBlockHeader::setPOWHashFunc(m_coinParams.block_header_pow_hash_function());

/*
    // Subscribe block tree handlers 
    m_blockTree.subscribeRemoveBestChain([&](const ChainHeader& header)
    {
        notifyBlockTreeChanged();
        notifyRemoveBestChain(header);
    });

    m_blockTree.subscribeAddBestChain([&](const ChainHeader& header)
    {
        notifyBlockTreeChanged();
        notifyAddBestChain(header);
        uchar_vector hash = header.getHashLittleEndian();
        std::stringstream status;
        status << "Added to best chain: " << hash.getHex() << " Height: " << header.height << " ChainWork: " << header.chainWork.getDec();
        notifyStatus(status.str());
    });
*/

    // Subscribe peer handlers
    m_peer.subscribeOpen([&](CoinQ::Peer& /*peer*/)
    {
        m_bConnected = true;
        notifyOpen();
        try
        {
            if (m_bloomFilter.isSet())
            {
                Coin::FilterLoadMessage filterLoad(m_bloomFilter.getNHashFuncs(), m_bloomFilter.getNTweak(), m_bloomFilter.getNFlags(), m_bloomFilter.getFilter());
                m_peer.send(filterLoad);
                LOGGER(trace) << "Sent filter to peer." << std::endl;
            }

            LOGGER(trace) << "Peer connection opened." << endl;
            m_peer.getHeaders(m_blockTree.getLocatorHashes(-1));
        }
        catch (const std::exception& e)
        {
            LOGGER(error) << "NetworkSync - m_peer open handler - " << e.what() << std::endl;
            notifyBlockTreeError(e.what());
        }
    });

    m_peer.subscribeClose([this](CoinQ::Peer& /*peer*/)
    {
        if (m_bConnected) { stop(); }
        notifyClose();
    });

    m_peer.subscribeTimeout([&](CoinQ::Peer& /*peer*/)
    {
        notifyTimeout();
    });

    m_peer.subscribeConnectionError([&](CoinQ::Peer& /*peer*/, const std::string& error)
    {
        notifyConnectionError(error);
    });

    m_peer.subscribeProtocolError([&](CoinQ::Peer& /*peer*/, const std::string& error)
    {
        notifyProtocolError(error);
    });

    m_peer.subscribeInv([&](CoinQ::Peer& peer, const Coin::Inventory& inv)
    {
        if (!m_bConnected) return;
        LOGGER(trace) << "Received inventory message:" << std::endl << inv.toIndentedString(2) << std::endl;

        using namespace Coin;
        GetDataMessage getData;
        for (auto& item: inv.items)
        {
            switch (item.itemType)
            {
            case MSG_TX:
                getData.items.push_back(item);
                break;
            case MSG_BLOCK:
                getData.items.push_back(InventoryItem(MSG_FILTERED_BLOCK, item.hash));
                break;
            default:
                break;
            } 
        }

        if (!getData.items.empty()) { m_peer.send(getData); }
    });

    m_peer.subscribeTx([&](CoinQ::Peer& /*peer*/, const Coin::Transaction& tx)
    {
        if (!m_bConnected) return;
        uchar_vector txHash = tx.getHashLittleEndian();
        LOGGER(trace) << "Received transaction: " << txHash.getHex() << endl;

        if (m_currentMerkleTxHashes.empty())
        {
            m_mempoolTxs.insert(txHash);
            notifyNewTx(tx);
            return;
        }

        try
        {
            // Notify of confirmations of previously received transactions as well as the current one
            boost::lock_guard<boost::mutex> syncLock(m_syncMutex);
            processMempoolConfirmations();
            if (!m_currentMerkleTxHashes.empty() && txHash == m_currentMerkleTxHashes.front())
            {
                LOGGER(trace) << "New merkle transaction (" << (m_currentMerkleTxIndex + 1) << " of " << m_currentMerkleTxCount << "): " << txHash.getHex() << endl;
                notifyMerkleTx(m_currentMerkleBlock, tx, m_currentMerkleTxIndex++, m_currentMerkleTxCount);
                m_currentMerkleTxHashes.pop();
            }
            processMempoolConfirmations();

            // Once the queue is empty, if we're at the tip signal completion of block sync.
            if (m_currentMerkleTxHashes.empty())
            {
                uchar_vector currentMerkleBlockHash = m_currentMerkleBlock.blockHeader.getHashLittleEndian();
                const ChainHeader& chainTip = m_blockTree.getTip();
                if (chainTip.getHashLittleEndian() == currentMerkleBlockHash)
                {
                    LOGGER(trace) << "Block sync detected from tx handler." << endl;
                    m_lastRequestedBlockHash.clear();
                    m_lastSynchedBlockHash = currentMerkleBlockHash;
                    notifyBlocksSynched();
                    return;
                }

                // Ask for the next block
                const ChainHeader& nextHeader = m_blockTree.getHeader(m_currentMerkleBlock.height + 1);
                m_lastRequestedBlockHash = nextHeader.getHashLittleEndian();
                
                try
                {
                    m_peer.getFilteredBlock(m_lastRequestedBlockHash);
                }
                catch (const exception& e)
                {
                    notifyConnectionError(e.what());
                }
            }
        }
        catch (const exception& e)
        {
            LOGGER(error) << "Protocol error processing merkle transactions: " << e.what() << endl;
            notifyProtocolError(e.what());
        }
    });

    m_peer.subscribeHeaders([&](CoinQ::Peer& peer, const Coin::HeadersMessage& headersMessage)
    {
        if (!m_bConnected) return;
        LOGGER(trace) << "Received headers message..." << std::endl;

        try
        {
            if (headersMessage.headers.size() > 0)
            {
                notifySynchingHeaders();
                for (auto& item: headersMessage.headers)
                {
                    try
                    {
                        boost::unique_lock<boost::mutex> fileFlushLock(m_fileFlushMutex);
                        if (m_blockTree.insertHeader(item)) { m_bHeadersSynched = false; }
                    }
                    catch (const std::exception& e)
                    {
                        std::stringstream err;
                        err << "Block tree insertion error for block " << item.getHashLittleEndian().getHex() << ": " << e.what(); // TODO: localization
                        LOGGER(error) << err.str() << std::endl;
                        notifyBlockTreeError(err.str());
                        throw e;
                    }
                }

                LOGGER(trace)   << "Processed " << headersMessage.headers.size() << " headers."
                                << " mBestHeight: " << m_blockTree.getBestHeight()
                                << " mTotalWork: " << m_blockTree.getTotalWork().getDec()
                                << " Attempting to fetch more headers..." << std::endl;

                notifyBlockTreeChanged();
                std::stringstream status;
                status << "Best Height: " << m_blockTree.getBestHeight() << " / " << "Total Work: " << m_blockTree.getTotalWork().getDec();
                notifyStatus(status.str());

                peer.getHeaders(m_blockTree.getLocatorHashes(1));
            }
            else
            {
                m_fileFlushCond.notify_one();
/*
                if (!m_blockTree.flushed())
                {
                    notifyStatus("Flushing block chain to file...");
                    m_blockTree.flushToFile(m_blockTreeFile);
                    notifyStatus("Done flushing block chain to file");
                }
*/

                notifyBlockTreeChanged();
                if (!m_bHeadersSynched)
                {
                    m_bHeadersSynched = true;
                    notifyHeadersSynched();
                }
            }
        }
        catch (const std::exception& e)
        {
            LOGGER(error) << "block tree exception: " << e.what() << std::endl;
        }
    });

    m_peer.subscribeBlock([&](CoinQ::Peer& /*peer*/, const Coin::CoinBlock& block)
    {
        if (!m_bConnected) return;
        uchar_vector hash = block.blockHeader.getHashLittleEndian();
        boost::lock_guard<boost::mutex> lock(m_syncMutex);
        try {
            if (m_blockTree.hasHeader(hash))
            {
                notifyBlock(block);
            }
            else if (m_blockTree.insertHeader(block.blockHeader))
            {
                notifyStatus("Flushing block chain to file...");
                m_blockTree.flushToFile(m_blockTreeFile);
                m_bHeadersSynched = true;
                notifyStatus("Done flushing block chain to file");
                notifyHeadersSynched();
                notifyBlock(block);
            }
            else
            {
                LOGGER(debug) << "NetworkSync block handler - block rejected - hash: " << hash.getHex() << std::endl;
            }
        }
        catch (const std::exception& e)
        {
            LOGGER(error) << "NetworkSync block handler - block hash: " << hash.getHex() << " - " << e.what() << std::endl;
            notifyStatus("NetworkSync block handler error.");
        }
    });

    m_peer.subscribeMerkleBlock([&](CoinQ::Peer& /*peer*/, const Coin::MerkleBlock& merkleBlock)
    {
        if (!m_bConnected) return;

        uchar_vector merkleBlockHash = merkleBlock.blockHeader.getHashLittleEndian();
        LOGGER(trace) << "Received merkle block: " << merkleBlockHash.getHex() << endl;

        const ChainHeader& chainTip = m_blockTree.getHeader(-1);
        uchar_vector chainTipHash = chainTip.getHashLittleEndian();
        LOGGER(trace) << "Current chain tip: " << chainTipHash.getHex() << " Height: " << chainTip.height << endl;

        try
        {
            // Constructing the partial tree will validate the merkle root - throws exception if invalid.
            Coin::PartialMerkleTree merkleTree(merkleBlock.nTxs, merkleBlock.hashes, merkleBlock.flags, merkleBlock.blockHeader.merkleRoot);

            LOGGER(debug) << "Last requested merkle block: " << m_lastRequestedBlockHash.getHex() << endl;
            boost::lock_guard<boost::mutex> syncLock(m_syncMutex);
            if (merkleBlockHash == m_lastRequestedBlockHash)
            {
                // It's the block we requested - sync it and continue requesting the next until we're at the tip
                const ChainHeader& merkleHeader = m_blockTree.getHeader(merkleBlockHash);
                syncMerkleBlock(ChainMerkleBlock(merkleBlock, true, merkleHeader.height, merkleHeader.chainWork), merkleTree);

                if (merkleBlockHash == chainTipHash)
                {
                    // We're at the tip
                    LOGGER(trace) << "Block sync detected from merkle block handler." << endl;
                    m_lastRequestedBlockHash.clear();
                    m_lastSynchedBlockHash = chainTipHash;
                    notifyBlocksSynched();
                }
                else
                {
                    // Ask for the next block
                    const ChainHeader& nextHeader = m_blockTree.getHeader(merkleHeader.height + 1);
                    m_lastRequestedBlockHash = nextHeader.getHashLittleEndian();
    
                    try
                    {
                        m_peer.getFilteredBlock(m_lastRequestedBlockHash);
                    }
                    catch (const exception& e)
                    {
                        notifyConnectionError(e.what());
                    }
                }
            }
            else if ((merkleBlock.blockHeader.prevBlockHash == chainTipHash) ||
                (merkleBlock.blockHeader.prevBlockHash == chainTip.prevBlockHash && merkleBlock.blockHeader.getWork() > chainTip.getWork()))
            {
                // The merkle block either connects to the current tip or it replaces the current tip (depth 1 reorg)
                // TODO: properly handle proof-of-stake

                // Try inserting into block tree. If it fails it throws a protocol error exception which is caught below.
                boost::unique_lock<boost::mutex> fileFlushLock(m_fileFlushMutex);
                m_blockTree.insertHeader(merkleBlock.blockHeader, m_bCheckProofOfWork);
                fileFlushLock.unlock();

                // Start flushing to file
                m_fileFlushCond.notify_one();

                notifyBlockTreeChanged();

                if (m_lastSynchedBlockHash == chainTipHash)
                {
                    // We were synched prior to this block - we need to process this merkle block and we'll be synched again
                    notifySynchingBlocks();
                    m_lastSynchedBlockHash = m_blockTree.getTip().getHashLittleEndian();
                    m_lastRequestedBlockHash.clear();
                    const ChainHeader& merkleHeader = m_blockTree.getHeader(merkleBlockHash);
                    syncMerkleBlock(ChainMerkleBlock(merkleBlock, true, merkleHeader.height, merkleHeader.chainWork), merkleTree);
                }
            }
            else if (!m_blockTree.hasHeader(merkleBlockHash))
            {
                // A reorg of depth 2 or greater has occurred - update block headers
                LOGGER(trace) << "NetworkSync merkle block handler - block rejected: " << merkleBlockHash.getHex() << endl;

                LOGGER(trace) << "REORG - resynching block headers from peer..." << endl;
                m_bHeadersSynched = false;
                try
                {
                    m_peer.getHeaders(m_blockTree.getLocatorHashes(-1));
                }
                catch (const exception& e)
                {
                    LOGGER(error) << "Block tree error: " << e.what() << endl;
                    notifyBlockTreeError(e.what());
                }
            }
/*
            // Constructing the partial tree will validate the merkle root - throws exception if invalid.
            Coin::PartialMerkleTree merkleTree(merkleBlock.nTxs, merkleBlock.hashes, merkleBlock.flags, merkleBlock.blockHeader.merkleRoot);
            LOGGER(trace) << "Merkle Tree:" << std::endl << merkleTree.toIndentedString() << std::endl;

            if (!m_blockTree.hasHeader(hash))
            {
                LOGGER(trace) << "NetworkSync - subscribeMerkleBlock - blocktree does not have header." << endl;
                boost::unique_lock<boost::mutex> lock(m_fileFlushMutex);
                bool bHeaderInserted = m_blockTree.insertHeader(merkleBlock.blockHeader);
                lock.unlock(); 

                if (bHeaderInserted)
                {
                    LOGGER(trace) << "NetworkSync - subscribeMerkleBlock - header was inserted." << endl;
                    m_fileFlushCond.notify_one();

                    m_bHeadersSynched = true;
                    m_bBlocksSynched = false;
                    notifyBlockTreeChanged();
                }
                else
                {
                    m_bHeadersSynched = false;
                    m_bBlocksSynched = false;

                    // Possible reorg
                    LOGGER(error) << "NetworkSync merkle block handler - block rejected: " << hash.getHex() << " - possible reorg." << endl;
                    try
                    {
                        m_peer.getHeaders(m_blockTree.getLocatorHashes(-1));
                    }
                    catch (const exception& e) {
                        LOGGER(error) << "Block tree error: " << e.what() << endl;
                        notifyBlockTreeError(e.what());
                    }
                    return;
                }
            }

            // TODO: queue received merkleblocks locally so we do not need to refetch from peer and can support downloading out-of-order from multiple peers.
            LOGGER(trace) << "m_bFetchingBlocks: " << (m_bFetchingBlocks ? "true" : "false") << " - m_bHeadersSynched: " << (m_bHeadersSynched ? "true" : "false") << std::endl;
            if (m_bFetchingBlocks && m_bHeadersSynched)
            {
                notifySynchingBlocks();

                ChainHeader header = m_blockTree.getHeader(hash);
                LOGGER(trace) << "m_lastRequestedBlockHeight: " << m_lastRequestedBlockHeight << ", header.height: " << header.height << std::endl;
                if ((uint32_t)header.height == m_lastRequestedBlockHeight)
                {
                    // Set tx hashes
                    m_currentMerkleBlock = ChainMerkleBlock(merkleBlock, true, header.height, header.chainWork);
                    std::vector<uchar_vector> txhashes = tree.getTxHashesLittleEndianVector();

                    while (!m_currentMerkleTxHashes.empty()) { m_currentMerkleTxHashes.pop(); }
                    for (auto& txhash: txhashes) { m_currentMerkleTxHashes.push(txhash); }
                    m_currentMerkleTxIndex = 0;
                    m_currentMerkleTxCount = txhashes.size();

                    if (m_currentMerkleTxCount > 0)
                    {
                        processMempoolConfirmations();
                    }
                    else
                    {
                        notifyMerkleBlock(m_currentMerkleBlock);
                    }

                    uint32_t bestHeight = m_blockTree.getBestHeight();
                    LOGGER(trace) << "bestHeight: " << bestHeight << endl;
                    if (bestHeight > (uint32_t)header.height)
                    {
                        // We still have more blocks to fetch
                        m_bBlocksFetched = false;
                        const ChainHeader& nextHeader = m_blockTree.getHeader(header.height + 1);
                        uchar_vector nextHash = nextHeader.getHashLittleEndian();
                        std::string hashString = nextHash.getHex();

                        std::stringstream status;
                        status << "Asking for block " << hashString << " / height: " << nextHeader.height;
                        LOGGER(debug) << status.str() << std::endl;
                        notifyStatus(status.str());
                        m_lastRequestedBlockHeight = nextHeader.height;
                        try
                        {
                            m_peer.getFilteredBlock(nextHash);
                        }
                        catch (const exception& e)
                        {
                            notifyConnectionError(e.what());
                        }
                    }
                    else if (bestHeight == m_lastRequestedBlockHeight)
                    {
                        // No more blocks to fetch for now
                        m_bBlocksFetched = true;
                        m_lastRequestedBlockHeight++; // The request is not made explicitly - it is implied. Now we wait...

                        // If filtered block contains none of our transactions, we must signal completion here rather than in tx handler
                        if (m_currentMerkleTxHashes.empty())
                        {
                            LOGGER(trace) << "Block sync detected from merkle block handler." << endl;
                            m_bBlocksSynched = true;
                            notifyBlocksSynched();
                        }
                    }
                }
            }
*/
        }
        catch (const exception& e)
        {
            LOGGER(error) << "NetworkSync - protocol error: " << e.what() << std::endl;
            notifyProtocolError(e.what());
        }
    });
}

NetworkSync::~NetworkSync()
{
    stop();
}

void NetworkSync::setCoinParams(const CoinQ::CoinParams& coinParams)
{
    if (m_bStarted) throw std::runtime_error("NetworkSync::setCoinParams() - must be stopped to set coin parameters.");
    boost::lock_guard<boost::mutex> lock(m_startMutex);
    if (m_bStarted) throw std::runtime_error("NetworkSync::setCoinParams() - must be stopped to set coin parameters.");

    m_coinParams = coinParams;    
}

void NetworkSync::loadHeaders(const std::string& blockTreeFile, bool bCheckProofOfWork, CoinQBlockTreeMem::callback_t callback)
{
    stopFileFlushThread();
    m_blockTreeFile = blockTreeFile;

    try
    {
        m_blockTree.loadFromFile(blockTreeFile, bCheckProofOfWork, callback);

        std::stringstream status;
        status << "Best Height: " << m_blockTree.getBestHeight() << " / " << "Total Work: " << m_blockTree.getTotalWork().getDec();
        notifyStatus(status.str());
        notifyAddBestChain(m_blockTree.getHeader(-1));
        return;
    }
    catch (const std::exception& e)
    {
        LOGGER(error) << "NetworkSync::loadHeaders() - " << e.what() << std::endl;
        notifyBlockTreeError(e.what());
    }

    m_blockTree.clear();
    m_blockTree.setGenesisBlock(m_coinParams.genesis_block());
    notifyStatus("Block tree file not found. A new one will be created.");
    notifyAddBestChain(m_blockTree.getHeader(-1));
}

/*
void NetworkSync::initBlockFilter()
{
    blockTree.unsubscribeAll();

    blockTree.subscribeRemoveBestChain([&](const ChainHeader& header) {
        notifyBlockTreeChanged();
        notifyRemoveBestChain(header);
    });

    blockTree.subscribeAddBestChain([&](const ChainHeader& header) {
        notifyBlockTreeChanged();
        notifyAddBestChain(header);
        uchar_vector hash = header.getHashLittleEndian();
        std::stringstream status;
        status << "Added to best chain: " << hash.getHex() << " Height: " << header.height << " ChainWork: " << header.chainWork.getDec();
        notifyStatus(status.str());
    });

    blockFilter.clear();
    blockFilter.connect([&](const ChainBlock& block) {
        uchar_vector blockHash = block.blockHeader.getHashLittleEndian();
        std::stringstream status;
        status << "Got block: " << blockHash.getHex() << " Height: " << block.height << " ChainWork: " << block.chainWork.getDec() << " # txs: " << block.txs.size();
        notifyStatus(status.str());
        for (auto& tx: block.txs) {
            notifyTx(tx);
        }

        notifyBlock(block);
        if (
            //resynching &&
             blockTree.getBestHeight() > block.height) {
            const ChainHeader& nextHeader = blockTree.getHeader(block.height + 1);
            uchar_vector blockHash = nextHeader.getHashLittleEndian();
            std::stringstream status;
            status << "Asking for block " << blockHash.getHex() << " Height: " << nextHeader.height;
            notifyStatus(status.str());
            peer.getBlock(blockHash);
        }
        else {
            resynching = false;
            notifyDoneBlockSync();
            //peer.getMempool();
        }
    });
}
*/

int NetworkSync::getBestHeight()
{
    return m_blockTree.getBestHeight();
}

void NetworkSync::syncBlocks(const std::vector<bytes_t>& locatorHashes, uint32_t startTime)
{
    if (!m_bConnected) throw runtime_error("NetworkSync::syncBlocks() - must connect before synching.");
    if (!m_bHeadersSynched) throw runtime_error("NetworkSync::syncBlocks() - headers must be synched before calling.");

    int startHeight;

    {
        boost::lock_guard<boost::mutex> syncLock(m_syncMutex);
        m_lastRequestedBlockHash.clear();
        m_lastSynchedBlockHash.clear();

        const ChainHeader* pMostRecentHeader = nullptr;
        for (auto& hash: locatorHashes)
        {
            try
            {
                pMostRecentHeader = &m_blockTree.getHeader(hash);
                if (pMostRecentHeader->inBestChain) break;
                pMostRecentHeader = nullptr;

                LOGGER(trace) << "reorg detected at height " << pMostRecentHeader->height << std::endl;
            }
            catch (const std::exception& e)
            {
                LOGGER(error) << "Block tree error: " << e.what() << endl;
                notifyBlockTreeError(e.what());
            }
        }


        if (pMostRecentHeader)
        {
            if (m_blockTree.getTipHeight() == pMostRecentHeader->height)
            {
                m_lastSynchedBlockHash = pMostRecentHeader->getHashLittleEndian();
                notifyBlocksSynched();
                return;
            } 
            else
            {
                startHeight = pMostRecentHeader->height + 1;
            }
        }
        else
        {
            startHeight = m_blockTree.getHeaderBefore(startTime).height;
        }
    }

    syncBlocks(startHeight);
}

void NetworkSync::syncBlocks(int startHeight)
{
    boost::lock_guard<boost::mutex> syncLock(m_syncMutex);
    m_lastRequestedBlockHash = m_blockTree.getHeader(startHeight).getHashLittleEndian();

    LOGGER(trace) "Resynching blocks " << startHeight << " - " << m_blockTree.getTipHeight() << endl;
    notifySynchingBlocks();

    LOGGER(trace) << "Asking for block " << m_lastRequestedBlockHash.getHex() << endl;
    m_peer.getFilteredBlock(m_lastRequestedBlockHash);
}
/*
void NetworkSync::resync(int resyncHeight)
{
    if (!isConnected_) throw std::runtime_error("Must connect before resynching."); 

    stopResync();

    boost::lock_guard<boost::mutex> lock(mutex);

    resynching = true;

    initBlockFilter();

    const ChainHeader& bestHeader = blockTree.getHeader(-1); // gets the best chain's head.
    if (resyncHeight < 0) resyncHeight += bestHeader.height;

    if (bestHeader.height >= resyncHeight) {
        std::stringstream status;
        status << "Resynching blocks " << resyncHeight << " - " << bestHeader.height;
        notifyStatus(status.str());
        const ChainHeader& resyncHeader = blockTree.getHeader(resyncHeight);
        uchar_vector blockHash = resyncHeader.getHashLittleEndian();
        status.str("");
        status << "Asking for block " << blockHash.getHex();
        notifyStatus(status.str());
        lastResyncHeight = (uint32_t)resyncHeight;
        peer.getFilteredBlock(blockHash);
    }

    //peer.getMempool();
}
*/

void NetworkSync::stopSynchingBlocks()
{
    boost::lock_guard<boost::mutex> lock(m_syncMutex);
    m_lastRequestedBlockHash.clear();
    m_lastSynchedBlockHash.clear();
    clearBloomFilter();
}

void NetworkSync::start(const std::string& host, const std::string& port)
{
    {
        if (m_bStarted) throw runtime_error("NetworkSync - already started.");
        boost::lock_guard<boost::mutex> lock(m_startMutex);
        if (m_bStarted) throw runtime_error("NetworkSync - already started.");
    
        LOGGER(trace) << "NetworkSync::start(" << host << ", " << port << ")" << std::endl;
        startFileFlushThread();
        startIOServiceThread();

        m_bStarted = true;

        std::string port_ = port.empty() ? m_coinParams.default_port() : port;
        m_peer.set(host, port_, m_coinParams.magic_bytes(), m_coinParams.protocol_version(), "Wallet v0.1", 0, false);

        LOGGER(trace) << "Starting peer " << host << ":" << port_ << "..." << endl;
        m_peer.start();
        LOGGER(trace) << "Peer started." << endl;
    }

    notifyStarted();
}

void NetworkSync::start(const std::string& host, int port)
{
    std::stringstream ssport;
    ssport << port;
    start(host, ssport.str());
}

void NetworkSync::stop()
{
    {
        if (!m_bStarted) return;
        boost::lock_guard<boost::mutex> lock(m_startMutex);
        if (!m_bStarted) return;

        m_bConnected = false;
        m_peer.stop();
        stopIOServiceThread();
        stopFileFlushThread();

        m_bStarted = false;
        m_bHeadersSynched = false;
        m_lastRequestedBlockHash.clear();
        while (!m_currentMerkleTxHashes.empty()) { m_currentMerkleTxHashes.pop(); }
    }

    notifyStopped();
}

void NetworkSync::sendTx(Coin::Transaction& tx)
{
    m_peer.send(tx); 
}

void NetworkSync::getTx(const bytes_t& hash)
{
    m_peer.getTx(hash);
}

void NetworkSync::getTxs(const hashvector_t& hashes)
{
    m_peer.getTxs(hashes);
}

void NetworkSync::getMempool()
{
    m_peer.getMempool();
}

void NetworkSync::getFilteredBlock(const bytes_t& hash)
{
    m_peer.getFilteredBlock(hash);
}

void NetworkSync::setBloomFilter(const Coin::BloomFilter& bloomFilter)
{
    m_bloomFilter = bloomFilter;
    if (!m_bloomFilter.isSet()) return;

    LOGGER(trace) << "Sending new bloom filter to peer." << endl;
    Coin::FilterLoadMessage filterLoad(m_bloomFilter.getNHashFuncs(), m_bloomFilter.getNTweak(), m_bloomFilter.getNFlags(), m_bloomFilter.getFilter());
    m_peer.send(filterLoad);
}

void NetworkSync::clearBloomFilter()
{
    LOGGER(trace) << "Clearing bloom filter." << endl;
    Coin::FilterClearMessage filterClear;
    m_peer.send(filterClear);
}

void NetworkSync::startIOServiceThread()
{
    if (m_bIOServiceStarted) throw std::runtime_error("NetworkSync - io service already started.");
    boost::lock_guard<boost::mutex> lock(m_ioServiceMutex);
    if (m_bIOServiceStarted) throw std::runtime_error("NetworkSync - io service already started.");

    LOGGER(trace) << "Starting IO service thread..." << endl;
    m_bIOServiceStarted = true;
    m_ioServiceThread = new boost::thread(boost::bind(&CoinQ::io_service_t::run, &m_ioService));
    LOGGER(trace) << "IO service thread started." << endl;
}

void NetworkSync::stopIOServiceThread()
{
    if (!m_bIOServiceStarted) return;
    boost::lock_guard<boost::mutex> lock(m_ioServiceMutex);
    if (!m_bIOServiceStarted) return;

    LOGGER(trace) << "Stopping IO service thread..." << endl;
    m_ioService.stop();
    m_ioServiceThread->join();
    delete m_ioServiceThread;
    m_ioService.reset();
    m_bIOServiceStarted = false;
    LOGGER(trace) << "IO service thread stopped." << endl; 
}

void NetworkSync::startFileFlushThread()
{
    if (m_bFlushingToFile) throw std::runtime_error("NetworkSync - file flush thread already started.");
    boost::lock_guard<boost::mutex> lock(m_fileFlushMutex);
    if (m_bFlushingToFile) throw std::runtime_error("NetworkSync - file flush thread already started.");

    LOGGER(trace) << "Starting file flushing thread..." << endl;
    m_bFlushingToFile = true;
    m_fileFlushThread = new boost::thread(&NetworkSync::fileFlushLoop, this);
    LOGGER(trace) << "File flushing thread started." << endl;
}

void NetworkSync::stopFileFlushThread()
{
    if (!m_bFlushingToFile) return;
    boost::unique_lock<boost::mutex> lock(m_fileFlushMutex);
    if (!m_bFlushingToFile) return;

    LOGGER(trace) << "Stopping file flushing thread..." << endl;
    m_bFlushingToFile = false;
    lock.unlock();
    m_fileFlushCond.notify_all();
    m_fileFlushThread->join();
    delete m_fileFlushThread;
    LOGGER(trace) << "File flushing thread stopped." << endl;
}

void NetworkSync::fileFlushLoop()
{
    while (true)
    {
        boost::unique_lock<boost::mutex> lock(m_fileFlushMutex);
        while (m_bFlushingToFile && m_blockTree.flushed()) { m_fileFlushCond.wait(lock); }
        if (!m_bFlushingToFile) break;

        LOGGER(trace) << "Starting blocktree file flush..." << endl;
        m_blockTree.flushToFile(m_blockTreeFile);
        LOGGER(trace) << "Finished flushing blocktree file." << endl;
    }
}

// merkleBlockHash is redundant, but we pass it anyways to avoid having to recompute it
void NetworkSync::syncMerkleBlock(const ChainMerkleBlock& merkleBlock, const Coin::PartialMerkleTree& merkleTree)
{
    // TODO: store the hash in block headers for quick repeated access
    uchar_vector merkleBlockHash = merkleBlock.blockHeader.getHashLittleEndian();

    LOGGER(trace) << "Synchronizing merkle block: " << merkleBlock.blockHeader.getHashLittleEndian().getHex() << endl;

    while (!m_currentMerkleTxHashes.empty()) { m_currentMerkleTxHashes.pop(); }

    // The byte order of the tx hashes must be reversed when moving between merkle trees and the block chain
    const std::list<uchar_vector>& reversedTxHashes = merkleTree.getTxHashes();
    m_currentMerkleTxCount = reversedTxHashes.size();
    int i = 0;
    for (auto& reversedTxHash: merkleTree.getTxHashes())
    {
        uchar_vector txHash = reversedTxHash.getReverse();
        m_currentMerkleTxHashes.push(txHash);
        LOGGER(trace) << "  Added tx to queue (" << ++i << " of " << m_currentMerkleTxCount << "): " << txHash.getHex() << endl;
    }
    
    if (m_currentMerkleTxCount == 0)
    {
        notifyMerkleBlock(merkleBlock);
    }
    else
    {
        // Set up merkle confirmation state
        m_currentMerkleBlock = merkleBlock;
        m_currentMerkleTxIndex = 0;

        // Confirm any transactions already in our mempool before letting tx handler do anything
        processMempoolConfirmations();
    }
}

void NetworkSync::processMempoolConfirmations()
{
    LOGGER(trace) << "Confirming from " << m_mempoolTxs.size() << " mempool transactions..." << endl;
    while (!m_currentMerkleTxHashes.empty() && m_mempoolTxs.count(m_currentMerkleTxHashes.front()))
    {
        const uchar_vector& txHash = m_currentMerkleTxHashes.front();
        LOGGER(trace) << "  Confirming tx (" << (m_currentMerkleTxIndex + 1) << " of " << m_currentMerkleTxCount << "): " << txHash.getHex() << endl;
        notifyTxConfirmed(m_currentMerkleBlock, txHash, m_currentMerkleTxIndex++, m_currentMerkleTxCount);
        m_mempoolTxs.erase(txHash);
        m_currentMerkleTxHashes.pop();
    }
}
