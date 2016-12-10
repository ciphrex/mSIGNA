///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// networksync.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#ifdef __WIN32__
#include <winsock2.h>
#endif

#include <CoinQ_peer_io.h>
#include <CoinQ_blocks.h>
#include <CoinQ_filter.h>

#include <CoinQ_signals.h>
#include <CoinQ_slots.h>

#include <QObject>
#include <QString>

#include <boost/thread.hpp>

#include <CoinQ_typedefs.h>

#include <BloomFilter.h>

typedef Coin::Transaction coin_tx_t;
typedef ChainHeader chain_header_t;
typedef ChainBlock chain_block_t;
typedef ChainMerkleBlock chain_merkle_block_t;

class NetworkSync : public QObject
{
    Q_OBJECT

public:
    NetworkSync();
    ~NetworkSync();

    bool isConnected() const { return isConnected_; }

    void initBlockTree(const QString& blockTreeFile);
    int getBestHeight();

    void start(const QString& host, int port);
    void stop();

    void setBloomFilter(const Coin::BloomFilter& bloomFilter_);

    void resync(const std::vector<bytes_t>& locatorHashes, uint32_t startTime);
    void resync(int resyncHeight);
    void stopResync();

    // MESSAGES TO PEER
    void sendTx(Coin::Transaction& tx);

    // EVENT SUBSCRIPTIONS
    void subscribeTx(tx_slot_t slot) { notifyTx.connect(slot); }
    void subscribeBlock(chain_block_slot_t slot) { notifyBlock.connect(slot); }
    void subscribeMerkleBlock(chain_merkle_block_slot_t slot) { notifyMerkleBlock.connect(slot); }
    void subscribeAddBestChain(chain_header_slot_t slot) { notifyAddBestChain.connect(slot); }
    void subscribeRemoveBestChain(chain_header_slot_t slot) { notifyRemoveBestChain.connect(slot); }

signals:
    void started();
    void stopped();

    void open();
    void timeout();
    void close();

    void status(const QString& message);
    void error(const QString& message);

    void doneSync();
    void doneResync();

    void newTx(const coin_tx_t& tx);
    void newBlock(const chain_block_t& block);
    void addBestChain(const chain_header_t& header);
    void removeBestChain(const chain_header_t& header);

private:
    CoinQ::io_service_t io_service;
    CoinQ::io_service_t::work work;
    boost::thread* io_service_thread;
    CoinQ::Peer peer;

    QString host;
    int port;
    int resyncHeight;

    QString blockTreeFile;
    CoinQBlockTreeMem blockTree;
    bool blockTreeFlushed;

    CoinQBestChainBlockFilter blockFilter;

    boost::mutex mutex;
    bool resynching;

    bool isConnected_;

    Coin::BloomFilter bloomFilter;

    void initBlockFilter();

    CoinQSignal<const Coin::Transaction&> notifyTx;
    CoinQSignal<const ChainBlock&> notifyBlock;
    CoinQSignal<const ChainMerkleBlock&> notifyMerkleBlock;
    CoinQSignal<const ChainHeader&> notifyAddBestChain;
    CoinQSignal<const ChainHeader&> notifyRemoveBestChain;
};

