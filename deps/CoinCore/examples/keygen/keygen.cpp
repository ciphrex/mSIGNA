////////////////////////////////////////////////////////////////////////////////
//
// keygen.cpp
//
// Generates a bitcoin signing key.
//
// Copyright (c) 2011-2012 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include <CoinKey.h>

#include <iostream>

using namespace std;

int main() 
{
    CoinKey coinKey;
    coinKey.generateNewKey();
    string receivingAddress = coinKey.getAddress();
    string walletImport = coinKey.getWalletImport();

    cout << endl
         << "Address:       " << receivingAddress << endl
         << "Wallet Import: " << walletImport << endl << endl;

    return 0;
}
