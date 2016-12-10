///////////////////////////////////////////////////////////////////////////////
//
// script templates HD multisig example
//
// hdmofn.cpp
//
// Copyright (c) 2012-2016 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

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
    ss  << "# Usage: " << appName << " <m> (<n> or <xpub1> [xpub2] ...)";
    return ss.str();
}

int main(int argc, char* argv[])
{
    if (argc < 3 || argc > 16)
    {
        cerr << help(argv[0]) << endl;
        return -1;
    }

    try
    {
        int m = strtol(argv[1], NULL, 0);
        if (m < 1 || m > 15) throw runtime_error("Invalid m.");

        int n;

        vector<uchar_vector> extkeys;

        string xpubstr(argv[2]);
        if (xpubstr.size() <= 2)
        {
            n = strtol(argv[2], NULL, 0);
            if (n < m || n > 15) throw runtime_error("Invalid n.");
            for (int k = 0; k < n; k++) { extkeys.push_back(HDSeed(secure_random_bytes(32)).getExtendedKey()); }
        }
        else
        {
            for (int k = 2; k < argc; k++)
            {
                uchar_vector xpub;
                if (!fromBase58Check(argv[k], xpub))
                    throw runtime_error("Invalid xpub.");

                extkeys.push_back(xpub);
            }

            n = extkeys.size();
            if (n < m || n > 15) throw runtime_error("Invalid n.");
        }

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
