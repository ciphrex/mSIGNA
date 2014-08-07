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
#include <logger/logger.h>

#include <iostream>
#include <signal.h>

#include <thread>
#include <chrono>

using namespace CoinDB;
using namespace std;

const std::string BLOCKTREE_FILENAME = "blocktree.dat";

bool g_bShutdown = false;

void finish(int sig)
{
    LOGGER(debug) << "Stopping..." << endl;
    g_bShutdown = true;
}

int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        cout << "Usage: " << argv[0] << " [vault file] [p2p host] [p2p port]" << endl;
        return 0;
    }

    string filename(argv[1]);
    string host(argv[2]);
    string port(argv[3]);
        
    INIT_LOGGER("SynchedVaultTest.log");

    signal(SIGINT, &finish);

    SynchedVault synchedVault;
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
        LOGGER(debug) << "Opening vault " << filename << endl;
        synchedVault.openVault(filename);
        LOGGER(debug) << "Loading block tree " << BLOCKTREE_FILENAME << endl;
        synchedVault.loadHeaders(BLOCKTREE_FILENAME);
        LOGGER(debug) << "Attempting to sync with " << host << ":" << port << endl;
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
