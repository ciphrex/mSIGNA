///////////////////////////////////////////////////////////////////////////////
//
// netsync example program 
//
// main.cpp
//
// Copyright (c) 2012-2014 Eric Lombrozo
//
// All Rights Reserved.

#include <CoinCore/CoinNodeData.h>
#include <CoinCore/MerkleTree.h>
#include <CoinCore/typedefs.h>

#include <CoinQ/CoinQ_netsync.h>
#include <CoinQ/CoinQ_coinparams.h>

#include <stdutils/stringutils.h>

#include <logger/logger.h>

#include <signal.h>

bool g_bShutdown = false;

void finish(int sig)
{
    std::cout << "Stopping..." << std::endl;
    g_bShutdown = true;
}

using namespace CoinQ;
using namespace Coin;
using namespace std;

int main(int argc, char* argv[])
{
    try
    {
        NetworkSelector networkSelector;

        if (argc < 3)
        {
            cerr << "# Usage: " << argv[0] << " <network> <host> [port]" << endl
                 << "# Supported networks: " << stdutils::delimited_list(networkSelector.getNetworkNames(), ", ") << endl;
            return -1;
        }

        CoinParams coinParams = networkSelector.getCoinParams(argv[1]);
        string host = argv[2];
        string port = (argc > 3) ? argv[3] : coinParams.default_port();

        cout << endl << "Connecting to " << coinParams.network_name() << " peer" << endl
             << "-------------------------------------------" << endl
             << "  host:             " << host << endl
             << "  port:             " << port << endl
             << "  magic bytes:      " << hex << coinParams.magic_bytes() << endl
             << "  protocol version: " << dec << coinParams.protocol_version() << endl
             << endl;

        INIT_LOGGER("netsync.log");
        LOGGER(info) << endl << endl << endl << endl << endl << endl;
    
        signal(SIGINT, &finish);
        signal(SIGTERM, &finish);

        Network::NetworkSync networkSync(coinParams);
        networkSync.loadHeaders("blocktree.dat", false, [&](const CoinQBlockTreeMem& blocktree) {
            cout << "Best height: " << blocktree.getBestHeight() << " Total work: " << blocktree.getTotalWork().getDec() << endl;
        });

        networkSync.subscribeStarted([&]() {
            cout << "NetworkSync started." << endl;
        });

        networkSync.subscribeStopped([&]() {
            cout << "NetworkSync stopped." << endl;
        });

        networkSync.subscribeOpen([&]() {
            cout << "NetworkSync open." << endl;
        });

        networkSync.subscribeClose([&]() {
            cout << "NetworkSync closed." << endl;
        });

        networkSync.subscribeTimeout([&]() {
            cout << "NetworkSync timeout." << endl;
        });

        networkSync.subscribeConnectionError([&](const string& error) {
            cout << "NetworkSync connection error: " << error << endl;
        });

        networkSync.subscribeProtocolError([&](const string& error) {
            cout << "NetworkSync protocol error: " << error << endl;
        });

        networkSync.subscribeBlockTreeError([&](const string& error) {
            cout << "NetworkSync block tree error: " << error << endl;
        });

        networkSync.subscribeFetchingHeaders([&]() {
            cout << "NetworkSync fetching headers." << endl;
        });

        networkSync.subscribeHeadersSynched([&]() {
            cout << "NetworkSync headers synched." << endl;
            hashvector_t hashes;
            networkSync.syncBlocks(hashes, time(NULL) - 10*60*60); // Start 10 hours earlier
        });

        networkSync.subscribeFetchingBlocks([&]() {
            cout << "NetworkSync fetching blocks." << endl;
        });

        networkSync.subscribeBlocksSynched([&]() {
            cout << "NetworkSync blocks synched." << endl;
        });

        networkSync.subscribeStatus([&](const string& status) {
            cout << "NetworkSync status: " << status << endl;
        });

        networkSync.subscribeTx([&](const Transaction& tx) {
            cout << "NetworkSync transaction:" << tx.getHashLittleEndian().getHex() << endl;
        });

        networkSync.subscribeBlock([&](const ChainBlock& block) {
            cout << "NetworkSync block: " << block.blockHeader.getHashLittleEndian().getHex() << " height: " << block.height << endl;
        });

        networkSync.subscribeMerkleBlock([&](const ChainMerkleBlock& merkleblock) {
            cout << "NetworkSync merkle block: " << merkleblock.blockHeader.getHashLittleEndian().getHex() << " height: " << merkleblock.height << endl;
        });

        networkSync.subscribeAddBestChain([&](const ChainHeader& header) {
            cout << "NetworkSync added to best chain: " << header.getHashLittleEndian().getHex() << " height: " << header.height << endl;
        });

        networkSync.subscribeRemoveBestChain([&](const ChainHeader& header) {
            cout << "NetworkSync removed from best chain: " << header.getHashLittleEndian().getHex() << " height: " << header.height << endl;
        });

        networkSync.subscribeBlockTreeChanged([&]() {
            cout << "NetworkSync block tree changed." << endl;
        });

        LOGGER(info) << "Starting..." << endl;
        networkSync.start(host, port);
        while (!g_bShutdown) { usleep(200); }

        LOGGER(info) << "Stopping..." << endl;
        networkSync.stop();
    }
    catch (const exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return -2;
    }

    return 0;
}
