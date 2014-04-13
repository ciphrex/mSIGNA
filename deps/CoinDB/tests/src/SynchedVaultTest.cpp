///////////////////////////////////////////////////////////////////////////////
//
// SynchedVaultTest.cpp
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.
//

#include <SynchedVault.h>
#include <logger.h>

#include <iostream>
#include <signal.h>

#include <thread>
#include <chrono>

using namespace CoinDB;
using namespace std;

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

    try
    {
        LOGGER(debug) << "Opening vault " << filename << endl;
        synchedVault.openVault(filename);
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
