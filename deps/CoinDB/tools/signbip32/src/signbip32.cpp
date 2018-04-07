///////////////////////////////////////////////////////////////////////////////
//
// signbip32.cpp
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include <CoinCore/hdkeys.h>
#include <CoinCore/Base58Check.h>
#include <CoinCore/secp256k1_openssl.h>
#include <CoinQ/CoinQ_script.h>
#include <stdutils/uchar_vector.h>

#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace Coin;
using namespace CoinCrypto;
using namespace CoinQ::Script;
using namespace std;

const unsigned char ADDRESS_VERSIONS[] = { 0x00, 0x05 };

void showUsage(char* argv[])
{
    cerr << "# Usage: " << argv[0] << " <master key> <path> <data hex>" << endl;
}

int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        showUsage(argv);
        return -1;
    }

    try
    {
        bytes_t extkey;
        if (!fromBase58Check(string(argv[1]), extkey)) throw runtime_error("Invalid master key base58.");

        HDKeychain keychain(extkey);
        if (!keychain.isPrivate()) throw runtime_error("Master key is not private.");

        keychain = keychain.getChild(string(argv[2]));
        uchar_vector data(argv[3]);

        secure_bytes_t privkey = keychain.privkey();
        secp256k1_key key;
        key.setPrivKey(privkey);
        uchar_vector signature = secp256k1_sign(key, data);

        cout << "Signature: " << uchar_vector(signature).getHex() << endl;
    }
    catch (const exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return -2;
    }

    return 0;
}
