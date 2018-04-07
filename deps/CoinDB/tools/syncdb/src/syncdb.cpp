///////////////////////////////////////////////////////////////////////////////
//
// syncdb.cpp
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//
// Utility for synchronizing a coin database with p2p network
//

#include "SyncDBConfig.h"

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

const std::string VERSION_INFO = "v0.2.0";

bool g_bShutdown = false;

void finish(int sig)
{
    LOGGER(debug) << "Stopping..." << endl;
    g_bShutdown = true;
}

void subscribeHandlers(SynchedVault& synchedVault)
{
    synchedVault.subscribeStatusChanged([&](SynchedVault::status_t status) {
        stringstream ss;
        ss << "Sync status: " << SynchedVault::getStatusString(status);
        LOGGER(info) << ss.str() << endl;
        cout << ss.str() << endl;
        if (status == SynchedVault::STOPPED) { g_bShutdown = true; }
    });

    synchedVault.subscribeTxInserted([](std::shared_ptr<Tx> tx)
    {
        stringstream ss;
        ss << "Transaction inserted: " << uchar_vector(tx->hash()).getHex();
        LOGGER(info) << ss.str() << endl;
        cout << ss.str() << endl;
    });

    synchedVault.subscribeTxUpdated([](std::shared_ptr<Tx> tx)
    {
        stringstream ss;
        ss << "Transaction updated: " << uchar_vector(tx->hash()).getHex() << " Status: " << Tx::getStatusString(tx->status());
        LOGGER(info) << ss.str() << endl;
        cout << ss.str() << endl;
    });

    synchedVault.subscribeMerkleBlockInserted([](std::shared_ptr<MerkleBlock> merkleblock)
    {
        stringstream ss;
        ss << "Merkle block inserted: " << uchar_vector(merkleblock->blockheader()->hash()).getHex() << " Height: " << merkleblock->blockheader()->height();
        LOGGER(info) << ss.str() << endl;
        cout << ss.str() << endl;
    });

    synchedVault.subscribeTxInsertionError([](std::shared_ptr<Tx> tx, const std::string& description)
    {
        stringstream ss;
        ss << "Transaction insertion error: " << uchar_vector(tx->hash()).getHex() << endl << "  " << description;
        LOGGER(info) << ss.str() << endl;
        cout << ss.str() << endl;
    });

    synchedVault.subscribeMerkleBlockInsertionError([](std::shared_ptr<MerkleBlock> merkleblock, const std::string& description)
    {
        stringstream ss;
        ss << "Merkle block insertion error: " << uchar_vector(merkleblock->blockheader()->hash()).getHex() << " Height: " << merkleblock->blockheader()->height() << endl << "  " << description;
        LOGGER(info) << ss.str() << endl;
        cout << ss.str() << endl;
    });

    synchedVault.subscribeBestHeaderChanged([](uint32_t bestheight, const bytes_t& besthash)
    {
        stringstream ss;
        ss << "Best height: " << bestheight << " Best hash: " << uchar_vector(besthash).getHex();
        LOGGER(info) << ss.str() << endl;
        cout << ss.str() << endl;
    });

    synchedVault.subscribeSyncHeaderChanged([](uint32_t syncheight, const bytes_t& synchash)
    {
        stringstream ss;
        ss << "Sync height: " << syncheight << " Sync hash: " << uchar_vector(synchash).getHex();
        LOGGER(info) << ss.str() << endl;
        cout << ss.str() << endl;
    });

    synchedVault.subscribeProtocolError([](const string& error, int /*code*/)
    {
        stringstream ss;
        ss << "Protocol error: " << error;
        LOGGER(error) << ss.str() << endl;
        cout << ss.str() << endl;
    });

    synchedVault.subscribeConnectionError([](const string& error, int /*code*/)
    {
        stringstream ss;
        ss << "Connection error: " << error;
        LOGGER(error) << ss.str() << endl;
        cout << ss.str() << endl;
    });

    synchedVault.subscribeBlockTreeError([](const string& error, int /*code*/)
    {
        stringstream ss;
        ss << "Blocktree error: " << error;
        LOGGER(error) << ss.str() << endl;
        cout << ss.str() << endl;
    });
}

int main(int argc, char* argv[])
{
    SyncDBConfig config;
    NetworkSelector networkSelector;

    try
    {
        if (!config.parseParams(argc, argv))
        {
            cout << config.getHelpOptions();
            return 0;
        }

        if (argc < 4)
        {
            cerr << "SyncDB by Eric Lombrozo " << VERSION_INFO << endl
                 << "# Usage: " << argv[0] << " <network> <dbname> <host> [port]" << endl
                 << "# Supported networks: " << stdutils::delimited_list(networkSelector.getNetworkNames(), ", ") << endl
                 << "# Use " << argv[0] << " --help for more options." << endl;
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

    string logfile = config.getDataDir() + "/syncdb.log";    
    INIT_LOGGER(logfile.c_str());

    string blocktreefile = config.getDataDir() + "/" + coinParams.network_name() + "_headers.dat";

    signal(SIGINT, &finish);
    signal(SIGTERM, &finish);

    LOGGER(trace) << "foo" << endl;
    SynchedVault synchedVault(coinParams);
    LOGGER(trace) << "bar" << endl;
    subscribeHandlers(synchedVault);

    try
    {
        cout << "Opening coin database " << dbname << endl;
        LOGGER(info) << "Opening coin database " << dbname << endl;
        synchedVault.openVault(config.getDatabaseUser(), config.getDatabasePassword(), dbname);

        cout << "Loading block tree " << blocktreefile << "..." << endl;
        LOGGER(info) << "Loading block tree " << blocktreefile << endl;
        synchedVault.loadHeaders(blocktreefile, false, [&](const CoinQBlockTreeMem& blockTree) {
            cout << "  " << blockTree.getBestHash().getHex() << " height: " << blockTree.getBestHeight() << endl;
            return !g_bShutdown;
        });

        if (g_bShutdown)
        {
            LOGGER(info) << "Interrupted." << endl;
            cout << "Interrupted." << endl;
            return 0;
        } 

        cout << "Done." << endl << endl;

        stringstream ss;
        ss << endl << "Network Settings" << endl
           << "-------------------------------------------" << endl
           << "  network:          " << coinParams.network_name() << endl
           << "  host:             " << host << endl
           << "  port:             " << port << endl
           << "  magic bytes:      " << hex << coinParams.magic_bytes() << endl
           << "  protocol version: " << dec << coinParams.protocol_version() << endl;

        LOGGER(info) << ss.str() << endl;
        cout << ss.str() << endl;

        cout << "Connecting to " << host << ":" << port << endl;
        LOGGER(info) << "Connecting to " << host << ":" << port << endl;
        synchedVault.startSync(host, port);
    }
    catch (const std::exception& e)
    {
        LOGGER(error) << "Error: " << e.what() << endl;
        cerr << "Error: " << e.what() << endl;
        synchedVault.stopSync();
        return 1;
    }

    while (!g_bShutdown) { std::this_thread::sleep_for(std::chrono::microseconds(200)); }

    synchedVault.stopSync();

    return 0;

}
