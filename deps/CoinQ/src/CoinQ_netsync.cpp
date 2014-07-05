///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_netsync.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include "CoinQ_netsync.h"

#include "CoinQ_typedefs.h"

#include <stdint.h>

#include <logger/logger.h>

using namespace CoinQ::Network;

NetworkSync::NetworkSync(const CoinQ::CoinParams& coinParams) :
    m_coinParams(coinParams),
    m_bStarted(false),
    m_work(m_ioService),
    m_peer(m_ioService),
    m_bConnected(false),
    m_bFetchingHeaders(false),
    m_bHeadersSynched(false),
    m_bFetchingBlocks(false),
    m_bBlocksSynched(false)
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
        boost::lock_guard<boost::mutex> lock(m_connectionMutex);
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
            notifyError(e.what());
        }
    });

    m_peer.subscribeTimeout([&](CoinQ::Peer& /*peer*/)
    {
        boost::lock_guard<boost::mutex> lock(m_connectionMutex);
        m_bConnected = false;
        notifyTimeout();
        notifyStopped();
    });

    m_peer.subscribeClose([&](CoinQ::Peer& /*peer*/, int code, const std::string& message)
    {
        boost::lock_guard<boost::mutex> lock(m_connectionMutex);
        m_bConnected = false;
        notifyClose();
        notifyStopped();
        std::stringstream ss;
        ss << "Peer closed with code " << code << ": " << message; // TODO: localization
        notifyStatus(ss.str());
    });

    m_peer.subscribeInv([&](CoinQ::Peer& peer, const Coin::Inventory& inv)
    {
        LOGGER(trace) << "Received inventory message:" << std::endl << inv.toIndentedString() << std::endl;
        if (!m_bConnected) return;

        Coin::GetDataMessage getData(inv);
        getData.toFilteredBlocks();

        boost::lock_guard<boost::mutex> lock(m_connectionMutex);
        if (!m_bConnected) return;
        m_peer.send(getData);
    });

    m_peer.subscribeTx([&](CoinQ::Peer& /*peer*/, const Coin::Transaction& tx)
    {
        LOGGER(trace) << "Received transaction: " << tx.getHashLittleEndian().getHex() << std::endl;
        notifyTx(tx);
    });

    m_peer.subscribeHeaders([&](CoinQ::Peer& peer, const Coin::HeadersMessage& headersMessage)
    {
        LOGGER(trace) << "Received headers message..." << std::endl;

        try
        {
            if (headersMessage.headers.size() > 0)
            {
                boost::lock_guard<boost::mutex> syncLock(m_syncMutex);
                m_bFetchingHeaders = true;
                notifyFetchingHeaders();
                for (auto& item: headersMessage.headers)
                {
                    try
                    {
                        if (m_blockTree.insertHeader(item))
                        {
                            m_bHeadersSynched = false;
                        }
                    }
                    catch (const std::exception& e)
                    {
                        std::stringstream err;
                        err << "Block tree insertion error for block " << item.getHashLittleEndian().getHex() << ": " << e.what(); // TODO: localization
                        LOGGER(error) << err.str() << std::endl;
                        notifyError(err.str());
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

                if (!m_bConnected) return;
                boost::lock_guard<boost::mutex> connectionLock(m_connectionMutex);
                if (!m_bConnected) return;
                peer.getHeaders(m_blockTree.getLocatorHashes(1));
            }
            else
            {  
                notifyStatus("Flushing block chain to file...");

                {
                    boost::lock_guard<boost::mutex> syncLock(m_syncMutex);
                    m_blockTree.flushToFile(m_blockTreeFile);
                    m_bHeadersSynched = true;
                }

                notifyStatus("Done flushing block chain to file");
                notifyHeadersSynched();
            }
        }
        catch (const std::exception& e)
        {
            LOGGER(error) << "block tree exception: " << e.what() << std::endl;
            m_bFetchingHeaders = false;
        }
    });

    m_peer.subscribeBlock([&](CoinQ::Peer& /*peer*/, const Coin::CoinBlock& block)
    {
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
        LOGGER(trace) << "Received merkle block:" << std::endl << merkleBlock.toIndentedString() << std::endl;
        uchar_vector hash = merkleBlock.blockHeader.getHashLittleEndian();

        boost::lock_guard<boost::mutex> lock(m_syncMutex);
        try {
            if (m_blockTree.hasHeader(hash)) {
                // Do nothing but skip over last else.
            }
            else if (m_blockTree.insertHeader(merkleBlock.blockHeader)) {
                notifyStatus("Flushing block chain to file...");
                m_blockTree.flushToFile(m_blockTreeFile);
                m_bHeadersSynched = true;
                notifyHeadersSynched();
                notifyStatus("Done flushing block chain to file");
            }
            else {
                LOGGER(debug) << "NetworkSync merkle block handler - block rejected - hash: " << hash.getHex() << std::endl;
                return;
            }

            ChainHeader header = m_blockTree.getHeader(hash);
            notifyMerkleBlock(ChainMerkleBlock(merkleBlock, true, header.height, header.chainWork));

            uint32_t bestHeight = m_blockTree.getBestHeight();
            if (m_bFetchingBlocks)
            {
                if (bestHeight > (uint32_t)header.height) // We still need to fetch more blocks
                {
                    const ChainHeader& nextHeader = m_blockTree.getHeader(header.height + 1);
                    uchar_vector hash = nextHeader.getHashLittleEndian();
                    std::string hashString = hash.getHex();

                    std::stringstream status;
                    status << "Asking for block " << hashString << " / height: " << nextHeader.height;
                    LOGGER(debug) << status.str() << std::endl;
                    notifyStatus(status.str());

                    if (m_bConnected)
                    {
                        boost::lock_guard<boost::mutex> lock(m_connectionMutex);
                        if (m_bConnected)
                        {
                            m_lastRequestedBlockHeight = nextHeader.height;
                            m_peer.getFilteredBlock(hash);
                        }
                    }

                }

                if (bestHeight == m_lastRequestedBlockHeight)
                {
                    m_bFetchingBlocks = false;
                    m_bBlocksSynched = true;
                    notifyBlocksSynched();
                }
            }
        }
        catch (const std::exception& e) {
            std::stringstream err;
            err << "NetworkSync merkle block handler - block hash: " << hash.getHex() << " - " << e.what();
            LOGGER(error) << err.str() << std::endl;
            notifyError(err.str());

            try {
                LOGGER(debug) << "NetworkSync merkle block handler - possible reorg - attempting to fetch block headers..." << std::endl;

                if (m_bConnected)
                {
                    boost::lock_guard<boost::mutex> lock(m_connectionMutex);
                    if (m_bConnected) { m_peer.getHeaders(m_blockTree.getLocatorHashes(-1)); }
                }
            }
            catch (const std::exception& e) {
                err.clear();
                err << "NetworkSync merkle block handler - error fetching block headers: " << e.what();
                LOGGER(error) << err.str() << std::endl;
                notifyError(e.what());
            } 
        }
    });
}

NetworkSync::~NetworkSync()
{
    if (m_bStarted) stop();
}

void NetworkSync::loadHeaders(const std::string& blockTreeFile, bool bCheckProofOfWork)
{
    m_blockTreeFile = blockTreeFile;

    try {
        std::stringstream status;
        {
            boost::lock_guard<boost::mutex> syncLock(m_syncMutex);
            m_blockTree.loadFromFile(blockTreeFile, bCheckProofOfWork);
            m_bHeadersSynched = true;
            status << "Best Height: " << m_blockTree.getBestHeight() << " / " << "Total Work: " << m_blockTree.getTotalWork().getDec();
        }
        notifyHeadersSynched();
        notifyStatus(status.str());
        return;
    }
    catch (const std::exception& e) {
        notifyStatus(e.what());
    }

    {
        boost::lock_guard<boost::mutex> syncLock(m_syncMutex);
        m_blockTree.clear();

        m_blockTree.setGenesisBlock(m_coinParams.genesis_block());
        m_bHeadersSynched = true;
    }
    notifyHeadersSynched();
    notifyStatus("Block tree file not found. A new one will be created.");
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

/*
    boost::lock_guard<boost::mutex> lock(m_connectionMmutex);
    if (!m_bConnected) throw std::runtime_error("NetworkSync::syncBlocks() - must connect before synching.");
*/

    
    stopSyncBlocks();
    boost::lock_guard<boost::mutex> syncLock(m_syncMutex);
    m_bFetchingBlocks = true;

 //   initBlockFilter();

    ChainHeader header;
    bool foundHeader = false;
    for (auto& hash: locatorHashes) {
        try {
            header = m_blockTree.getHeader(hash);
            if (header.inBestChain) {
                foundHeader = true;
                break;
            }
            else {
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

    if (bestHeader.height >= nextBlockRequestHeight)
    {
        std::stringstream status;
        status << "Resynching blocks " << nextBlockRequestHeight << " - " << bestHeader.height;
        LOGGER(debug) << status.str() << std::endl;
        notifyStatus(status.str());
        const ChainHeader& nextBlockRequestHeader = m_blockTree.getHeader(nextBlockRequestHeight);
        uchar_vector hash = nextBlockRequestHeader.getHashLittleEndian();
        status.str("");
        status << "Asking for block " << hash.getHex();
        LOGGER(debug) << status.str() << std::endl;
        notifyStatus(status.str());
        if (m_bConnected)
        {
            boost::lock_guard<boost::mutex> lock(m_connectionMutex);
            if (m_bConnected)
            { 
                m_lastRequestedBlockHeight = (uint32_t)nextBlockRequestHeight;
                m_peer.getFilteredBlock(hash);
            }
        }
    }
    else
    {
        m_bFetchingBlocks = false;
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
    if (m_bStarted) throw std::runtime_error("NetworkSync::start() - already started.");
    boost::lock_guard<boost::mutex> lock(m_startMutex);
    if (m_bStarted) throw std::runtime_error("NetworkSync::start() - already started.");

    std::string port_ = port.empty() ? m_coinParams.default_port() : port;
    m_bStarted = true;
    m_bFetchingHeaders = false;
    m_bFetchingBlocks = false;
    m_peer.set(host, port_, m_coinParams.magic_bytes(), m_coinParams.protocol_version(), "Wallet v0.1", 0, false);
    m_ioServiceThread = boost::thread(boost::bind(&CoinQ::io_service_t::run, &m_ioService));
    m_peer.start();
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
    if (!m_bStarted) return;
    boost::lock_guard<boost::mutex> lock(m_startMutex);
    if (!m_bStarted) return;

    m_bStarted = false;
    m_bFetchingHeaders = false;
    m_bFetchingBlocks = false;

    m_peer.stop();
    m_ioServiceThread.join();
    m_bConnected = false;
    notifyStopped();
}

void NetworkSync::sendTx(Coin::Transaction& tx)
{
    if (!m_bConnected) throw std::runtime_error("NetworkSync::sendTx() - Must be connected to send transactions.");
    boost::lock_guard<boost::mutex> lock(m_connectionMutex);
    if (!m_bConnected) throw std::runtime_error("NetworkSync::sendTx() - Must be connected to send transactions.");

    m_peer.send(tx); 
}

void NetworkSync::getTx(uchar_vector& hash)
{
    if (!m_bConnected) throw std::runtime_error("NetworkSync::getTx() - Must be connected to get transactions.");
    boost::lock_guard<boost::mutex> lock(m_connectionMutex);
    if (!m_bConnected) throw std::runtime_error("NetworkSync::getTx() - Must be connected to get transactions.");

    m_peer.getTx(hash);
}

void NetworkSync::getMempool()
{
    if (!m_bConnected) throw std::runtime_error("NetworkSync::getTx() - Must be connected to get mempool.");
    boost::lock_guard<boost::mutex> lock(m_connectionMutex);
    if (!m_bConnected) throw std::runtime_error("NetworkSync::getTx() - Must be connected to get mempool.");

    m_peer.getMempool();
}

void NetworkSync::setBloomFilter(const Coin::BloomFilter& bloomFilter)
{
    m_bloomFilter = bloomFilter;
    if (!m_bloomFilter.isSet()) return;

    if (!m_bConnected) return;
    boost::lock_guard<boost::mutex> lock(m_connectionMutex);
    if (!m_bConnected) return;

    Coin::FilterLoadMessage filterLoad(m_bloomFilter.getNHashFuncs(), m_bloomFilter.getNTweak(), m_bloomFilter.getNFlags(), m_bloomFilter.getFilter());
    m_peer.send(filterLoad);
    LOGGER(trace) << "Sent filter to peer." << std::endl;
}
