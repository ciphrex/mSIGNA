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
