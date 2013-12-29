////////////////////////////////////////////////////////////////////////////////
//
// CoinQ.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#define SQLITE3CPP_LOG

#include "CoinQ_peerasync.h"
#include "CoinQ_filter.h"
#include "CoinQ_blocks.h"
#include "CoinQ_txs.h"
#include "CoinQ_keys.h"
#include "CoinQ_sqlite3.h"
#include "CoinQ_websocket.h"

#include <BloomFilter.h>

#include <iostream>
#include <string>

#include <signal.h>

#include <boost/program_options.hpp>
#include <boost/thread.hpp>



// Select the network

#define USE_BITCOIN_NETWORK
//#define USE_LITECOIN_NETWORK

namespace listener_network
{
#if defined(USE_BITCOIN_NETWORK)
    const uint32_t MAGIC_BYTES = 0xd9b4bef9ul;
    const uint32_t PROTOCOL_VERSION = 70001;
    const uint32_t DEFAULT_PORT = 8333;
    const uint8_t  ADDRESS_VERSION = 0x00;
    const uint8_t  MULTISIG_ADDRESS_VERSION = 0x05;
    const char*    NETWORK_NAME = "Bitcoin";
#elif defined(USE_LITECOIN_NETWORK)
    const uint32_t MAGIC_BYTES = 0xdbb6c0fbul;
    const uint32_t PROTOCOL_VERSION = 60002;
    const uint32_t DEFAULT_PORT = 9333;
    const uint8_t  ADDRESS_VERSION = 0x30;
    const uint8_t  MULTISIG_ADDRESS_VERSION = 0x05;
    const char*    NETWORK_NAME = "Litecoin";
#endif
};



namespace po = boost::program_options;

using namespace Coin;
using namespace std;

bool g_bShutdown = false;

void finish(int sig)
{
    std::cout << "Stopping..." << std::endl;
    g_bShutdown = true;
}   

bool initBlockTree(CoinQBlockTreeMem& blockTree, const CoinBlockHeader& genesisBlock, const std::string& filename = "")
{
    if (!filename.empty()) {
        try {
            std::cout << "Loading block tree from file..." << std::flush;
            blockTree.loadFromFile(filename);
            std::cout << "Done. "
                      << " mBestHeight: " << blockTree.getBestHeight()
                      << " mTotalWork: " << blockTree.getTotalWork().getDec() << std::endl;
            return true;
        }
        catch (const std::exception& e) {
            std::cout << std::endl;
            std::cout << "Block tree file loading error: " << e.what() << std::endl;
        }
    }

    std::cout << "Constructing new block tree." << std::endl;
    blockTree.clear();
    blockTree.setGenesisBlock(genesisBlock);
    return false;
}

void onDoneSync(CoinQ::Peer& peer, ICoinQBlockTree& blockTree, ICoinQTxStore& txStore, CoinQ::WebSocket::Server& wsServer, bool bResync, int resyncHeight)
{
    const ChainHeader& bestHeader = blockTree.getHeader(-1); // gets the best chain's head.
    txStore.setBestHeight(bestHeader.height);
    wsServer.setBestHeader(bestHeader);
    wsServer.pushHeader(bestHeader);

    blockTree.subscribeAddBestChain([&](const ChainHeader& header) {
        uchar_vector hash = header.getHashLittleEndian();
        cout << "Added to best chain:     " << hash.getHex()
             << " Height: " << header.height
             << " ChainWork: " << header.chainWork.getDec() << endl;

        txStore.setBestHeight(header.height);
        wsServer.setBestHeader(header);
        wsServer.pushHeader(header);
    });

    if (bResync) {
        if (bestHeader.height >= resyncHeight) {
            cout << "Resynching blocks " << resyncHeight << " - " << bestHeader.height << endl;
            const ChainHeader& resyncHeader = blockTree.getHeader(resyncHeight);
            uchar_vector blockHash = resyncHeader.getHashLittleEndian();
            cout << "Asking for block " << blockHash.getHex() << endl;
            peer.getBlock(blockHash);
        }
    }

    peer.getMempool();
}

int main(int argc, char** argv)
{
    signal(SIGINT, &finish);
 
    // Program Options
    std::string configFile;
    std::string peerHost;
    int peerPort;
    int wsPort;
    std::string wsAllowedIps;
    std::string blockTreeFile;
    std::string txFile;
    std::string addressesFile;
    int resyncHeightInput;
    int resyncHeight = -1;

    std::string wsAllowedIpsDesc("regular expression which socket client IP addresses must match (default: ");
    wsAllowedIpsDesc += CoinQ::WebSocket::DEFAULT_ALLOWED_IPS + ")";

    po::options_description options("Options");
    options.add_options()
        ("help", "produce help message")
        ("configfile", po::value<std::string>( &configFile ), "name of configuration file from which to read these options")
        ("peerhost", po::value<std::string>( &peerHost ), "p2p host to which to connect (default: localhost)")
        ("peerport", po::value<int>( &peerPort ), "p2p port on which to connect (default: 8333)")
        ("wsport", po::value<int>( &wsPort ), "port on which to accept websocket connections (default: 9002)")
        ("wsallowedips", po::value<std::string>( &wsAllowedIps ), wsAllowedIpsDesc.c_str())
        ("blocktree", po::value<std::string>( &blockTreeFile ), "name of file in which block tree is stored (required)")
        ("txdb", po::value<std::string>( &txFile ), "name of file to use for transaction database (required)")
        ("addresses", po::value<std::string>( &addressesFile ), "name of file containing addresses to watch (required)")
        ("resyncheight", po::value<int>( &resyncHeightInput ), "height from which to download blocks (use negative number to subtract from best height at last shutdown, omit to prevent resync)")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, options), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << options << std::endl;
        return 1;
    }

    if (vm.count("configfile")) {
        std::ifstream config(configFile);
        po::store(po::parse_config_file(config, options), vm);
        config.close();
        po::notify(vm);
    }

    if (!vm.count("blocktree") || !vm.count("txdb") || !vm.count("addresses")) {
        std::cout << "Missing required options." << std::endl << options << std::endl;
        return -1;
    }

    if (!vm.count("peerhost")) peerHost = "localhost";
    if (!vm.count("peerport")) peerPort = 8333;
    if (!vm.count("wsport")) wsPort = 9002;
    if (!vm.count("wsallowedips")) wsAllowedIps = CoinQ::WebSocket::DEFAULT_ALLOWED_IPS;
    bool bResync = vm.count("resyncheight");

    std::cout << std::endl;
    std::cout << "Using the following options:" << std::endl
              << "peerhost: " << peerHost << std::endl
              << "peerport: " << peerPort << std::endl
              << "wsport: " << wsPort << std::endl
              << "wsallowedips: " << wsAllowedIps << std::endl
              << "blocktreefile: " << blockTreeFile << std::endl
              << "txfile: " << txFile << std::endl
              << "addressfile: " << addressesFile << std::endl
              << "resyncheight: ";

    if (bResync) {
        std::cout << resyncHeightInput;
    }
    else {
        std::cout << "none";
    }
    std::cout << std::endl;

    std::cout << std::endl;

    const CoinBlockHeader kGenesisBlock(1, 1231006505, 486604799, 2083236893, g_zero32bytes, uchar_vector("4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));
//    const CoinBlockHeader kGenesisBlock(2, 1375162707, 436242792, 3601811532, uchar_vector("000000000000001617794c92f14979ee194f070bb550f4a1beb179c37c6d107d"), uchar_vector("3ea9fc33d82a0ab796930f7bcdbe8aeb656d1f18d1fc35278eb5d705c0074204"));

    CoinQBlockTreeMem blockTree;
    bool bFlushed = initBlockTree(blockTree, kGenesisBlock, blockTreeFile);

    CoinQTxStoreSqlite3 txStore;
    if (!txFile.empty()) {
        try {
            std::cout << "Opening transaction file..." << std::flush;
            txStore.open(txFile);
            std::cout << "Done." << std::endl;
        }
        catch (const std::exception& e) {
            std::cout << std::endl;
            std::cout << "Exception opening " << txFile << ": " << e.what() << std::endl;
        }
    }
    txStore.subscribeInsert([](const ChainTransaction& tx) {
        std::cout << "Transaction inserted: " << tx.getHashLittleEndian().getHex() << std::endl;
    });
    txStore.subscribeDelete([](const ChainTransaction& tx) {
        std::cout << "Transaction deleted: " << tx.getHashLittleEndian().getHex() << std::endl;
    });
    txStore.subscribeConfirm([](const ChainTransaction& tx) {
        std::cout << "Transaction confirmed: " << tx.getHashLittleEndian().getHex() << std::endl;
    });
    txStore.subscribeUnconfirm([](const ChainTransaction& tx) {
        std::cout << "Transaction unconfirmed: " << tx.getHashLittleEndian().getHex() << std::endl;
    });

    CoinQ::Keys::AddressSet filterAddresses;
    if (!addressesFile.empty()) {
        std::cout << "Loading filter addresses..." << std::flush;
        try {
            filterAddresses.loadFromFile(addressesFile);
            std::cout << "Done." << std::endl;
        }
        catch (const std::exception& e) {
            std::cout << e.what() << std::endl;
        }
    }

    // Start WebSocket server
    CoinQ::WebSocket::Server wsServer(wsPort, wsAllowedIps);
    try {
        wsServer.setBestHeader(blockTree.getHeader(-1));
        std::cout << "Starting websockets server..." << std::flush;
        wsServer.start();
        std::cout << "Done." << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << std::endl;
        std::cerr << "Error starting websockets server: " << e.what() << std::endl;
        return 2;
    }

    CoinQTxAddressFilter txFilter(&filterAddresses, CoinQTxFilter::Mode::BOTH);
    txFilter.connect([&](const ChainTransaction& tx) {
        if (txStore.isOpen()) {
            std::cout << "Inserting transaction into store: " << tx.getHashLittleEndian().getHex() << std::endl;
            txStore.insert(tx);
        }
        wsServer.pushTx(tx);
    });

    SetAddressVersion(listener_network::ADDRESS_VERSION);
    SetMultiSigAddressVersion(listener_network::MULTISIG_ADDRESS_VERSION);

    CoinQ::io_service_t io_service;
    CoinQ::io_service_t::work work(io_service);
    boost::thread io_service_thread(boost::bind(&CoinQ::io_service_t::run, &io_service));

    CoinQ::tcp::resolver resolver(io_service);
    std::stringstream ssPort;
    ssPort << peerPort;
    CoinQ::tcp::resolver::query query(peerHost, ssPort.str());
    auto iter = resolver.resolve(query);
    CoinQ::Peer peer(io_service, *iter, listener_network::MAGIC_BYTES, listener_network::PROTOCOL_VERSION);

    peer.subscribeOpen([&](CoinQ::Peer& peer) {
        cout << "Connection opened to " << peer.getName() << "." << endl;
        try {
            cout << "Fetching block headers..." << endl;
            peer.getHeaders(blockTree.getLocatorHashes(-1));
        }
        catch (const std::exception& e) {
            std::cout << "peer.getHeaders() error: " << e.what() << std::endl;
        }
    });

    peer.subscribeTimeout([&](CoinQ::Peer& peer) {
        std::cout << "Peer " << peer.getName() << " timed out." << std::endl;
    });

    peer.subscribeClose([&](CoinQ::Peer& peer, int code, const std::string& message) {
        std:: cout << "Peer " << peer.getName() << " closed with code " << code << ": " << message << "." << std::endl;
    });

    peer.subscribeInv([&](CoinQ::Peer& peer, const Coin::Inventory& inv) {
        Coin::GetDataMessage getData(inv);
        Coin::CoinNodeMessage msg(peer.getMagic(), &getData);
        peer.send(msg);
    });

    peer.subscribeTx([&](CoinQ::Peer& peer, const Transaction& tx) { txFilter.push(tx); });

    peer.subscribeHeaders([&](CoinQ::Peer& peer, const HeadersMessage& headers) {
        cout << "Received headers message..." << endl;
        try {
            if (headers.headers.size() > 0) {
                for (auto& item: headers.headers) {
                    try {
                        if (blockTree.insertHeader(item)) {
                            bFlushed = false;
                        }
                    }
                    catch (const std::exception& e) {
                        cout << "block tree insertion error for block " << item.getHashLittleEndian().getHex() << ": "  << e.what() << endl;
                        throw e;
                    }
                }

                cout << "Processed " << headers.headers.size() << " headers."
                     << " mBestHeight: " << blockTree.getBestHeight()
                     << " mTotalWork: " << blockTree.getTotalWork().getDec()
                     << " Attempting to fetch more headers..." << endl;
                peer.getHeaders(blockTree.getLocatorHashes(1));
            }
            else {
                cout << "No more headers." << endl;
                if (!blockTreeFile.empty() && !bFlushed) {
                    cout << "Flushing to file..." << flush;
                    blockTree.flushToFile(blockTreeFile);
                    bFlushed = true;
                    cout << "Done." << endl;
                }
                onDoneSync(peer, blockTree, txStore, wsServer, bResync, resyncHeight);
                // Add a bloom filter
/*
                Coin::BloomFilter bloomFilter(10, 0.01, 0x14325264, 0);
                bloomFilter.insert(uchar_vector("06f1b66fb6c0e253f24c74d3ed972ff447ca285c"));
                bloomFilter.insert(uchar_vector("47db54281bbb73e1b631c96548c8a77bf5957bff"));
                Coin::FilterLoadMessage filterLoad;
                filterLoad.filter = bloomFilter.getFilter();
                filterLoad.nHashFuncs = bloomFilter.getNHashFuncs();
                filterLoad.nTweak = bloomFilter.getNTweak();
                filterLoad.nFlags = bloomFilter.getNFlags();
                Coin::CoinNodeMessage msg(peer.getMagic(), &filterLoad);
                peer.send(msg);
*/
            }
        }
        catch (const std::exception& e) {
            std::cout << std::endl << "block tree exception: " << e.what() << std::endl;
        }
    });

    peer.subscribeMerkleBlock([&](CoinQ::Peer& peer, const MerkleBlock& merkleBlock) {
        cout << "Received merkleblock:" << endl << merkleBlock.toIndentedString();
    });

    CoinQBestChainBlockFilter blockFilter(&blockTree);
    blockFilter.connect([&](const ChainBlock& block) {
        uchar_vector blockHash = block.blockHeader.getHashLittleEndian();
        cout << "Got block: " << blockHash.getHex() << " height: " << block.height << " chainWork: " << block.chainWork.getDec() << endl;
        for (unsigned int i = 0; i < block.txs.size(); i++) {
            ChainTransaction tx(block.txs[i], block.getHeader(), i);
            txFilter.push(tx);
        }

        wsServer.pushBlock(block);
        if (bResync && (block.height >= resyncHeight) && (blockTree.getBestHeight() > block.height)) {
            const ChainHeader& nextHeader = blockTree.getHeader(block.height + 1);
            uchar_vector blockHash = nextHeader.getHashLittleEndian();
            cout << "Asking for block " << blockHash.getHex() << endl;
            peer.getBlock(blockHash);
            //peer.getFilteredBlock(blockHash);
        }
    });

    blockTree.subscribeRemoveBestChain([&](const ChainHeader& header) {
        uchar_vector hash = header.getHashLittleEndian();
        cout << "Removed from best chain: " << hash.getHex()
             << " Height: " << header.height
             << " ChainWork: " << header.chainWork.getDec() << endl;
        if (txStore.isOpen()) {
            txStore.unconfirm(hash);
        }
    });
/*
    blockTree.subscribeReorg([&](const ChainHeader& header) {
        resyncHeight = header.height;
        uchar_vector hash = header.getHashLittleEndian();
        cout << "Reorg from block " << hash.getHex() << " at height " << resyncHeight << "." << endl;
        peer.getBlock(hash);
    });
*/
    peer.subscribeBlock([&](CoinQ::Peer& peer, const CoinBlock& block) {
        uchar_vector hash = block.blockHeader.getHashLittleEndian();
        if (!blockTree.hasHeader(hash)) {
            try {
                blockTree.insertHeader(block.blockHeader);
                if (!blockTreeFile.empty()) {
                    cout << "Flushing to file..." << flush;
                    blockTree.flushToFile(blockTreeFile);
                    cout << "Done." << endl;
                }
            }
            catch (const std::exception& e) {
                std::cout << std::endl << "block tree exception: " << e.what() << std::endl;
            }
        }
        blockFilter.push(block);
    });

    while (!g_bShutdown) {
        try
        {
            // TODO: Fix the following logic.
            if (bResync) resyncHeight = (resyncHeightInput >= 0) ? resyncHeightInput : blockTree.getBestHeight() + resyncHeightInput;
            blockTree.clearAddBestChain();
            cout << "Connecting to peer " << peer.getName() << "." << endl;
            peer.start();
        }
        catch (const exception& e)
        {
            std::cout << std::endl;
            std::cout << "Error: " << e.what() << std::endl;
            std::cout << "Retrying connection in 5 seconds..." << std::endl;
            boost::this_thread::sleep(boost::posix_time::seconds(5));
            continue;
        }

        while (!g_bShutdown && peer.isRunning()) {
            usleep(200);
        }

        if (!g_bShutdown) {
            std::cout << "Peer disconnected. Retrying connection in 5 seconds..." << std::endl;
            boost::this_thread::sleep(boost::posix_time::seconds(5));
        }
    }

    std::cout << "Stopping wsServer." << std::endl;
    wsServer.stop();

    std::cout << "Stopping peer." << std::endl;
    peer.stop();

    io_service.stop();
    io_service_thread.join();

    while (peer.isRunning()) { usleep(200); }
    return 0;
}

