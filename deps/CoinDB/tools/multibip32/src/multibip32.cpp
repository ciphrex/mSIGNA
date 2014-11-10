///////////////////////////////////////////////////////////////////////////////
//
// multibip32.cpp
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.
//

#include <CoinCore/hdkeys.h>
#include <CoinCore/Base58Check.h>
#include <CoinQ/CoinQ_script.h>
#include <stdutils/uchar_vector.h>

#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace Coin;
using namespace CoinQ::Script;
using namespace std;

const unsigned char ADDRESS_VERSIONS[] = { 0x00, 0x05 };

int main(int argc, char* argv[])
{
    if (argc < 4 || argc % 2) // Parameter count must be even
    {
        cerr << "# Usage: " << argv[0] << " <minsigs> <master key 1> <path 1> ... [master key n] [path n]" << endl;
        return -1;
    }

    try
    {
        uint32_t minsigs = strtoul(argv[1], NULL, 10);
        vector<bytes_t> pubkeys;

        for (size_t i = 2; i < argc; i+=2)
        {
            bytes_t extkey;
            if (!fromBase58Check(string(argv[i]), extkey))
            {
                stringstream err;
                err << "Invalid master key base58: " << argv[i];
                throw runtime_error(err.str());
            }

            HDKeychain keychain(extkey);
            keychain = keychain.getChild(string(argv[i+1]));
            pubkeys.push_back(keychain.pubkey());
        }

        Script script(Script::PAY_TO_MULTISIG_SCRIPT_HASH, minsigs, pubkeys);
        uchar_vector txoutscript = script.txoutscript();
        cout << "TxOut script: " << txoutscript.getHex() << endl;
        cout << "Address:      " << getAddressForTxOutScript(txoutscript, ADDRESS_VERSIONS) << endl;
    }
    catch (const exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return -2;
    }

    return 0;
}
