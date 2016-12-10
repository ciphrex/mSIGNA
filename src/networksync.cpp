///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// networksync.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "networksync.h"

#include <typedefs.h>

#include <stdint.h>

#include "severitylogger.h"

// Select the network

#define USE_BITCOIN
//#define USE_LITECOIN
//#define USE_QUARKCOIN
#include <CoinQ_coinparams.h>

NetworkSync::NetworkSync()
    : work(io_service), io_service_thread(NULL), peer(io_service), blockFilter(&blockTree), resynching(false), isConnected_(false)
{
    Coin::CoinBlockHeader::setHashFunc(COIN_PARAMS::BLOCK_HEADER_HASH_FUNCTION);

    io_service_thread = new boost::thread(boost::bind(&CoinQ::io_service_t::run, &io_service));

    peer.subscribeOpen([&](CoinQ::Peer& peer) {
        isConnected_ = true;
        emit open();
        try {
            if (bloomFilter.isSet()) {
                Coin::FilterLoadMessage filterLoad(bloomFilter.getNHashFuncs(), bloomFilter.getNTweak(), bloomFilter.getNFlags(), bloomFilter.getFilter());
                peer.send(filterLoad);
        std::cout << "Sent filter to peer." << std::endl;
            }

//            emit status(tr("Fetching block headers..."));
            peer.getHeaders(blockTree.getLocatorHashes(-1));
        }
        catch (const std::exception& e) {
            emit error(QString::fromStdString(e.what()));
        }
    });

    peer.subscribeTimeout([&](CoinQ::Peer& /*peer*/) {
        isConnected_ = false;
        emit timeout();
    });

    peer.subscribeClose([&](CoinQ::Peer& /*peer*/, int code, const std::string& message) {
        isConnected_ = false;
        emit close();
        emit status(tr("Peer closed with code ") + QString::number(code) + ": " + QString::fromStdString(message));
    });

    peer.subscribeInv([&](CoinQ::Peer& peer, const Coin::Inventory& inv) {
        std::cout << "Received inventory message:" << std::endl << inv.toIndentedString() << std::endl;
        Coin::GetDataMessage getData(inv);
        getData.toFilteredBlocks();
        peer.send(getData);
    });

    peer.subscribeTx([&](CoinQ::Peer& /*peer*/, const Coin::Transaction& tx) {
        notifyTx(tx);
        //std::cout << "Received transaction: " << tx.getHashLittleEndian().getHex() << std::endl;
    });

    peer.subscribeHeaders([&](CoinQ::Peer& peer, const Coin::HeadersMessage& headers) {
        std::cout << "Received headers message..." << std::endl;
        try {
            if (headers.headers.size() > 0) {
                for (auto& item: headers.headers) {
                    try {
                        if (blockTree.insertHeader(item)) {
                            blockTreeFlushed = false;
                        }
                    }
                    catch (const std::exception& e) {
                        std::cout << "block tree insertion error for block " << item.getHashLittleEndian().getHex() << ": "  << e.what() << std::endl;
                        emit error(tr("Block tree insertion error for block ") + QString::fromStdString(item.getHashLittleEndian().getHex()) + ": " + QString::fromStdString(e.what()));
                        throw e;
                    }
                }

                std::cout << "Processed " << headers.headers.size() << " headers."
                     << " mBestHeight: " << blockTree.getBestHeight()
                     << " mTotalWork: " << blockTree.getTotalWork().getDec()
                     << " Attempting to fetch more headers..." << std::endl;
                emit status(tr("Best Height: ") + QString::number(blockTree.getBestHeight()) + " / " + tr("Total Work: ") + QString::fromStdString(blockTree.getTotalWork().getDec()));
                peer.getHeaders(blockTree.getLocatorHashes(1));
            }
            else {
                emit status(tr("Flushing block chain to file..."));
                blockTree.flushToFile(blockTreeFile.toStdString());
                blockTreeFlushed = true;
                emit status(tr("Done flushing block chain to file"));
                emit doneSync();
                peer.getMempool();
            }
        }
        catch (const std::exception& e) {
            std::cout << std::endl << "block tree exception: " << e.what() << std::endl;
        }
    });

    peer.subscribeBlock([&](CoinQ::Peer& /*peer*/, const Coin::CoinBlock& block) {
        uchar_vector hash = block.blockHeader.getHashLittleEndian();
        try {
            if (blockTree.hasHeader(hash)) {
                notifyBlock(block);
            }
            else if (blockTree.insertHeader(block.blockHeader)) {
                emit status(tr("Flushing block chain to file..."));
                blockTree.flushToFile(blockTreeFile.toStdString());
                blockTreeFlushed = true;
                emit status(tr("Done flushing block chain to file"));
                notifyBlock(block);
                emit doneSync();
            }
            else {
#ifdef USE_LOGGING
                LOGGER(debug) << "NetworkSync block handler - block rejected - hash: " << hash.getHex();
#endif
            }
        }
        catch (const std::exception& e) {
#ifdef USE_LOGGING
            LOGGER(error) << "NetworkSync block handler - block hash: "
                << hash.getHex() << " - " << e.what();
#endif
            emit status(tr("NetworkSync block handler error."));
        }
    });

    peer.subscribeMerkleBlock([&](CoinQ::Peer& /*peer*/, const Coin::MerkleBlock& merkleBlock) {
        std::cout << "Received merkle block:" << std::endl << merkleBlock.toIndentedString() << std::endl;
        uchar_vector hash = merkleBlock.blockHeader.getHashLittleEndian();
        try {
            if (blockTree.hasHeader(hash)) {
                // Do nothing but skip over last else.
            }
            else if (blockTree.insertHeader(merkleBlock.blockHeader)) {
                emit status(tr("Flushing block chain to file..."));
                blockTree.flushToFile(blockTreeFile.toStdString());
                blockTreeFlushed = true;
                emit status(tr("Done flushing block chain to file"));
            }
            else {
#ifdef USE_LOGGING
                LOGGER(debug) << "NetworkSync merkle block handler - block rejected - hash: " << hash.getHex();
#endif
                return;
            }

            ChainHeader header = blockTree.getHeader(hash);
            notifyMerkleBlock(ChainMerkleBlock(merkleBlock, true, header.height, header.chainWork));

            if (resynching && blockTree.getBestHeight() > header.height) {
                const ChainHeader& nextHeader = blockTree.getHeader(header.height + 1);
                uchar_vector blockHash = nextHeader.getHashLittleEndian();
#ifdef USE_LOGGING
                LOGGER(debug) << "Asking for block " << blockHash.getHex() << " / height: " << nextHeader.height;
#endif
                emit status(tr("Asking for block ") + QString::fromStdString(blockHash.getHex()) + tr(" / height: ") + QString::number(nextHeader.height));
                peer.getFilteredBlock(blockHash);
            }
            else {
                resynching = false;
                emit doneResync();
                peer.getMempool();
            }
        }
        catch (const std::exception& e) {
#ifdef USE_LOGGING
            LOGGER(error) << "NetworkSync merkle block handler - block hash: "
                << hash.getHex() << " - " << e.what();
#endif
            emit status(tr("NetworkSync merkle block handler error."));
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

void NetworkSync::initBlockTree(const QString& blockTreeFile)
{
    this->blockTreeFile = blockTreeFile;
    try {
        blockTree.loadFromFile(blockTreeFile.toStdString());
        blockTreeFlushed = true;
        emit status(tr("Best Height: ") + QString::number(blockTree.getBestHeight()) + " / " + tr("Total Work: ") + QString::fromStdString(blockTree.getTotalWork().getDec()));
        return;
    }
    catch (const std::exception& e) {
        emit status(QString::fromStdString(e.what()));
    }

    blockTree.clear();

    blockTree.setGenesisBlock(COIN_PARAMS::GENESIS_BLOCK);
    blockTreeFlushed = false;
    emit status(tr("Block tree file not found. A new one will be created."));
}

void NetworkSync::initBlockFilter()
{
    blockTree.unsubscribeAll();

    blockTree.subscribeRemoveBestChain([&](const ChainHeader& header) {
        emit removeBestChain(header);
    });

    blockTree.subscribeAddBestChain([&](const ChainHeader& header) {
        emit addBestChain(header);
        uchar_vector hash = header.getHashLittleEndian();
        emit status(tr("Added to best chain: ") + QString::fromStdString(hash.getHex()) +
             tr(" Height: ") + QString::number(header.height) +
             tr(" ChainWork: ") + QString::fromStdString(header.chainWork.getDec()));
    });

    blockFilter.clear();
    blockFilter.connect([&](const ChainBlock& block) {
        uchar_vector blockHash = block.blockHeader.getHashLittleEndian();
        emit status(tr("Got block: ") + QString::fromStdString(blockHash.getHex()) +
            tr(" height: ") + QString::number(block.height) +
            tr(" chainWork: ") + QString::fromStdString(block.chainWork.getDec()) +
            tr(" # txs: ") + QString::number(block.txs.size()));
        for (auto& tx: block.txs) {
            notifyTx(tx);
        }

        notifyBlock(block);
        if (resynching && blockTree.getBestHeight() > block.height) {
            const ChainHeader& nextHeader = blockTree.getHeader(block.height + 1);
            uchar_vector blockHash = nextHeader.getHashLittleEndian();
            emit status(tr("Asking for block ") + QString::fromStdString(blockHash.getHex()) + tr(" / height: ") + QString::number(nextHeader.height));
            peer.getBlock(blockHash);
        }
        else {
            resynching = false;
            emit doneResync();
            peer.getMempool();
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
            foundHeader = true;
            break;
        }
        catch (const std::exception& e) {
            emit status(e.what());
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
        emit status(tr("Resynching blocks ") + QString::number(resyncHeight) + " - " + QString::number(bestHeader.height));
        const ChainHeader& resyncHeader = blockTree.getHeader(resyncHeight);
        uchar_vector blockHash = resyncHeader.getHashLittleEndian();
        emit status(tr("Asking for block ") + QString::fromStdString(blockHash.getHex()));
        peer.getFilteredBlock(blockHash);
    }
    else {
        resynching = false;
        emit doneResync();
        peer.getMempool();
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
        emit status(tr("Resynching blocks ") + QString::number(resyncHeight) + " - " + QString::number(bestHeader.height));
        const ChainHeader& resyncHeader = blockTree.getHeader(resyncHeight);
        uchar_vector blockHash = resyncHeader.getHashLittleEndian();
        emit status(tr("Asking for block ") + QString::fromStdString(blockHash.getHex()));
        peer.getFilteredBlock(blockHash);
    }

    peer.getMempool();
}

void NetworkSync::stopResync()
{
    if (!resynching) return;
    boost::lock_guard<boost::mutex> lock(mutex);
    resynching = false; 
}

void NetworkSync::start(const QString& host, int port)
{
    // TODO: proper mutexes

    if (isConnected_) {
        stop();
    }

    peer.set(host.toStdString(), QString::number(port).toStdString(), COIN_PARAMS::MAGIC_BYTES, COIN_PARAMS::PROTOCOL_VERSION, "Wallet v0.1", 0, false);
    peer.start();
    emit started();
}

void NetworkSync::stop()
{
    peer.stop();
    emit stopped();
}

void NetworkSync::sendTx(Coin::Transaction& tx)
{
    if (!isConnected_) {
        throw std::runtime_error("Must be connected to send transactions.");
    }

    peer.send(tx); 
}

void NetworkSync::setBloomFilter(const Coin::BloomFilter& bloomFilter_)
{
    bloomFilter = bloomFilter_;
 
    if (isConnected_ && bloomFilter.isSet()) {
        Coin::FilterLoadMessage filterLoad(bloomFilter.getNHashFuncs(), bloomFilter.getNTweak(), bloomFilter.getNFlags(), bloomFilter.getFilter());
        peer.send(filterLoad);
std::cout << "Sent filter to peer." << std::endl;
    }
}
