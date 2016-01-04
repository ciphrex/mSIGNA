///////////////////////////////////////////////////////////////////////////////
//
// script templates HD multisig example
//
// hdmofn.cpp
//
// Copyright (c) 2012-2016 Eric Lombrozo
//
// All Rights Reserved.

#include <CoinQ/CoinQ_script.h>
#include <CoinCore/Base58Check.h>
#include <CoinCore/hash.h>
#include <CoinCore/hdkeys.h>
#include <CoinCore/random.h>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace Coin;
using namespace CoinQ::Script;
using namespace std;

const unsigned char ADDRESS_VERSIONS[] = {0, 5};

string help(char* appName)
{
    stringstream ss;
    ss  << "# Usage: " << appName << " <m> <n>";
    return ss.str();
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        cerr << help(argv[0]) << endl;
        return -1;
    }

    try
    {
        int m = strtol(argv[1], NULL, 0);
        int n = strtol(argv[2], NULL, 0);

        if (m < 1 || m > 15 || m > n) throw runtime_error("Invalid m.");
        if (n < 1 || n > 15) throw runtime_error("Invalid n.");

        uchar_vector redeemPattern, emptySigs;
        redeemPattern << (OP_1_OFFSET + m);
        for (int k = 0; k < n; k++)
        {
            redeemPattern << OP_PUBKEY << k;
            emptySigs << OP_0;
        }
        redeemPattern << (OP_1_OFFSET + n) << OP_CHECKMULTISIG;
        ScriptTemplate redeemTemplate(redeemPattern);

        uchar_vector inPattern;
        inPattern << OP_0 << emptySigs << OP_TOKEN << 0;
        ScriptTemplate inTemplate(inPattern);

        uchar_vector outPattern;
        outPattern << OP_HASH160 << OP_TOKENHASH << 0 << OP_EQUAL;
        ScriptTemplate outTemplate(outPattern);

        vector<uchar_vector> extkeys;
        for (int k = 0; k < n; k++) { extkeys.push_back(HDSeed(secure_random_bytes(32)).getExtendedKey()); }

        cout    << endl;
        cout    << "Extended Keys"
                << "-------------" << endl;
        for (auto& extkey: extkeys)
        {
            cout << toBase58Check(extkey) << endl; 
        }

        SymmetricHDKeyGroup keyGroup(extkeys);

        cout    << endl;
        cout    << "Scripts"
                << "-------" << endl;
        for (int i = 1; i <= 10; i++)
        {
            uchar_vector redeemscript   = redeemTemplate.script(keyGroup.pubkeys());
            uchar_vector txoutscript    = outTemplate.script(redeemscript);
            uchar_vector txinscript     = inTemplate.script(redeemscript);

            cout << "index:         " << keyGroup.index() << endl;
            cout << "redeemscript:  " << redeemscript.getHex() << endl;
            cout << "txoutscript:   " << txoutscript.getHex() << endl;
            cout << "txinscript:    " << txinscript.getHex() << endl;
            cout << "address:       " << toBase58Check(hash160(redeemscript), ADDRESS_VERSIONS[1]) << endl << endl;

            keyGroup.next();
        }
    }
    catch (const exception& e)
    {
        cerr << e.what() << endl;
        return -2;
    }

    return 0;
}
