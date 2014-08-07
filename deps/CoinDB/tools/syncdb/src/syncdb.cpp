///////////////////////////////////////////////////////////////////////////////
//
// syncdb.cpp
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.
//
// Utility for synchronizing a coin database with p2p network
//

#include <SynchedVault.h>

#include <CoinQ/CoinQ_coinparams.h>

#include <logger/logger.h>
#include <stdutils/stringutils.h>

#include <iostream>
#include <signal.h>

#include <thread>
#include <chrono>

using namespace CoinDB;
using namespace CoinQ;
using namespace std;

const std::string VERSION_INFO = "v0.1.0";
const std::string BLOCKTREE_FILENAME = "blocktree.dat";

bool g_bShutdown = false;

void finish(int sig)
{
    LOGGER(debug) << "Stopping..." << endl;
    g_bShutdown = true;
}

int main(int argc, char* argv[])
{
    NetworkSelector networkSelector;

    try
    {
        if (argc < 4)
        {
            cerr << "SyncDB by Eric Lombrozo " << VERSION_INFO << endl
                 << "# Usage: " << argv[0] << " <network> <dbname> <host> [port]" << endl
                 << "# Supported networks: " << stdutils::delimited_list(networkSelector.getNetworkNames(), ", ") << endl;
            return -1;
        }

        networkSelector.select(argv[1]);
    }
    catch (const exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return -2;
    }

    const CoinParams& coinParams = networkSelector.getCoinParams();

    string dbname = argv[2];
    string host = argv[3];
    string port = argc > 4 ? argv[4] : coinParams.default_port();

    cout << endl << "Network Settings" << endl
         << "-------------------------------------------" << endl
         << "  network:          " << coinParams.network_name() << endl
         << "  host:             " << host << endl
         << "  port:             " << port << endl
         << "  magic bytes:      " << hex << coinParams.magic_bytes() << endl
         << "  protocol version: " << dec << coinParams.protocol_version() << endl
         << endl;

        
    INIT_LOGGER("syncdb.log");

    signal(SIGINT, &finish);

    SynchedVault synchedVault(coinParams);
    synchedVault.subscribeTxInserted([](std::shared_ptr<Tx> tx)
    {
        cout << "Transaction inserted: " << uchar_vector(tx->hash()).getHex() << endl;
    });
    synchedVault.subscribeTxStatusChanged([](std::shared_ptr<Tx> tx)
    {
        cout << "Transaction status changed: " << uchar_vector(tx->hash()).getHex() << " New status: " << Tx::getStatusString(tx->status()) << endl;
    });
    synchedVault.subscribeMerkleBlockInserted([](std::shared_ptr<MerkleBlock> merkleblock)
    {
        cout << "Merkle block inserted: " << uchar_vector(merkleblock->blockheader()->hash()).getHex() << " Height: " << merkleblock->blockheader()->height() << endl;
    });

    try
    {
        cout << "Opening coin database " << dbname << endl;
        LOGGER(info) << "Opening coin database " << dbname << endl;
        synchedVault.openVault(dbname);

        cout << "Loading block tree " << BLOCKTREE_FILENAME << endl;
        LOGGER(info) << "Loading block tree " << BLOCKTREE_FILENAME << endl;
        synchedVault.loadHeaders(BLOCKTREE_FILENAME);

        cout << "Connecting to " << host << ":" << port << endl;
        LOGGER(info) << "Connecting to " << host << ":" << port << endl;
        synchedVault.startSync(host, port);
    }
    catch (const std::exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    while (!g_bShutdown) { std::this_thread::sleep_for(std::chrono::microseconds(200)); }

    return 0;

}
