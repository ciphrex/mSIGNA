///////////////////////////////////////////////////////////////////////////////
//
// blockchain download example program 
//
// main.cpp
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include <BlockchainDownload.h>

#include <logger/logger.h>

#include <stdutils/stringutils.h>

#include <signal.h>

bool g_bShutdown = false;

void finish(int sig)
{
    std::cout << "Stopping..." << std::endl;
    g_bShutdown = true;
}

using namespace CoinQ;
using namespace std;

int main(int argc, char* argv[])
{
    try
    {
        NetworkSelector networkSelector;

        if (argc < 3)
        {
            cerr << "# Usage: " << argv[0] << " <network> <host> [start hash] [start height] [port]" << endl
                 << "# Supported networks: " << stdutils::delimited_list(networkSelector.getNetworkNames(), ", ") << endl;
            return -1;
        }

        CoinParams coinParams = networkSelector.getCoinParams(argv[1]);
        string host = argv[2];

        vector<uchar_vector> locatorHashes;
        if (argc > 3) { locatorHashes.push_back(uchar_vector(argv[3])); }

        int startHeight = (argc > 4) ? strtoll(argv[4], NULL, 0) : 0;

        string port = (argc > 5) ? argv[5] : coinParams.default_port();

        Network::BlockchainDownload download(coinParams);

        cout << endl << "Connecting to " << coinParams.network_name() << " peer" << endl
             << "-------------------------------------------" << endl
             << "  host:             " << host << endl
             << "  port:             " << port << endl
             << "  magic bytes:      " << hex << coinParams.magic_bytes() << endl
             << "  protocol version: " << dec << coinParams.protocol_version() << endl
             << endl;

        download.subscribeStarted([&]()         { cout << "Blockchain download started." << endl; });
        download.subscribeStopped([&]()         { cout << "Blockchain download stopped." << endl; });
        download.subscribeOpen([&]()            { cout << "Peer connection opened." << endl; });
        download.subscribeClose([&]()           { cout << "Peer connection closed." << endl; });
        download.subscribeTimeout([&]()         { cout << "Peer timed out." << endl; });
        download.subscribeConnectionError([&](const string& error, int /*code*/) { cout << "Connection error: " << error << endl; });

        download.subscribeBlock([&](const Coin::CoinBlock& block)    { cout << "Block - hash: " << block.hash().getHex() << " height: " << download.getBestHeight() + startHeight + 1 << endl; });
        download.subscribeProtocolError([&](const string& error, int /*code*/)   { cout << "Protocol error:" << error << endl; });

        INIT_LOGGER("blockchain.log");
    
        signal(SIGINT, &finish);
        signal(SIGTERM, &finish);

        download.start(host, port, locatorHashes);

        while (!g_bShutdown) { usleep(200); }
        download.stop();
    }
    catch (const exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return -2;
    }

    return 0;
}
