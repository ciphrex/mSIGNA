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

#include <thread>
#include <chrono>

using namespace CoinQ::Network;
using namespace std;

NetworkSync::NetworkSync(const CoinQ::CoinParams& coinParams, bool bCheckProofOfWork) :
    m_coinParams(coinParams),
    m_bCheckProofOfWork(bCheckProofOfWork),
    m_bStarted(false),
    m_bIOServiceStarted(false),
    m_work(m_ioService),
    m_bConnected(false),
    m_peer(m_ioService),
    m_bFlushingToFile(false),
    m_bHeadersSynched(false),
    m_bMissingTxs(false)
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
        uchar_vector hash = header.hash();
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
            // TODO: propagate code
            notifyBlockTreeError(e.what(), -1);
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

    m_peer.subscribeConnectionError([&](CoinQ::Peer& /*peer*/, const std::string& error, int code)
    {
        notifyConnectionError(error, code);
    });

    m_peer.subscribeProtocolError([&](CoinQ::Peer& /*peer*/, const std::string& error, int code)
    {
        notifyProtocolError(error, code);
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
        LOGGER(trace) << "Received transaction: " << tx.hash().getHex() << endl;

        boost::unique_lock<boost::mutex> syncLock(m_syncMutex);
        if (m_currentMerkleTxHashes.empty())
        {
            {
                boost::lock_guard<boost::mutex> mempoolLock(m_mempoolMutex);
                m_mempoolTxs.insert(tx.hash());
            }

            syncLock.unlock();
            notifyNewTx(tx);
        }
        else if (!m_bMissingTxs)
        {
            syncLock.unlock();
            processBlockTx(tx);
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
                        err << "Block tree insertion error for block " << item.hash().getHex() << ": " << e.what(); // TODO: localization
                        LOGGER(error) << err.str() << std::endl;
                        // TODO: propagate code
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

                vector<uchar_vector> locatorHashes = m_blockTree.getLocatorHashes(1);
                if (locatorHashes.empty()) throw runtime_error("Blocktree is empty.");
                if (headersMessage.headers[headersMessage.headers.size() - 1].hash() != locatorHashes[0])
                {
                    throw runtime_error("Blocktree conflicts with peer.");
                }
 
                peer.getHeaders(locatorHashes);
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
            notifyBlockTreeError(e.what(), -1);
        }
    });

    m_peer.subscribeBlock([&](CoinQ::Peer& /*peer*/, const Coin::CoinBlock& block)
    {
        if (!m_bConnected) return;

        LOGGER(trace) << "Received block: " << block.hash().getHex() << endl;

        try
        {
            boost::unique_lock<boost::mutex> syncLock(m_syncMutex);
            if (!m_bMissingTxs || m_currentMerkleBlock.hash() != block.hash()) return;    // Not the block we're working on.

            LOGGER(trace) << "Processing " << block.txs.size() << " block transactions..." << endl;
            for (auto& tx: block.txs)
            {
                if (m_currentMerkleTxHashes.empty()) break; // We got all our transactions.

                if (tx.hash() == m_currentMerkleTxHashes.front())
                {
                    LOGGER(trace) << "New merkle transaction (" << (m_currentMerkleTxIndex + 1) << " of " << m_currentMerkleTxCount << "): " << tx.hash().getHex() << endl;

                    notifyMerkleTx(m_currentMerkleBlock, tx, m_currentMerkleTxIndex++, m_currentMerkleTxCount);
                    m_currentMerkleTxHashes.pop();

                    {
                        boost::lock_guard<boost::mutex> mempoolLock(m_mempoolMutex);
                        m_mempoolTxs.erase(tx.hash());
                    }
                }
            }

            if (!m_currentMerkleTxHashes.empty())
            {
                // In principle this should never happen. If it does we missed some earlier check.
                throw runtime_error("Block is missing some transactions.");
            }

            m_bMissingTxs = false;

            // Once the queue is empty, if we're at the tip signal completion of block sync.
            uchar_vector currentMerkleBlockHash = m_currentMerkleBlock.hash();
            const ChainHeader& chainTip = m_blockTree.getTip();
            if (chainTip.hash() == currentMerkleBlockHash)
            {
                LOGGER(trace) << "Block sync detected from block handler." << endl;
                m_lastRequestedMerkleBlockHash.clear();
                m_lastSynchedMerkleBlockHash = currentMerkleBlockHash;
                syncLock.unlock();
                notifyBlocksSynched();
                return;
            }

            // Ask for the next block
            const ChainHeader& nextHeader = m_blockTree.getHeader(m_currentMerkleBlock.height + 1);
            m_lastRequestedMerkleBlockHash = nextHeader.hash();
            LOGGER(trace) << "Asking for filtered block from block handler: " << m_lastRequestedMerkleBlockHash.getHex() << std::endl;
            
            try
            {
                m_peer.getFilteredBlock(m_lastRequestedMerkleBlockHash);
            }
            catch (const exception& e)
            {
                // TODO: Propagate code
                syncLock.unlock();
                notifyConnectionError(e.what(), -1);
            }
        }
        catch (const exception& e)
        {
            // TODO: Propagate code
            notifyProtocolError(e.what(), -1);
        }
    });

    m_peer.subscribeMerkleBlock([&](CoinQ::Peer& /*peer*/, const Coin::MerkleBlock& merkleBlock)
    {
        if (!m_bConnected) return;

        uchar_vector merkleBlockHash = merkleBlock.hash();
        LOGGER(trace) << "Received merkle block: " << merkleBlockHash.getHex() << endl;

        const ChainHeader& chainTip = m_blockTree.getHeader(-1);
        uchar_vector chainTipHash = chainTip.hash();
        LOGGER(trace) << "Current chain tip: " << chainTipHash.getHex() << " Height: " << chainTip.height << endl;

        try
        {
            // Constructing the partial tree will validate the merkle root - throws exception if invalid.
            Coin::PartialMerkleTree merkleTree(merkleBlock.merkleTree());

            LOGGER(debug) << "Last requested merkle block: " << m_lastRequestedMerkleBlockHash.getHex() << endl;

            if (!m_bHeadersSynched)
            {
                LOGGER(trace) << "NetworkSync merkle block handler  - Headers are still not synched." << endl;

                LOGGER(trace) << "REORG - attempting again to resync block headers from peer..." << endl;
                try
                {
                    m_peer.getHeaders(m_blockTree.getLocatorHashes(-1));
                }
                catch (const exception& e)
                {
                    LOGGER(error) << "Block tree error: " << e.what() << endl;
                    // TODO: propagate code
                    notifyBlockTreeError(e.what(), -1);
                }
            }

            boost::unique_lock<boost::mutex> syncLock(m_syncMutex);
            if (merkleBlockHash == m_lastRequestedMerkleBlockHash)
            {
                // It's the block we requested - sync it and continue requesting the next until we're at the tip
                const ChainHeader& merkleHeader = m_blockTree.getHeader(merkleBlockHash);
                syncMerkleBlock(ChainMerkleBlock(merkleBlock, true, merkleHeader.height, merkleHeader.chainWork), merkleTree);

                if (!m_currentMerkleTxHashes.empty()) return; // We need to wait for some transactions

                if (merkleBlockHash == chainTipHash)
                {
                    // We're at the tip
                    LOGGER(trace) << "Block sync detected from merkle block handler." << endl;
                    m_lastRequestedMerkleBlockHash.clear();
                    m_lastSynchedMerkleBlockHash = chainTipHash;
                    syncLock.unlock();
                    notifyBlocksSynched();
                }
                else
                {
                    // Ask for the next block
                    const ChainHeader& nextHeader = m_blockTree.getHeader(merkleHeader.height + 1);
                    m_lastRequestedMerkleBlockHash = nextHeader.hash();
                    LOGGER(trace) << "Asking for filtered block (2) " << m_lastRequestedMerkleBlockHash.getHex() << endl;
    
                    try
                    {
                        m_peer.getFilteredBlock(m_lastRequestedMerkleBlockHash);
                    }
                    catch (const exception& e)
                    {
                        syncLock.unlock();
                        // TODO: propagate code
                        notifyConnectionError(e.what(), -1);
                    }
                }
            }
            else if ((merkleBlock.prevBlockHash() == chainTipHash) ||
                (merkleBlock.prevBlockHash() == chainTip.prevBlockHash() && merkleBlock.getWork() > chainTip.getWork()))
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

                if (m_lastSynchedMerkleBlockHash == chainTipHash)
                {
                    // We were synched prior to this block - we need to process this merkle block and we'll be synched again
                    notifySynchingBlocks();
                    const ChainHeader& merkleHeader = m_blockTree.getHeader(merkleBlockHash);
                    syncMerkleBlock(ChainMerkleBlock(merkleBlock, true, merkleHeader.height, merkleHeader.chainWork), merkleTree);
                    if (m_currentMerkleTxHashes.empty())
                    {
                        m_lastSynchedMerkleBlockHash = m_blockTree.getTip().hash();
                        syncLock.unlock();
                        notifyBlocksSynched();
                    }
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
                    // TODO: propagate code
                    notifyBlockTreeError(e.what(), -1);
                }
            }
        }
        catch (const exception& e)
        {
            LOGGER(error) << "NetworkSync - protocol error: " << e.what() << std::endl;
            // TODO: propagate code
            notifyProtocolError(e.what(), -1);
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
        // TODO: propagate code
        notifyBlockTreeError(e.what(), -1);
    }

    m_blockTree.clear();
    m_blockTree.setGenesisBlock(m_coinParams.genesis_block());
    notifyStatus("Block tree file not found. A new one will be created.");
    notifyAddBestChain(m_blockTree.getHeader(-1));
}

int NetworkSync::getBestHeight() const
{
    return m_blockTree.getBestHeight();
}

const bytes_t& NetworkSync::getBestHash() const
{
    return m_blockTree.getBestHash();
}

void NetworkSync::syncBlocks(const std::vector<bytes_t>& locatorHashes, uint32_t startTime)
{
    if (!m_bConnected) throw runtime_error("NetworkSync::syncBlocks() - must connect before synching.");
    if (!m_bHeadersSynched) throw runtime_error("NetworkSync::syncBlocks() - headers must be synched before calling.");
    
    LOGGER(trace) << "NetworkSync::syncBlocks - locatorHashes: " << locatorHashes.size() << " startTime: " << startTime << endl;
    int startHeight;

    boost::lock_guard<boost::mutex> syncLock(m_syncMutex);

    const ChainHeader* pMostRecentHeader = nullptr;
    for (auto& hash: locatorHashes)
    {
        try
        {
            pMostRecentHeader = &m_blockTree.getHeader(hash);
            if (pMostRecentHeader && pMostRecentHeader->inBestChain) break;
            pMostRecentHeader = nullptr;
        }
        catch (const std::exception& e)
        {
            LOGGER(error) << "Block tree error: " << e.what() << endl;
            // TODO: propagate code
            notifyBlockTreeError(e.what(), -1);
        }
    }


    if (pMostRecentHeader)
    {
        if (m_blockTree.getTipHeight() == pMostRecentHeader->height)
        {
            m_lastSynchedMerkleBlockHash = pMostRecentHeader->hash();
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

    do_syncBlocks(startHeight);
}

void NetworkSync::syncBlocks(int startHeight)
{
    boost::lock_guard<boost::mutex> syncLock(m_syncMutex);
    do_syncBlocks(startHeight);
}

void NetworkSync::do_syncBlocks(int startHeight)
{
    m_lastSynchedMerkleBlockHash.clear();
    m_lastRequestedMerkleBlockHash = m_blockTree.getHeader(startHeight).hash();

    LOGGER(trace) "Resynching blocks " << startHeight << " - " << m_blockTree.getTipHeight() << endl;
    notifySynchingBlocks();

    LOGGER(trace) << "Asking for filtered block (3) " << m_lastRequestedMerkleBlockHash.getHex() << endl;
    m_peer.getFilteredBlock(m_lastRequestedMerkleBlockHash);
}

void NetworkSync::stopSynchingBlocks(bool bClearFilter)
{
    boost::lock_guard<boost::mutex> lock(m_syncMutex);
    m_lastRequestedMerkleBlockHash.clear();
    m_lastSynchedMerkleBlockHash.clear();
    if (bClearFilter) { clearBloomFilter(); }
}

void NetworkSync::addToMempool(const uchar_vector& txHash)
{
    boost::lock_guard<boost::mutex> mempoolLock(m_mempoolMutex);
    m_mempoolTxs.insert(txHash);
}

void NetworkSync::insertTx(const Coin::Transaction& tx)
{
    {
        boost::lock_guard<boost::mutex> mempoolLock(m_mempoolMutex);
        m_mempoolTxs.insert(tx.hash());
    }

    notifyNewTx(tx);
}

void NetworkSync::insertMerkleBlock(const Coin::MerkleBlock& merkleBlock, const vector<Coin::Transaction>& txs)
{
    boost::unique_lock<boost::mutex> fileFlushLock(m_fileFlushMutex);
    m_blockTree.insertHeader(merkleBlock.blockHeader, m_bCheckProofOfWork);

    const ChainHeader& merkleHeader = m_blockTree.getHeader(merkleBlock.hash());

    fileFlushLock.unlock();
    m_fileFlushCond.notify_one();
    
    ChainMerkleBlock chainMerkleBlock(merkleBlock, true, merkleHeader.height, merkleHeader.chainWork);

    LOGGER(trace) << "Inserting merkle block: " << merkleBlock.hash().getHex() << endl;

    boost::unique_lock<boost::mutex> syncLock(m_syncMutex);
    int n = txs.size();
    if (n == 0)
    {
        notifyMerkleBlock(chainMerkleBlock);
    }
    else
    {
        int i = 0;
        for (auto& tx: txs)
        {
            LOGGER(trace) << "New merkle transaction (" << (m_currentMerkleTxIndex + 1) << " of " << m_currentMerkleTxCount << "): " << tx.hash().getHex() << endl;
            notifyMerkleTx(chainMerkleBlock, tx, i++, n);

            {
                boost::lock_guard<boost::mutex> mempoolLock(m_mempoolMutex);
                m_mempoolTxs.erase(tx.hash());
            }
        }
    }
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
        m_lastRequestedMerkleBlockHash.clear();
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
    LOGGER(trace) << "Asking for block filtered (4) " << m_lastRequestedMerkleBlockHash.getHex() << endl;
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
    m_ioServiceThread = boost::thread(boost::bind(&CoinQ::io_service_t::run, &m_ioService));
    LOGGER(trace) << "IO service thread started." << endl;
}

void NetworkSync::stopIOServiceThread()
{
    if (!m_bIOServiceStarted) return;
    boost::lock_guard<boost::mutex> lock(m_ioServiceMutex);
    if (!m_bIOServiceStarted) return;

    LOGGER(trace) << "Stopping IO service thread..." << endl;
    m_ioService.stop();
    LOGGER(trace) << "Waiting for io_service::run() to exit..." << endl;
    while (!m_ioService.stopped()) { std::this_thread::sleep_for(std::chrono::microseconds(200)); }
    //LOGGER(trace) << "Joining IO service thread..." << endl;
    //m_ioServiceThread.join();
    LOGGER(trace) << "Resetting IO service..." << endl;
    m_ioService.reset();
    m_bIOServiceStarted = false;
    LOGGER(trace) << "IO service thread stopped." << endl; 
}

void NetworkSync::startFileFlushThread()
{
    if (m_bFlushingToFile) throw std::runtime_error("NetworkSync - file flush thread already started.");
    boost::unique_lock<boost::mutex> lock(m_fileFlushMutex);
    if (m_bFlushingToFile) throw std::runtime_error("NetworkSync - file flush thread already started.");

    LOGGER(trace) << "Starting file flushing thread..." << endl;
    m_bFlushingToFile = true;
    m_fileFlushThread = boost::thread(&NetworkSync::fileFlushLoop, this);
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
    m_fileFlushThread.join();
    LOGGER(trace) << "File flushing thread stopped." << endl;
}

void NetworkSync::fileFlushLoop()
{
    while (true)
    {
        boost::unique_lock<boost::mutex> lock(m_fileFlushMutex);
        while (m_bFlushingToFile && m_blockTree.flushed()) { m_fileFlushCond.wait(lock); }
        if (!m_bFlushingToFile) break;

        try
        {
            LOGGER(trace) << "Starting blocktree file flush..." << endl;
            m_blockTree.flushToFile(m_blockTreeFile);
            LOGGER(trace) << "Finished flushing blocktree file." << endl;
        }
        // TODO: use async timer
        catch (const BlockTreeException& e)
        {
            lock.unlock();
            LOGGER(error) << "Blocktree file flush error: " << e.what() << endl;
            notifyBlockTreeError(e.what(), e.code());
            LOGGER(trace) << "Retrying blocktree file flush in 5 seconds..." << endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
        catch (const exception& e)
        {
            lock.unlock();
            LOGGER(error) << "Blocktree file flush error: " << e.what() << endl;
            notifyBlockTreeError(e.what(), -1);
            LOGGER(trace) << "Retrying blocktree file flush in 5 seconds..." << endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}

void NetworkSync::syncMerkleBlock(const ChainMerkleBlock& merkleBlock, const Coin::PartialMerkleTree& merkleTree)
{
    LOGGER(trace) << "Synchronizing merkle block: " << merkleBlock.hash().getHex() << " height: " << merkleBlock.height << endl;

    while (!m_currentMerkleTxHashes.empty()) { m_currentMerkleTxHashes.pop(); }

    // The byte order of the tx hashes must be reversed when moving between merkle trees and the block chain
    const std::list<uchar_vector>& reversedTxHashes = merkleTree.getTxHashes();

    if (reversedTxHashes.empty())
    {
        notifyMerkleBlock(merkleBlock);
        return;
    }

    m_currentMerkleTxCount = reversedTxHashes.size();
    int i = 0;
    for (auto& reversedTxHash: merkleTree.getTxHashes())
    {
        uchar_vector txHash = reversedTxHash.getReverse();
        m_currentMerkleTxHashes.push(txHash);
        LOGGER(trace) << "  Added tx to queue (" << ++i << " of " << m_currentMerkleTxCount << "): " << txHash.getHex() << endl;
    }
    
    // Set up merkle confirmation state
    m_currentMerkleBlock = merkleBlock;
    m_currentMerkleTxIndex = 0;

    // Confirm any transactions already in our mempool before letting tx handler do anything
    processMempoolConfirmations();
}

void NetworkSync::processBlockTx(const Coin::Transaction& tx)
{
    string txHashHex = tx.hash().getHex();
    LOGGER(trace) << "NetworkSync::processBlockTx(" << txHashHex << ")" << endl;
    try
    {
        boost::unique_lock<boost::mutex> syncLock(m_syncMutex);

        // Notify of confirmations of previously received transactions as well as the current one
        processMempoolConfirmations();
        if (!m_currentMerkleTxHashes.empty())
        {
            if (tx.hash() == m_currentMerkleTxHashes.front())
            {
                LOGGER(trace) << "NetworkSync::processBlockTx - New merkle transaction (" << (m_currentMerkleTxIndex + 1) << " of " << m_currentMerkleTxCount << "): " << txHashHex << endl;
                notifyMerkleTx(m_currentMerkleBlock, tx, m_currentMerkleTxIndex++, m_currentMerkleTxCount);
                m_currentMerkleTxHashes.pop();
            }
            else if ((!m_lastRequestedMerkleBlockHash.empty()) && (m_lastRequestedBlockHash != m_lastRequestedMerkleBlockHash))
            {
                m_bMissingTxs = true;
                m_lastRequestedBlockHash = m_lastRequestedMerkleBlockHash;
                LOGGER(trace) << "We are missing some transactions in the mempool - perhaps due to reorg." << endl;
                LOGGER(trace) << "Asking for block " << m_lastRequestedBlockHash.getHex() << endl;
                try
                {
                    m_peer.getBlock(m_lastRequestedBlockHash);
                    LOGGER(debug) << "Got block " << m_lastRequestedBlockHash.getHex() << endl;
                }
                catch (const exception& e)
                {
                    m_lastRequestedBlockHash.clear();
                    // TODO: Propagate code
                    syncLock.unlock();
                    notifyConnectionError(e.what(), -1);
                }

                return;
            }
        }
        processMempoolConfirmations();

        if (!m_currentMerkleTxHashes.empty()) return; // we're still missing transactions

        // Once the queue is empty, if we're at the tip signal completion of block sync.
        uchar_vector currentMerkleBlockHash = m_currentMerkleBlock.hash();
        const ChainHeader& chainTip = m_blockTree.getTip();
        if (chainTip.hash() == currentMerkleBlockHash)
        {
            LOGGER(trace) << "Block sync detected from tx handler." << endl;
            m_lastRequestedMerkleBlockHash.clear();
            m_lastSynchedMerkleBlockHash = currentMerkleBlockHash;
            syncLock.unlock();
            notifyBlocksSynched();
            return;
        }

        // Ask for the next block
        const ChainHeader& nextHeader = m_blockTree.getHeader(m_currentMerkleBlock.height + 1);
        m_lastRequestedMerkleBlockHash = nextHeader.hash();
        LOGGER(trace) << "Asking for filtered block (1) " << m_lastRequestedMerkleBlockHash.getHex() << std::endl;
        
        try
        {
            m_peer.getFilteredBlock(m_lastRequestedMerkleBlockHash);
        }
        catch (const exception& e)
        {
            // TODO: Propagate code
            syncLock.unlock();
            notifyConnectionError(e.what(), -1);
        }
    }
    catch (const exception& e)
    {
        LOGGER(error) << "Protocol error processing merkle transactions: " << e.what() << endl;
        // TODO: propagate code
        notifyProtocolError(e.what(), -1);
    }
}

void NetworkSync::processMempoolConfirmations()
{
    boost::unique_lock<boost::mutex> mempoolLock(m_mempoolMutex);
    LOGGER(trace) << "Confirming " << m_currentMerkleTxHashes.size() << " merkle block transactions from " << m_mempoolTxs.size() << " mempool transactions..." << endl;
    while (!m_currentMerkleTxHashes.empty() && m_mempoolTxs.count(m_currentMerkleTxHashes.front()))
    {
        const uchar_vector& txHash = m_currentMerkleTxHashes.front();
        LOGGER(trace) << "  Confirming tx (" << (m_currentMerkleTxIndex + 1) << " of " << m_currentMerkleTxCount << "): " << txHash.getHex() << endl;
        mempoolLock.unlock();
        notifyTxConfirmed(m_currentMerkleBlock, txHash, m_currentMerkleTxIndex++, m_currentMerkleTxCount);

        mempoolLock.lock();
        m_mempoolTxs.erase(txHash);
        m_currentMerkleTxHashes.pop();
    }
    LOGGER(trace) << "Done processing mempool confirmations." << endl;
}
