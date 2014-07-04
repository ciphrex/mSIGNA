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

NetworkSync::NetworkSync(const CoinQ::CoinParams& coin_params)
    : coin_params_(coin_params), work(io_service), io_service_thread(NULL), peer(io_service), blockFilter(&blockTree), resynching(false), isConnected_(false)
{
    Coin::CoinBlockHeader::setHashFunc(coin_params_.block_header_hash_function());
    Coin::CoinBlockHeader::setPOWHashFunc(coin_params_.block_header_pow_hash_function());

    io_service_thread = new boost::thread(boost::bind(&CoinQ::io_service_t::run, &io_service));

    peer.subscribeOpen([&](CoinQ::Peer& peer) {
        isConnected_ = true;
        notifyOpen();
        try {
            if (bloomFilter.isSet()) {
                Coin::FilterLoadMessage filterLoad(bloomFilter.getNHashFuncs(), bloomFilter.getNTweak(), bloomFilter.getNFlags(), bloomFilter.getFilter());
                peer.send(filterLoad);
                LOGGER(trace) << "Sent filter to peer." << std::endl;
            }

//            notifyStatus("Fetching block headers...");
            peer.getHeaders(blockTree.getLocatorHashes(-1));
        }
        catch (const std::exception& e) {
            notifyError(e.what());
        }
    });

    peer.subscribeTimeout([&](CoinQ::Peer& /*peer*/) {
        isConnected_ = false;
        notifyTimeout();
        notifyStopped();
    });

    peer.subscribeClose([&](CoinQ::Peer& /*peer*/, int code, const std::string& message) {
        isConnected_ = false;
        notifyClose();
        notifyStopped();
        std::stringstream ss;
        ss << "Peer closed with code " << code << ": " << message; // TODO: localization
        notifyStatus(ss.str());
    });

    peer.subscribeInv([&](CoinQ::Peer& peer, const Coin::Inventory& inv) {
        LOGGER(trace) << "Received inventory message:" << std::endl << inv.toIndentedString() << std::endl;
        Coin::GetDataMessage getData(inv);
        getData.toFilteredBlocks();
        peer.send(getData);
    });

    peer.subscribeTx([&](CoinQ::Peer& /*peer*/, const Coin::Transaction& tx) {
        LOGGER(trace) << "Received transaction: " << tx.getHashLittleEndian().getHex() << std::endl;
        notifyTx(tx);
    });

    peer.subscribeHeaders([&](CoinQ::Peer& peer, const Coin::HeadersMessage& headers) {
        LOGGER(trace) << "Received headers message..." << std::endl;
        try {
            if (headers.headers.size() > 0) {
                for (auto& item: headers.headers) {
                    try {
                        if (blockTree.insertHeader(item)) {
                            blockTreeFlushed = false;
                        }
                    }
                    catch (const std::exception& e) {
                        std::stringstream err;
                        err << "Block tree insertion error for block " << item.getHashLittleEndian().getHex() << ": " << e.what(); // TODO: localization
                        LOGGER(error) << err.str() << std::endl;
                        notifyError(err.str());
                        throw e;
                    }
                }

                LOGGER(trace) << "Processed " << headers.headers.size() << " headers."
                     << " mBestHeight: " << blockTree.getBestHeight()
                     << " mTotalWork: " << blockTree.getTotalWork().getDec()
                     << " Attempting to fetch more headers..." << std::endl;
                notifyBlockTreeChanged();
                std::stringstream status;
                status << "Best Height: " << blockTree.getBestHeight() << " / " << "Total Work: " << blockTree.getTotalWork().getDec();
                notifyStatus(status.str());
                peer.getHeaders(blockTree.getLocatorHashes(1));
            }
            else {
                notifyStatus("Flushing block chain to file...");
                blockTree.flushToFile(blockTreeFile);
                blockTreeFlushed = true;
                notifyStatus("Done flushing block chain to file");
                notifyDoneSync();
                //peer.getMempool();
            }
        }
        catch (const std::exception& e) {
            LOGGER(error) << "block tree exception: " << e.what() << std::endl;
        }
    });

    peer.subscribeBlock([&](CoinQ::Peer& /*peer*/, const Coin::CoinBlock& block) {
        uchar_vector hash = block.blockHeader.getHashLittleEndian();
        try {
            if (blockTree.hasHeader(hash)) {
                notifyBlock(block);
            }
            else if (blockTree.insertHeader(block.blockHeader)) {
                notifyStatus("Flushing block chain to file...");
                blockTree.flushToFile(blockTreeFile);
                blockTreeFlushed = true;
                notifyStatus("Done flushing block chain to file");
                notifyBlock(block);
                notifyDoneSync();
            }
            else {
                LOGGER(debug) << "NetworkSync block handler - block rejected - hash: " << hash.getHex() << std::endl;
            }
        }
        catch (const std::exception& e) {
            LOGGER(error) << "NetworkSync block handler - block hash: " << hash.getHex() << " - " << e.what() << std::endl;
            notifyStatus("NetworkSync block handler error.");
        }
    });

    peer.subscribeMerkleBlock([&](CoinQ::Peer& /*peer*/, const Coin::MerkleBlock& merkleBlock) {
        LOGGER(trace) << "Received merkle block:" << std::endl << merkleBlock.toIndentedString() << std::endl;
        uchar_vector hash = merkleBlock.blockHeader.getHashLittleEndian();
        try {
            if (blockTree.hasHeader(hash)) {
                // Do nothing but skip over last else.
            }
            else if (blockTree.insertHeader(merkleBlock.blockHeader)) {
                notifyStatus("Flushing block chain to file...");
                blockTree.flushToFile(blockTreeFile);
                blockTreeFlushed = true;
                notifyStatus("Done flushing block chain to file");
            }
            else {
                LOGGER(debug) << "NetworkSync merkle block handler - block rejected - hash: " << hash.getHex() << std::endl;
                return;
            }

            ChainHeader header = blockTree.getHeader(hash);
            notifyMerkleBlock(ChainMerkleBlock(merkleBlock, true, header.height, header.chainWork));

            uint32_t bestHeight = blockTree.getBestHeight();
            if (resynching)
            {
                if (bestHeight > (uint32_t)header.height) // We still need to fetch more blocks
                {
                    const ChainHeader& nextHeader = blockTree.getHeader(header.height + 1);
                    uchar_vector blockHash = nextHeader.getHashLittleEndian();
                    LOGGER(debug) << "Asking for block " << blockHash.getHex() << " / height: " << nextHeader.height << std::endl;
                    std::stringstream status;
                    status << "Asking for block " << blockHash.getHex() << " / height: " << nextHeader.height;
                    notifyStatus(status.str());
                    lastResyncHeight = nextHeader.height;
                    peer.getFilteredBlock(blockHash);
                }

                if (bestHeight == lastResyncHeight)
                {
                    resynching = false;
                    notifyDoneResync();
                    //peer.getMempool();
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
                peer.getHeaders(blockTree.getLocatorHashes(-1));
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
    stop();
    io_service.stop();
    io_service_thread->join();
    delete io_service_thread;
}

void NetworkSync::initBlockTree(const std::string& blockTreeFile, bool bCheckProofOfWork)
{
    this->blockTreeFile = blockTreeFile;
    try {
        blockTree.loadFromFile(blockTreeFile, bCheckProofOfWork);
        blockTreeFlushed = true;
        std::stringstream status;
        status << "Best Height: " << blockTree.getBestHeight() << " / " << "Total Work: " << blockTree.getTotalWork().getDec();
        notifyStatus(status.str());
        return;
    }
    catch (const std::exception& e) {
        notifyStatus(e.what());
    }

    blockTree.clear();

    blockTree.setGenesisBlock(coin_params_.genesis_block());
    blockTreeFlushed = false;
    notifyStatus("Block tree file not found. A new one will be created.");
}

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
        if (/*resynching &&*/ blockTree.getBestHeight() > block.height) {
            const ChainHeader& nextHeader = blockTree.getHeader(block.height + 1);
            uchar_vector blockHash = nextHeader.getHashLittleEndian();
            std::stringstream status;
            status << "Asking for block " << blockHash.getHex() << " Height: " << nextHeader.height;
            notifyStatus(status.str());
            peer.getBlock(blockHash);
        }
        else {
            resynching = false;
            notifyDoneResync();
            //peer.getMempool();
        }
    });
}

int NetworkSync::getBestHeight()
{
    return blockTree.getBestHeight();
}

void NetworkSync::resync(const std::vector<bytes_t>& locatorHashes, uint32_t startTime)
{
    if (!isConnected_) throw std::runtime_error("Must connect before resynching.");

    stopResync();

    boost::lock_guard<boost::mutex> lock(mutex);

    resynching = true;

    initBlockFilter();

    ChainHeader header;
    bool foundHeader = false;
    for (auto& hash: locatorHashes) {
        try {
            header = blockTree.getHeader(hash);
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

    const ChainHeader& bestHeader = blockTree.getHeader(-1);

    int resyncHeight;
    if (foundHeader) {
        resyncHeight = header.height + 1;
    }
    else {
        ChainHeader header = blockTree.getHeaderBefore(startTime);
        resyncHeight = header.height;
    }

    if (bestHeader.height >= resyncHeight) {
        std::stringstream status;
        status << "Resynching blocks " << resyncHeight << " - " << bestHeader.height;
        LOGGER(debug) << status.str() << std::endl;
        notifyStatus(status.str());
        const ChainHeader& resyncHeader = blockTree.getHeader(resyncHeight);
        uchar_vector blockHash = resyncHeader.getHashLittleEndian();
        status.str("");
        status << "Asking for block " << blockHash.getHex();
        LOGGER(debug) << status.str() << std::endl;
        notifyStatus(status.str());
        lastResyncHeight = (uint32_t)resyncHeight;
        peer.getFilteredBlock(blockHash);
    }
    else {
        resynching = false;
        notifyDoneResync();
        //peer.getMempool();
    }
}

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

void NetworkSync::stopResync()
{
    if (!resynching) return;
    boost::lock_guard<boost::mutex> lock(mutex);
    resynching = false; 
}

void NetworkSync::start(const std::string& host, const std::string& port)
{
    // TODO: proper mutexes

    if (isConnected_) {
        stop();
    }

    peer.set(host, port, coin_params_.magic_bytes(), coin_params_.protocol_version(), "Wallet v0.1", 0, false);

    peer.start();
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
    resynching = false;
    peer.stop();
    notifyStopped();
}

void NetworkSync::sendTx(Coin::Transaction& tx)
{
    if (!isConnected_) {
        throw std::runtime_error("Must be connected to send transactions.");
    }

    peer.send(tx); 
}

void NetworkSync::getTx(uchar_vector& hash)
{
    if (!isConnected_) {
        throw std::runtime_error("Must be connected to get transactions.");
    }

    peer.getTx(hash);
}

void NetworkSync::getMempool()
{
    if (!isConnected_) {
        throw std::runtime_error("Must be connected to get mempool.");
    }

    peer.getMempool();
}

void NetworkSync::setBloomFilter(const Coin::BloomFilter& bloomFilter_)
{
    bloomFilter = bloomFilter_;
 
    if (isConnected_ && bloomFilter.isSet()) {
        Coin::FilterLoadMessage filterLoad(bloomFilter.getNHashFuncs(), bloomFilter.getNTweak(), bloomFilter.getNFlags(), bloomFilter.getFilter());
        peer.send(filterLoad);
        LOGGER(trace) << "Sent filter to peer." << std::endl;
    }
}
