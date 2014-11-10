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

void showUsage(char* argv[])
{
    cerr << "# Usage 1: " << argv[0] << " <master key> <path>" << endl;
    cerr << "# Usage 2: " << argv[0] << " <minsigs> <master key 1> <path 1> ... [master key n] [path n]" << endl;
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        showUsage(argv);
        return -1;
    }

    try
    {
        if (argc == 3)
        {
            bytes_t extkey;
            if (!fromBase58Check(string(argv[1]), extkey)) throw runtime_error("Invalid master key base58.");

            HDKeychain keychain(extkey);
            keychain = keychain.getChild(string(argv[2]));

            uchar_vector pubkey = keychain.pubkey();
            vector<bytes_t> pubkeys;
            pubkeys.push_back(pubkey);

            Script script(Script::PAY_TO_PUBKEY_HASH, 1, pubkeys);
            uchar_vector txoutscript = script.txoutscript();
            cout << "TxOut script: " << txoutscript.getHex() << endl;
            cout << "Address:      " << getAddressForTxOutScript(txoutscript, ADDRESS_VERSIONS) << endl;
            cout << "Public key:   " << pubkey.getHex() << endl;
            return 0;
        }

        if (argc % 2)
        {
            showUsage(argv);
            return -1;
        }

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

        //sort(pubkeys.begin(), pubkeys.end());

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
