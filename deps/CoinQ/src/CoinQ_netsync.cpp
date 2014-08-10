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

NetworkSync::NetworkSync(const CoinQ::CoinParams& coinParams) :
    m_coinParams(coinParams),
    m_bStarted(false),
    m_bIOServiceStarted(false),
    m_ioServiceThread(nullptr),
    m_work(m_ioService),
    m_bConnected(false),
    m_peer(m_ioService),
    m_bFlushingToFile(false),
    m_fileFlushThread(nullptr),
    m_bHeadersSynched(false),
    m_bFetchingBlocks(false),
    m_bBlocksFetched(false),
    m_bBlocksSynched(false),
    m_lastRequestedBlockHeight(0)
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

//            notifyStatus("Fetching block headers...");
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
                if (m_bBlocksSynched)
                {
                    getData.items.push_back(item);
                }
                break;
            case MSG_BLOCK:
                if (m_bHeadersSynched)
                {
                    getData.items.push_back(InventoryItem(MSG_FILTERED_BLOCK, item.hash));
                }
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
        bytes_t txhash = tx.getHashLittleEndian();
        LOGGER(trace) << "Received transaction: " << uchar_vector(txhash).getHex() << std::endl;

        if (m_currentMerkleTxHashes.empty())
        {
            notifyNewTx(tx);
            m_mempoolTxs.insert(txhash);
            return;
        }

        try
        {
            if (m_bFetchingBlocks)
            {
                // Notify of confirmations of previously received transactions as well as the current one
                processConfirmations();
                if (!m_currentMerkleTxHashes.empty() && txhash == m_currentMerkleTxHashes.front())
                {
                    LOGGER(trace) << "New merkle transaction: " << m_currentMerkleTxIndex << " of " << m_currentMerkleTxCount << endl;
                    notifyMerkleTx(m_currentMerkleBlock, tx, m_currentMerkleTxIndex++, m_currentMerkleTxCount);
                    m_currentMerkleTxHashes.pop();
                }
                processConfirmations();

                // Once the queue is empty, signal completion of block sync.
                // The m_bBlocksSynched flag ensures we won't end up here again until we receive a new block.
                if (m_currentMerkleTxHashes.empty() && m_bBlocksFetched)
                {
                    LOGGER(trace) << "Block sync detected from tx handler." << endl;
                    m_bBlocksSynched = true;
                    notifyBlocksSynched();
                }
            }
            else
            {
                throw runtime_error("Should not be receiving transactions if not synched and not fetching blocks.");
            }
        }
        catch (const exception& e)
        {
            notifyProtocolError(e.what());

            // TODO: Attempt to recover
            
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
                boost::lock_guard<boost::mutex> syncLock(m_syncMutex);
                notifyFetchingHeaders();
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
                m_lastRequestedBlockHeight = m_blockTree.getBestHeight() + 1;
                m_bHeadersSynched = true;
                notifyHeadersSynched();
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

        uchar_vector hash = merkleBlock.blockHeader.getHashLittleEndian();
        LOGGER(trace) << "Received merkle block: " << hash.getHex() << std::endl;

        try
        {
            // Constructing the partial tree will validate the merkle root - throws exception if invalid.
            Coin::PartialMerkleTree tree(merkleBlock.nTxs, merkleBlock.hashes, merkleBlock.flags, merkleBlock.blockHeader.merkleRoot);
            LOGGER(trace) << "Merkle Tree:" << std::endl << tree.toIndentedString() << std::endl;

            if (!m_blockTree.hasHeader(hash))
            {
                boost::unique_lock<boost::mutex> lock(m_fileFlushMutex);
                bool bHeaderInserted = m_blockTree.insertHeader(merkleBlock.blockHeader);
                lock.unlock(); 

                if (bHeaderInserted)
                {
                    // TODO: Flushing block chain to file is a time consuming operation - we need to keep a local queue to store incoming messages
                    // and do the flushing in a separate thread.
    /*
                    notifyStatus("Flushing block chain to file...");
                    m_blockTree.flushToFile(m_blockTreeFile);
                    notifyStatus("Done flushing block chain to file");
    */

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
                notifyFetchingBlocks();

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
                        processConfirmations();
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
        }
        catch (const exception& e) {
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
    if (!m_bConnected) throw std::runtime_error("NetworkSync::syncBlocks() - must connect before synching.");

    if (!m_bHeadersSynched) throw std::runtime_error("NetworkSync::syncBlocks() - headers must be synched before calling.");

    m_bFetchingBlocks = true;
    m_bBlocksFetched = false;
    m_bBlocksSynched = false;

    ChainHeader header;
    bool foundHeader = false;
    for (auto& hash: locatorHashes)
    {
        try
        {
            header = m_blockTree.getHeader(hash);
            if (header.inBestChain)
            {
                foundHeader = true;
                break;
            }
            else
            {
                LOGGER(debug) << "reorg detected at height " << header.height << std::endl;
            }
        }
        catch (const std::exception& e) {
            notifyStatus(e.what());
        }
    }

    const ChainHeader& bestHeader = m_blockTree.getHeader(-1);

    int nextBlockRequestHeight;
    if (foundHeader)
    {
        nextBlockRequestHeight = header.height + 1;
    }
    else
    {
        ChainHeader header = m_blockTree.getHeaderBefore(startTime);
        nextBlockRequestHeight = header.height;
    }

    m_lastRequestedBlockHeight = (uint32_t)nextBlockRequestHeight;

    if (bestHeader.height >= nextBlockRequestHeight)
    {
        std::stringstream status;
        status << "Resynching blocks " << nextBlockRequestHeight << " - " << bestHeader.height;
        LOGGER(debug) << status.str() << std::endl;
        notifyStatus(status.str());
        notifyFetchingBlocks();
        const ChainHeader& nextBlockRequestHeader = m_blockTree.getHeader(nextBlockRequestHeight);
        uchar_vector hash = nextBlockRequestHeader.getHashLittleEndian();
        status.clear();
        status << "Asking for block " << hash.getHex();
        LOGGER(debug) << status.str() << std::endl;
        notifyStatus(status.str());
        m_peer.getFilteredBlock(hash);
    }
    else
    {
        m_bBlocksSynched = true;
        notifyBlocksSynched();
    }
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

void NetworkSync::stopSyncBlocks()
{
    if (!m_bFetchingBlocks) return;
    boost::lock_guard<boost::mutex> lock(m_syncMutex);
    m_bFetchingBlocks = false;

    // TODO: signal to indicate sync has stopped
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
        m_bBlocksSynched = false;
        m_bFetchingBlocks = false;
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

    Coin::FilterLoadMessage filterLoad(m_bloomFilter.getNHashFuncs(), m_bloomFilter.getNTweak(), m_bloomFilter.getNFlags(), m_bloomFilter.getFilter());
    m_peer.send(filterLoad);
    LOGGER(trace) << "Sent filter to peer." << std::endl;
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

void NetworkSync::processConfirmations()
{
    LOGGER(trace) << "NetworkSync::processConfirmations()" << endl;
    while (!m_currentMerkleTxHashes.empty() && m_mempoolTxs.count(m_currentMerkleTxHashes.front()))
    {
        LOGGER(trace) << "NetworkSync::processConfirmations() - " << m_currentMerkleTxIndex << " of " << m_currentMerkleTxCount << " - Mempool transaction count: " << m_mempoolTxs.size() << endl;
        notifyTxConfirmed(m_currentMerkleBlock, m_currentMerkleTxHashes.front(), m_currentMerkleTxIndex, m_currentMerkleTxCount);
        m_mempoolTxs.erase(m_currentMerkleTxHashes.front());

        m_currentMerkleTxHashes.pop();
        m_currentMerkleTxIndex++;
    }
}
