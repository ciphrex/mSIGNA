////////////////////////////////////////////////////////////////////////////////
//
// txsqlite.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#define SQLITE3CPP_LOG

#include "CoinQ_peer.h"
#include "CoinQ_filter.h"
#include "CoinQ_blocks.h"
#include "CoinQ_txs.h"
#include "CoinQ_keys.h"
#include "CoinQ_sqlite3.h"

#include <iostream>
#include <string>

#include <boost/program_options.hpp>

namespace po = boost::program_options;

using namespace Coin;

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

void onDoneSync(CoinQPeer& peer, ICoinQBlockTree& blockTree, ICoinQTxStore& txStore, int resyncHeight)
{
    txStore.setBestHeight(blockTree.getBestHeight());

    blockTree.subscribeAddBestChain([&](const ChainHeader& header) {
        uchar_vector hash = header.getHashLittleEndian();
        cout << "Added to best chain:     " << hash.getHex()
             << " Height: " << header.height
             << " ChainWork: " << header.chainWork.getDec() << endl;

        txStore.setBestHeight(header.height);
    });

    if (resyncHeight > -1) {
        int maxHeight = blockTree.getBestHeight();
        if (maxHeight >= resyncHeight) {
            cout << "Resynching blocks " << resyncHeight << " - " << maxHeight << endl;
            const ChainHeader& header = blockTree.getHeader(resyncHeight);
            std::string blockHashStr = header.getHashLittleEndian().getHex();
            cout << "Asking for block " << blockHashStr << endl;
            peer.askForBlock(blockHashStr);
        }
    }
}

int main(int argc, char** argv)
{
    // Program Options
    std::string configFile;
    std::string host;
    int port;
    std::string blockTreeFile;
    std::string txFile;
    std::string addressesFile;
    int resyncHeight;

    po::options_description options("Options");
    options.add_options()
        ("help", "produce help message")
        ("configfile", po::value<std::string>( &configFile ), "name of configuration file from which to read these options")
        ("host", po::value<std::string>( &host ), "p2p host to which to connect (default: localhost)")
        ("port", po::value<int>( &port ), "port on which to connect (default: 8333)")
        ("blocktreefile", po::value<std::string>( &blockTreeFile ), "name of file in which block tree is stored (required)")
        ("txfile", po::value<std::string>( &txFile ), "name of file in which to store transactions (required)")
        ("addressfile", po::value<std::string>( &addressesFile ), "name of file containing addresses to watch (required)")
        ("resyncheight", po::value<int>( &resyncHeight ), "height from which to download blocks (use -1 to prevent resync, default: -2, last confirmed height - 10)")
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

    if (!vm.count("blocktreefile") || !vm.count("txfile") || !vm.count("addressfile")) {
        std::cout << "Missing required options." << std::endl << options << std::endl;
        return -1;
    }

    if (!vm.count("host")) host = "localhost";
    if (!vm.count("port")) port = 8333;
    if (!vm.count("resyncheight")) resyncHeight = -2;

    std::cout << "Using the following options:" << std::endl
              << "host: " << host << std::endl
              << "port: " << port << std::endl
              << "blocktreefile: " << blockTreeFile << std::endl
              << "txfile: " << txFile << std::endl
              << "addressfile: " << addressesFile << std::endl
              << "resyncheight: " << resyncHeight << std::endl << std::endl;

    const CoinBlockHeader kGenesisBlock(1, 1231006505, 486604799, 2083236893, g_zero32bytes, uchar_vector("4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));
//    const CoinBlockHeader kGenesisBlock(2, 1375162707, 436242792, 3601811532, uchar_vector("000000000000001617794c92f14979ee194f070bb550f4a1beb179c37c6d107d"), uchar_vector("3ea9fc33d82a0ab796930f7bcdbe8aeb656d1f18d1fc35278eb5d705c0074204"));

    CoinQBlockTreeMem blockTree;
    bool bFlushed = initBlockTree(blockTree, kGenesisBlock, blockTreeFile);

    if (resyncHeight == -2) resyncHeight = blockTree.getBestHeight() - 10;

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

    CoinQTxAddressFilter txFilter(&filterAddresses, CoinQTxFilter::Mode::BOTH);
    txFilter.connect([&](const ChainTransaction& tx) {
        if (txStore.isOpen()) {
            std::cout << "Inserting transaction into store: " << tx.getHashLittleEndian().getHex() << std::endl;
            txStore.insert(tx);
        }
    });

    SetAddressVersion(listener_network::ADDRESS_VERSION);
    SetMultiSigAddressVersion(listener_network::MULTISIG_ADDRESS_VERSION);

    CoinQPeer peer(host, port);

    peer.subscribeTx([&](const Transaction& tx) { txFilter.push(tx); });

    peer.subscribeHeaders([&](const HeadersMessage& headers) {
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
                onDoneSync(peer, blockTree, txStore, resyncHeight);
            }
        }
        catch (const std::exception& e) {
            std::cout << std::endl << "block tree exception: " << e.what() << std::endl;
        }
    });

    CoinQBestChainBlockFilter blockFilter(&blockTree);
    blockFilter.connect([&](const ChainBlock& block) {
        uchar_vector blockHash = block.blockHeader.getHashLittleEndian();
        cout << "Got block: " << blockHash.getHex() << " height: " << block.height << " chainWork: " << block.chainWork.getDec() << endl;
        for (unsigned int i = 0; i < block.txs.size(); i++) {
            txFilter.push(ChainTransaction(block.txs[i], block.getHeader(), i));
        }
        if ((resyncHeight > -1) && (block.height >= resyncHeight) && (blockTree.getBestHeight() > block.height)) {
            const ChainHeader& nextHeader = blockTree.getHeader(block.height + 1);
            std::string blockHashStr = nextHeader.getHashLittleEndian().getHex();
            cout << "Asking for block " << blockHashStr << endl;
            peer.askForBlock(blockHashStr);
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

    blockTree.subscribeReorg([&](const ChainHeader& header) {
        resyncHeight = header.height;
        std::string hash = header.getHashLittleEndian().getHex();
        cout << "Reorg from block " << hash << " at height " << resyncHeight << "." << endl;
        peer.askForBlock(hash);
    });

    peer.subscribeBlock([&](const CoinBlock& block) {
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

    try
    {
        cout << "Connecting to peer..." << flush;
        peer.start();
        cout << "Done." << endl;
        cout << "Fetching block headers..." << endl;
        peer.getHeaders(blockTree.getLocatorHashes(-1));
    }
    catch (const exception& e)
    {
        cerr << "Error: " << e.what() << endl << endl;
        return -1;
    }

    while (peer.isRunning()) { sleep(1); }
    return 0;

}
