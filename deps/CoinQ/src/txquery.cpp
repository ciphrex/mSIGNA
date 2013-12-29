////////////////////////////////////////////////////////////////////////////////
//
// txquery.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#define SQLITE3CPP_LOG

#include "CoinQ_filter.h"
#include "CoinQ_blocks.h"
#include "CoinQ_txs.h"
#include "CoinQ_keys.h"
#include "CoinQ_sqlite3.h"

#include <iostream>
#include <string>

using namespace Coin;

int main(int argc, char** argv)
{
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: " << argv[0] << " [tx file] [address file] <minconf=1>" << std::endl;
        return -1;
    }

    std::string txFile        = argv[1];
    std::string addressesFile = argv[2];
    int minConf = (argc > 3) ? strtol(argv[3], NULL, 0) : 1;

    CoinQTxStoreSqlite3 txStore;
    try {
        std::cout << "Opening transaction file..." << std::flush;
        txStore.open(txFile);
        std::cout << "Done." << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << std::endl;
        std::cerr << "Exception opening " << txFile << ": " << e.what() << std::endl;
        return 1;
    }

    CoinQ::Keys::AddressSet filterAddresses;
    std::cout << "Loading filter addresses..." << std::flush;
    try {
        filterAddresses.loadFromFile(addressesFile);
        std::cout << "Done." << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << std::endl;
        std::cerr << e.what() << std::endl;
        return 2;
    }

    try
    {
        uint64_t coins = 0;
        std::vector<ChainTxOut> txOuts;
        txOuts = txStore.getTxOuts(filterAddresses, ICoinQTxStore::Status::UNSPENT, minConf);
        for (auto& txOut: txOuts) {
            std::cout << txOut.toJson() << std::endl;
            coins += txOut.value;
        }
        std::cout << "Total coins: " << (coins / 100000000ull) << "." << (coins % 100000000ull) << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl << std::endl;
        return 3;
    }

    return 0;

}
