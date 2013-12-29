////////////////////////////////////////////////////////////////////////////////
//
// wallet.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include "CoinQ_filter.h"
#include "CoinQ_blocks.h"
#include "CoinQ_txs.h"
#include "CoinQ_keys.h"
#include "CoinQ_sqlite3.h"
#include "CoinQ_signer.h"

#include <Base58Check.h>

#include <iostream>
#include <string>

using namespace Coin;
using namespace CoinQ::Keys;

int main(int argc, char** argv)
{
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " [tx file] [key file] <address> <amount>>" << std::endl;
        return -1;
    }

    std::string txFile        = argv[1];
    std::string keyFile = argv[2];
//    int nNewKeys = (argc > 3) ? strtoul(argv[3], NULL, 10) : 0;
    std::string address = argv[3];
    uint64_t amount = strtoull(argv[4], NULL, 10);

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

    CoinQKeychainSqlite3 keychain;
    try {
        std::cout << "Opening key chain..." << std::flush;
        keychain.open(keyFile);
        std::cout << "Done." << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << std::endl;
        std::cerr << "Exception opening " << keyFile << ": " << e.what() << std::endl;
        return 1;
    }

//    if (nNewKeys > 0) keychain.generateNewKeys(nNewKeys, IKeychain::Type::RECEIVING, true);
    try {
        TransactionBuilder txBuilder;
        CoinQ::Signer::addCoins(keychain, txStore, txBuilder, amount, 1);
        txBuilder.addOutput(address, amount);
        
        txBuilder.clearDependencies();
        std::cout << txBuilder.getTx(SCRIPT_SIG_BROADCAST).toIndentedString() << std::endl;    
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 2;
    }
/*
    AddressSet addresses;
    std::vector<uchar_vector> keyHashes = keychain.getKeyHashes(IKeychain::Type::RECEIVING | IKeychain::Type::CHANGE, IKeychain::Status::POOL | IKeychain::Status::ALLOCATED);
    std::string address;
    for (auto& keyHash: keyHashes) {
        address = toBase58Check(keyHash, 0); 
        std::cout << address << std::endl;
        addresses.insert(address);
    }


    uint64_t coins = 0;
    std::vector<ChainTxOut> txOuts;

    try
    {
        txOuts = txStore.getTxOuts(addresses, ICoinQTxStore::Status::UNSPENT, 0);
        for (auto& txOut: txOuts) {
            std::cout << txOut.toJson() << std::endl;
            coins += txOut.value;
        }
        std::cout << "Total coins: " << (coins / 100000000ull) << "." << (coins % 100000000ull) << std::endl;

        CoinQSimpleInputPicker picker(txStore, 
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl << std::endl;
        return 3;
    }
*/
    return 0;
}
