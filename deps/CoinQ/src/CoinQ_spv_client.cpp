///////////////////////////////////////////////////////////////////////////////
//
// networksync.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include "networksync.h"

#include <CoinQ_peerasync.h>
#include <typedefs.h>

#include <stdint.h>

#include <logger.h>

// Select the network

#define USE_BITCOIN_NETWORK
//#define USE_LITECOIN_NETWORK

namespace listener_network
{
#if defined(USE_BITCOIN_NETWORK)
    const uint32_t MAGIC_BYTES = 0xd9b4bef9ul;
    const uint32_t PROTOCOL_VERSION = 70001;
    const uint32_t DEFAULT_PORT = 8333;
    const uint8_t  PAY_TO_PUBKEY_HASH_VERSION = 0x00;
    const uint8_t  PAY_TO_SCRIPT_HASH_VERSION = 0x05;
    const char*    NETWORK_NAME = "Bitcoin";
    const Coin::CoinBlockHeader GENESIS_BLOCK(1, 1231006505, 486604799, 2083236893, bytes_t(32, 0), uchar_vector("4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));
#elif defined(USE_LITECOIN_NETWORK)
    const uint32_t MAGIC_BYTES = 0xdbb6c0fbul;
    const uint32_t PROTOCOL_VERSION = 60002;
    const uint32_t DEFAULT_PORT = 9333;
    const uint8_t  PAY_TO_PUBKEY_HASH_VERSION = 0x30;
    const uint8_t  PAY_TO_SCRIPT_HASH_VERSION = 0x05;
    const char*    NETWORK_NAME = "Litecoin";
#endif
}


NetworkSync::NetworkSync()
    : work(io_service), io_service_thread(NULL), peer(NULL), blockFilter(&blockTree), resynching(false)
{
    io_service_thread = new boost::thread(boost::bind(&CoinQ::io_service_t::run, &io_service));
}

NetworkSync::~NetworkSync()
{
    stop();
    io_service.stop();
    io_service_thread->join();
    delete io_service_thread;
}

bool NetworkSync::isConnected() const
{
    return (peer != NULL);
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

    blockTree.setGenesisBlock(listener_network::GENESIS_BLOCK);
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
        if (resynching && blockTree.getBestHeight() > block.height && peer) {
            const ChainHeader& nextHeader = blockTree.getHeader(block.height + 1);
            uchar_vector blockHash = nextHeader.getHashLittleEndian();
            emit status(tr("Asking for block ") + QString::fromStdString(blockHash.getHex()) + tr(" / height: ") + QString::number(nextHeader.height));
            peer->getBlock(blockHash);
        }
        else {
            resynching = false;
            emit doneResync();
            peer->getMempool();
        }
    });
}

int NetworkSync::getBestHeight()
{
    return blockTree.getBestHeight();
}

void NetworkSync::resync(const std::vector<bytes_t>& locatorHashes, uint32_t startTime)
{
    if (!peer) throw std::runtime_error("Must connect before resynching.");

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
        peer->getBlock(blockHash);
    }
    else {
        resynching = false;
        emit doneResync();
        peer->getMempool();
    }
}

void NetworkSync::resync(int resyncHeight)
{
    if (!peer) throw std::runtime_error("Must connect before resynching."); 

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
        peer->getBlock(blockHash);
    }

    peer->getMempool();
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
    if (peer) {
        stop();
    }

    try {
        // TODO: use async api to resolve host
        CoinQ::tcp::resolver resolver(io_service);
        CoinQ::tcp::resolver::query query(host.toStdString(), QString::number(port).toStdString());
        CoinQ::tcp::resolver::iterator end;
        CoinQ::tcp::resolver::iterator iter = resolver.resolve(query);
        if (iter == end) {
            emit error(tr("Could not resolve host ") + host + ":" + QString::number(port));
            return;
        } 
        peer = new CoinQ::Peer(io_service, *iter, listener_network::MAGIC_BYTES, listener_network::PROTOCOL_VERSION);
    }
    catch (const std::exception& e) {
        emit error(QString::fromStdString(e.what()));
        return;
    }
    catch (const boost::system::error_code& ec) {
        emit error(QString::fromStdString(ec.message()));
        return;
    }
    catch (...) {
        emit error(tr("Unknown error connecting to host ") + host + ":" + QString::number(port));
        return;
    }

    peer->subscribeOpen([&](CoinQ::Peer& peer) {
        emit open();
        try {
//            emit status(tr("Fetching block headers..."));
            peer.getHeaders(blockTree.getLocatorHashes(-1));
        }
        catch (const std::exception& e) {
            emit error(QString::fromStdString(e.what()));
        }
    });

    peer->subscribeTimeout([&](CoinQ::Peer& /*peer*/) {
        emit timeout();
    });

    peer->subscribeClose([&](CoinQ::Peer& /*peer*/, int code, const std::string& message) {
        emit close();
        emit status(tr("Peer closed with code ") + QString::number(code) + ": " + QString::fromStdString(message));
    });

    peer->subscribeInv([&](CoinQ::Peer& peer, const Coin::Inventory& inv) {
        Coin::GetDataMessage getData(inv);
        Coin::CoinNodeMessage msg(peer.getMagic(), &getData);
        peer.send(msg);
    });

    peer->subscribeTx([&](CoinQ::Peer& /*peer*/, const Coin::Transaction& tx) {
        notifyTx(tx);
        //std::cout << "Received transaction: " << tx.getHashLittleEndian().getHex() << std::endl;
    });

    peer->subscribeHeaders([&](CoinQ::Peer& peer, const Coin::HeadersMessage& headers) {
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
            }
        }
        catch (const std::exception& e) {
            std::cout << std::endl << "block tree exception: " << e.what() << std::endl;
        }
    });

    peer->subscribeBlock([&](CoinQ::Peer& /*peer*/, const Coin::CoinBlock& block) {
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
                LOGGER(debug) << "NetworkSync block handler - block rejected - hash: " << hash.getHex() << std::endl;
            }
        }
        catch (const std::exception& e) {
            LOGGER(error) << "NetworkSync block handler - block hash: " << hash.getHex() << " - " << e.what() << std::endl;
            emit status(tr("NetworkSync block handler error."));
        }
    });

    peer->start();
    emit started();
}

void NetworkSync::stop()
{
    if (peer) {
        peer->stop();
        delete peer;
        peer = NULL;
        emit stopped();
    }
}

void NetworkSync::sendTx(Coin::Transaction& tx) const
{
    if (!peer) {
        throw std::runtime_error("Must be connected to send transactions.");
    }

    Coin::CoinNodeMessage msg(peer->getMagic(), &tx);
    peer->send(msg); 
}
