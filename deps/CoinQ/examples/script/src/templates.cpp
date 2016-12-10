///////////////////////////////////////////////////////////////////////////////
//
// script templates example
//
// templates.cpp
//
// Copyright (c) 2012-2016 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include <CoinQ/CoinQ_script.h>
#include <CoinQ/scriptnum.h>
#include <CoinCore/Base58Check.h>
#include <CoinCore/hash.h>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace Coin;
using namespace CoinQ::Script;
using namespace std;

const unsigned char ADDRESS_VERSIONS[] = {30, 50, 6, 40};

string help(char* appName)
{
    stringstream ss;
    ss  << "# Usage: " << appName << " <type> [params]" << endl
        << "#   p2pk    <pubkey>" << endl
        << "#   p2pkh   <pubkey>" << endl
        << "#   mofn    <m> <pubkey1> [pubkey2] ..." << endl
        << "#   cltv    <master pubkey> <timelock pubkey> <locktime>" << endl
        << "#   p2wpkh  <pubkey>" << endl
        << "#   p2wsh   <redeemscript>";
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
        if (type == "p2pk")
        {
            if (argc != 3) throw runtime_error(help(argv[0]));

            uchar_vector pubkey(argv[2]);
            if (pubkey.size() != 33) throw runtime_error("Invalid pubkey length.");

            uchar_vector outPattern;
            outPattern << OP_PUBKEY << 0 << OP_CHECKSIG;
            ScriptTemplate outTemplate(outPattern);

            uchar_vector inPattern;
            inPattern << OP_0;
            ScriptTemplate inTemplate(inPattern);

            cout << "txoutscript:   " << outTemplate.script(pubkey).getHex() << endl;
            cout << "txinscript:    " << inTemplate.script(pubkey).getHex() << endl;
            cout << "address:       " << toBase58Check(hash160(pubkey), ADDRESS_VERSIONS[0]) << endl;
        }
        else if (type == "p2pkh")
        {
            if (argc != 3) throw runtime_error(help(argv[0]));

            uchar_vector pubkey(argv[2]);
            if (pubkey.size() != 33) throw runtime_error("Invalid pubkey length.");

            uchar_vector outPattern;
            outPattern << OP_DUP << OP_HASH160 << OP_PUBKEYHASH << 0 << OP_EQUALVERIFY << OP_CHECKSIG;
            ScriptTemplate outTemplate(outPattern);

            uchar_vector inPattern;
            inPattern << OP_0 << OP_PUBKEY << 0;
            ScriptTemplate inTemplate(inPattern);

            cout << "txoutscript:   " << outTemplate.script(pubkey).getHex() << endl;
            cout << "txinscript:    " << inTemplate.script(pubkey).getHex() << endl;
            cout << "address:       " << toBase58Check(hash160(pubkey), ADDRESS_VERSIONS[0]) << endl;
        }
        else if (type == "mofn")
        {
            if (argc < 4) throw runtime_error(help(argv[0]));

            int minsigs = strtol(argv[2], NULL, 0);
            if (minsigs < 1 || minsigs > 15)
                throw runtime_error("Invalid minsigs.");

            vector<uchar_vector> pubkeys;
            for (int i = 3; i < argc; i++)
            {
                uchar_vector pubkey(argv[i]);
                if (pubkey.size() != 33) throw runtime_error("Invalid pubkey length.");

                pubkeys.push_back(pubkey);
            }
            if ((int)pubkeys.size() < minsigs || pubkeys.size() > 15)
                throw runtime_error("Invalid pubkey count.");

            uchar_vector redeemPattern, emptySigs;
            redeemPattern << (OP_1_OFFSET + minsigs);
            for (size_t i = 0; i < pubkeys.size(); i++)
            {
                redeemPattern << OP_PUBKEY << i;
                emptySigs << OP_0;
            }
            redeemPattern << (OP_1_OFFSET + pubkeys.size()) << OP_CHECKMULTISIG;

            SymmetricKeyGroup keyGroup(pubkeys);
            ScriptTemplate redeemTemplate(redeemPattern);
            uchar_vector redeemscript = redeemTemplate.script(keyGroup.pubkeys());

            uchar_vector txoutscript;
            txoutscript << OP_HASH160 << pushStackItem(hash160(redeemscript)) << OP_EQUAL;

            uchar_vector txinscript;
            txinscript << OP_0 << emptySigs << pushStackItem(redeemscript);

            cout << "redeemscript:  " << redeemscript.getHex() << endl;
            cout << "txoutscript:   " << txoutscript.getHex() << endl;
            cout << "txinscript:    " << txinscript.getHex() << endl;
            cout << "address:       " << toBase58Check(hash160(redeemscript), ADDRESS_VERSIONS[1]) << endl;
        }
        else if (type == "cltv")
        {
            if (argc != 5) throw runtime_error(help(argv[0]));

            uchar_vector master_pubkey(argv[2]);
                if (master_pubkey.size() != 33) throw runtime_error("Invalid master pubkey.");

            uchar_vector timelock_pubkey(argv[3]);
                if (timelock_pubkey.size() != 33) throw runtime_error("Invalid timelock pubkey.");

            uint32_t locktime = strtoul(argv[4], NULL, 0);

            uchar_vector redeemPattern;
            redeemPattern   <<  OP_DUP
                            <<  OP_PUBKEY << 1 << OP_CHECKSIG
                            <<  OP_IF
                            <<      pushStackItem(CScriptNum::serialize(locktime))
                            <<      OP_CHECKLOCKTIMEVERIFY << OP_DROP
                            <<  OP_ELSE
                            <<      OP_PUBKEY << 0 << OP_CHECKSIG
                            <<  OP_ENDIF;

            ScriptTemplate redeemTemplate(redeemPattern);
            redeemTemplate.apply(master_pubkey);
            redeemTemplate.apply(timelock_pubkey);
            uchar_vector redeemscript = redeemTemplate.script();

            uchar_vector txoutscript;
            txoutscript << OP_HASH160 << pushStackItem(hash160(redeemscript)) << OP_EQUAL;

            uchar_vector txinscript;
            txinscript << OP_0 << pushStackItem(redeemscript);

            cout << "redeemscript:  " << redeemscript.getHex() << endl;
            cout << "txoutscript:   " << txoutscript.getHex() << endl;
            cout << "txinscript:    " << txinscript.getHex() << endl;
            cout << "address:       " << toBase58Check(hash160(redeemscript), ADDRESS_VERSIONS[1]) << endl;
        }
        else if (type == "p2wpkh")
        {
            if (argc != 3) throw runtime_error(help(argv[0]));

            uchar_vector pubkey(argv[2]);
            WitnessProgram_P2WPKH wp(pubkey);
            cout << "type:        " << wp.typestring() << endl;
            cout << "script:      " << wp.script().getHex() << endl;
            cout << "p2shscript:  " << wp.p2shscript().getHex() << endl;
            cout << "p2shaddress: " << wp.p2shaddress(ADDRESS_VERSIONS) << endl;
            cout << "address:     " << wp.address(ADDRESS_VERSIONS) << endl;
            cout << "stack:" << endl;
            for (auto& item: wp.stack()) { cout << "  " << item.getHex() << endl; }
        }
        else if (type == "p2wsh")
        {
            if (argc != 3) throw runtime_error(help(argv[0]));

            uchar_vector redeemscript(argv[2]);
            WitnessProgram_P2WSH wp(redeemscript);
            cout << "type:        " << wp.typestring() << endl;
            cout << "script:      " << wp.script().getHex() << endl;
            cout << "p2shscript:  " << wp.p2shscript().getHex() << endl;
            cout << "p2shaddress: " << wp.p2shaddress(ADDRESS_VERSIONS) << endl;
            cout << "address:     " << wp.address(ADDRESS_VERSIONS) << endl;
            cout << "stack:" << endl;
            for (auto& item: wp.stack()) { cout << "  " << item.getHex() << endl; }
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
