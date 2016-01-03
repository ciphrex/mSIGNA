///////////////////////////////////////////////////////////////////////////////
//
// script templates example
//
// templates.cpp
//
// Copyright (c) 2012-2016 Eric Lombrozo
//
// All Rights Reserved.

#include <CoinQ/CoinQ_script.h>
#include <CoinCore/Base58Check.h>
#include <CoinCore/hash.h>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace Coin;
using namespace CoinQ::Script;
using namespace std;

const unsigned char ADDRESS_VERSIONS[] = {30, 50};

string help(char* appName)
{
    stringstream ss;
    ss  << "# Usage: " << appName << " <type> [params]" << endl
        << "#   p2pkh <pubkey>" << endl
        << "#   mofn <m> <pubkey1> [pubkey2] ...";
    return ss.str();
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        cerr << help(argv[0]) << endl;
        return -1;
    }

    try
    {
        string type(argv[1]);
        if (type == "p2pkh")
        {
            if (argc != 3) throw runtime_error(help(argv[0]));

            uchar_vector pubkey(argv[2]);
            if (pubkey.size() != 33) throw runtime_error("Invalid pubkey length.");

            uchar_vector tokenPubKey;
            tokenPubKey << OP_DUP << OP_HASH160 << OP_PUBKEYHASH << 0 << OP_EQUALVERIFY << OP_CHECKSIG;
            ScriptTemplate scriptPubKeyTemplate(tokenPubKey);
            cout << "scriptPubKey:  " << scriptPubKeyTemplate.script(pubkey).getHex() << endl;

            uchar_vector tokenSig;
            tokenSig << OP_0 << OP_PUBKEY << 0;
            ScriptTemplate scriptSigTemplate(tokenSig);
            cout << "scriptSig:     " << scriptSigTemplate.script(pubkey).getHex() << endl;

            cout << "address:       " << toBase58Check(hash160(pubkey), ADDRESS_VERSIONS[0]) << endl;
        }
        else if (type == "mofn")
        {
            if (argc < 4) throw runtime_error(help(argv[0]));

            int minsigs = strtol(argv[2], NULL, 0);
            if (minsigs < 1 || minsigs > 15)
                throw runtime_error("Invalid minsigs.");

            vector<bytes_t> pubkeys;
            for (int i = 3; i < argc; i++)
            {
                uchar_vector pubkey(argv[i]);
                if (pubkey.size() != 33) throw runtime_error("Invalid pubkey length.");

                pubkeys.push_back(pubkey);
            }
            if ((int)pubkeys.size() < minsigs || pubkeys.size() > 15)
                throw runtime_error("Invalid pubkey count.");

            uchar_vector tokenRedeemScript;
            tokenRedeemScript << (OP_1_OFFSET + minsigs);
            for (size_t i = 0; i < pubkeys.size(); i++)
            {
                tokenRedeemScript << OP_PUBKEY << i;
            }
            tokenRedeemScript << (OP_1_OFFSET + pubkeys.size());

            SymmetricKeyGroup keyGroup(pubkeys);
            ScriptTemplate redeemscriptTemplate(tokenRedeemScript);
            uchar_vector redeemscript = redeemscriptTemplate.script(keyGroup.pubkeys());

            cout << "redeemscript: " << redeemscript.getHex() << endl;

            uchar_vector scriptPubKey;
            scriptPubKey << OP_HASH160 << pushStackItem(hash160(redeemscript)) << OP_EQUAL;

            cout << "scriptPubKey: " << scriptPubKey.getHex() << endl;

            cout << "address:      " << toBase58Check(hash160(redeemscript), ADDRESS_VERSIONS[1]) << endl;
        }
        else
        {
            throw runtime_error(help(argv[0]));
        }

    }
    catch (const exception& e)
    {
        cerr << e.what() << endl;
        return -2;
    }

    return 0;
}
