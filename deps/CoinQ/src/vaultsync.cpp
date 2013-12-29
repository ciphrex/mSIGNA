///////////////////////////////////////////////////////////////////////////////
//
// vaultsync.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

//
// Build with:
//      g++ -o obj/vaultsync.o -DDATABASE_SQLITE -c vault.cpp -std=c++0x -I../deps/CoinClasses/src
//      g++ -o ../build/vaultsync obj/vault.o CoinQ_vault_db-odb.o -lodb-sqlite -lodb -lssl -lcrypto
//

#include "../deps/cli/cli.hpp"
#include <CoinKey.h>
#include <Base58Check.h>

#include <memory>
#include <algorithm>
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include "CoinQ_database.hxx"

#include "CoinQ_vault.h"
#include "CoinQ_peerasync.h"
#include "CoinQ_blocks.h"
#include "CoinQ_filter.h"

#include "CoinQ_vault_db.hxx"
#include "CoinQ_vault_db-odb.hxx"

const unsigned char BASE58_VERSIONS[] = { 0x00, 0x05 };

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

using namespace std;
using namespace odb::core;
using namespace Coin;
using namespace CoinQ::Script;
using namespace CoinQ::Vault;

bool initBlockTree(CoinQBlockTreeMem& blockTree, const Coin::CoinBlockHeader& genesisBlock, const std::string& filename = "")
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

void onDoneSync(CoinQ::Peer& peer, ICoinQBlockTree& blockTree, Vault& vault, bool bResync, int resyncHeight)
{
    const ChainHeader& bestHeader = blockTree.getHeader(-1); // gets the best chain's head.
 //   txStore.setBestHeight(bestHeader.height);
//    wsServer.setBestHeader(bestHeader);
//    wsServer.pushHeader(bestHeader);
 
    blockTree.subscribeAddBestChain([&](const ChainHeader& header) {
        uchar_vector hash = header.getHashLittleEndian();
        cout << "Added to best chain:     " << hash.getHex()
             << " Height: " << header.height
             << " ChainWork: " << header.chainWork.getDec() << endl;
 
//        txStore.setBestHeight(header.height);
//        wsServer.setBestHeader(header);
//        wsServer.pushHeader(header);
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

cli::result_t cmd_sync(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() < 2 || params.size() > 5) {
        return "sync <vault filename> <blocktree file> [resync height=-1] [host=localhost] [port=8333]- synchronize vault with blockchain and mempool starting after a particular block.";
    }

    std::string blocktree_file = params[1];

    bool bResync = true;
    int resyncHeight = -1;
    if (params.size() > 2) {
        resyncHeight = strtol(params[2].c_str(), NULL, 0);
    }
    
    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};
    unique_ptr<database> db(open_database(argc, argv));

    Vault vault(argc, argv, false);

    const Coin::CoinBlockHeader kGenesisBlock(1, 1231006505, 486604799, 2083236893, bytes_t(32, 0), uchar_vector("4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));

//    const CoinBlockHeader kGenesisBlock(2, 1375162707, 436242792, 3601811532, uchar_vector("000000000000001617794c92f14979ee194f070bb550f4a1beb179c37c6d107d"), uchar_vector("3ea9fc33d82a0ab796930f7bcdbe8aeb656d1f18d1fc35278eb5d705c0074204"));
 
    CoinQBlockTreeMem blockTree;
    bool bFlushed = initBlockTree(blockTree, kGenesisBlock, params[1]);
    if (resyncHeight < 0) resyncHeight += blockTree.getBestHeight();
 
    std::string host = params.size() > 3 ? params[3] : std::string("localhost");
    std::string port = params.size() > 4 ? params[4] : std::string("8333");

    CoinQ::io_service_t io_service;
    CoinQ::io_service_t::work work(io_service);
    boost::thread io_service_thread(boost::bind(&CoinQ::io_service_t::run, &io_service));
 
    CoinQ::tcp::resolver resolver(io_service);
    CoinQ::tcp::resolver::query query(host, port);
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
        std::cout << "Peer " << peer.getName() << " closed with code " << code << ": " << message << "." << std::endl;
    });
 
    peer.subscribeInv([&](CoinQ::Peer& peer, const Coin::Inventory& inv) {
        Coin::GetDataMessage getData(inv);
        Coin::CoinNodeMessage msg(peer.getMagic(), &getData);
        peer.send(msg);
    });

    peer.subscribeTx([&](CoinQ::Peer& peer, const Coin::Transaction& coin_tx) {
        std::shared_ptr<Tx> tx(new Tx());
        tx->set(coin_tx);
        try {
            if (vault.addTx(tx)) {
                cout << "Inserted tx: " << uchar_vector(tx->hash()).getHex() << endl;
            }
        }
        catch (const std::exception& e) {
            cout << "Error: " << e.what() << endl;
        }
    });

    peer.subscribeHeaders([&](CoinQ::Peer& peer, const Coin::HeadersMessage& headers) {
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
                if (!blocktree_file.empty() && !bFlushed) {
                    cout << "Flushing to file..." << flush;
                    blockTree.flushToFile(blocktree_file);
                    bFlushed = true;
                    cout << "Done." << endl;
                }
                onDoneSync(peer, blockTree, vault, bResync, resyncHeight);
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
        for (auto& coin_tx: block.txs) {
            std::shared_ptr<Tx> tx(new Tx());
            tx->set(coin_tx);
            try {
                if (vault.addTx(tx)) {
                    cout << "Inserted tx: " << uchar_vector(tx->hash()).getHex() << endl;
                }
            }
            catch (const std::exception& e) {
                cout << "Error: " << e.what() << endl;
            }
        }

        if (vault.insertBlock(block)) {
            cout << "Inserted block: " << blockHash.getHex() << endl;
        }
/*
        for (unsigned int i = 0; i < block.txs.size(); i++) {
            ChainTransaction tx(block.txs[i], block.getHeader(), i);
            txFilter.push(tx);
        }
 
        wsServer.pushBlock(block);
*/
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
    });

    peer.subscribeBlock([&](CoinQ::Peer& peer, const Coin::CoinBlock& block) {
        uchar_vector hash = block.blockHeader.getHashLittleEndian();
        if (!blockTree.hasHeader(hash)) {
            try {
                blockTree.insertHeader(block.blockHeader);
                if (!blocktree_file.empty()) {
                    cout << "Flushing to file..." << flush;
                    blockTree.flushToFile(blocktree_file);
                    cout << "Done." << endl;
                }
            }
            catch (const std::exception& e) {
                std::cout << std::endl << "block tree exception: " << e.what() << std::endl;
            }
        }
        blockFilter.push(block);
    });
 
    peer.start();
    io_service_thread.join();

    return "Done.";
}

int main(int argc, char** argv)
{
    cli::command_map cmds("VaultSync by Eric Lombrozo v0.0.1");
    cmds.add("sync", &cmd_sync);
    try {
        return cmds.exec(argc, argv);
    }
    catch (const std::exception& e) {
        cerr << e.what() << endl;
        return 1;
    }
    return 0;
}
