///////////////////////////////////////////////////////////////////////////////
//
// txbuilder_commands.cpp
//
// Copyright (c) 2011-2013 Eric Lombrozo
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "txbuilder_commands.h"

#include <StandardTransactions.h>
#include <CoinKey.h>
#include <numericdata.h>

#include <map>
#include <iostream>

using namespace Coin;

std::string newtx(bool bHelp, params_t& params)
{
    if (bHelp || params.size() > 2) {
        return "newtx [version=1] [locktime=0] - create a blank transaction.";
    }

    uint32_t version = (params.size() > 0) ? strtoul(params[0].c_str(), NULL, 0) : 1;
    uint32_t locktime = (params.size() > 1) ? strtoul(params[1].c_str(), NULL, 0) : 0;

    Transaction tx;
    tx.version = version;
    tx.lockTime = locktime;
    return tx.getSerialized().getHex();
}

std::string createmultisig(bool bHelp, params_t& params)
{
    if (bHelp || params.size() < 2) {
        return "createmultisig <nrequired> <key 1> [<key 2> <key 3> ...] - creates a multisignature address.";
    }

    MultiSigRedeemScript multiSig(strtoul(params[0].c_str(), NULL, 10));
    int nKeys = params.size();
    for (int i = 1; i < nKeys; i++) {
        multiSig.addPubKey(params[i]);
    }
    return multiSig.toJson();
}

std::string parsemultisig(bool bHelp, params_t& params)
{
    if (bHelp || params.size() != 1) {
        return "parsemultisig <redeemScript> - parses a multisignature redeem script.";
    }

    MultiSigRedeemScript multiSig;
    multiSig.parseRedeemScript(params[0]);
    return multiSig.toJson(true);
}

std::string addoutput(bool bHelp, params_t& params)
{
    if (bHelp || params.size() != 3) {
        return "addoutput <txblob> <address> <value> - add a standard output to a transaction. Pass \"null\" for txhex to create a new transaction.";
    }

    TransactionBuilder txBuilder;
    if (params[0] != "") {
        txBuilder.setSerialized(params[0]);
        txBuilder.addOutput(params[1], strtoull(params[2].c_str(), NULL, 10));
    }
    return txBuilder.getSerialized().getHex();
}

std::string removeoutput(bool bHelp, params_t& params)
{
    if (bHelp || params.size() != 2) {
        return "removeoutput <txbuilder blob> <index>";
    }

    TransactionBuilder txBuilder;
    txBuilder.setSerialized(params[0]);
    txBuilder.removeOutput(strtoul(params[1].c_str(), NULL, 10));
    return txBuilder.getSerialized().getHex();
}

// TODO : Make function detect input type automatically.
std::string addaddressinput(bool bHelp, params_t& params)
{
    if (bHelp || params.size() < 4 || params.size() > 5) {
        return "addaddressinput <txhex> <outhash> <outindex> <pubkey> [sequence = 0xffffffff] - adds a standard pay-to-address input to a transaction with an optional signature. Pass \"null\" for txhex to create a new transaction.";
    }

    Transaction tx;
    if (params[0] != "") {
        tx.setSerialized(uchar_vector(params[0]));
    }

    uint32_t sequence = (params.size() == 5) ? strtoul(params[4].c_str(), NULL, 0) : 0xffffffff;
    P2AddressTxIn txIn(params[1], strtoul(params[2].c_str(), NULL, 10), params[3], sequence);
    txIn.setScriptSig(SCRIPT_SIG_EDIT);

    tx.addInput(txIn);
    return tx.getSerialized().getHex();
}

std::string addmofninput(bool bHelp, params_t& params)
{
    if (bHelp || params.size() < 4 || params.size() > 5) {
        return "addmofninput <txhex> <outhash> <outindex> <redeemscript> [sequence = 0xffffffff] - adds an m-of-n input to transaction.";
    }

    Transaction tx;
    if (params[0] != "") {
        tx.setSerialized(uchar_vector(params[0]));
    }

    uint32_t sequence = (params.size() == 5) ? strtoul(params[4].c_str(), NULL, 0) : 0xffffffff;
    MofNTxIn txIn(params[1], strtoul(params[2].c_str(), NULL, 10), params[3], sequence);
    txIn.setScriptSig(SCRIPT_SIG_EDIT);

    tx.addInput(txIn);
    return tx.getSerialized().getHex();
}

std::string adddeps(bool bHelp, params_t& params)
{
    if (bHelp || params.size() < 2) {
        return "adddeps <txbuilder blob> <dep1> [<dep2> <dep3> ...] - adds dependency transactions to builder object.";
    }

    std::string concat = "";
    for (uint i = 0; i < params.size(); i++) {
        concat += params[i];
    }

    TransactionBuilder txBuilder;
    txBuilder.setSerialized(concat);
    return txBuilder.getSerialized().getHex();
}

std::string listdeps(bool bHelp, params_t& params)
{
    if (bHelp || params.size() != 1) {
        return "listdeps <txbuilder blob> - lists the hashes of included dependency transactions.";
    }

    TransactionBuilder txBuilder;
    txBuilder.setSerialized(params[0]);
    std::vector<uchar_vector> hashes = txBuilder.getDependencyHashes();

    std::stringstream ss;
    ss << "[";
    for (uint i = 0; i < hashes.size(); i++) {
        if (i > 0) ss << ", ";
        ss << "\"" << hashes[i].getHex() << "\"";
    }
    ss << "]";
    return ss.str();
}

std::string stripdeps(bool bHelp, params_t& params)
{
    if (bHelp || params.size() != 1) {
        return "stripdeps <txbuilder blob> - strips the blob of all unused dependencies.";
    }

    TransactionBuilder txBuilder;
    txBuilder.setSerialized(params[0]);
    txBuilder.stripDependencies();
    return txBuilder.getSerialized().getHex();
}

std::string addinput(bool bHelp, params_t& params)
{
    if (bHelp || params.size() < 4 || params.size() > 5) {
        return "addinput <txbuilder blob> <outhash> <outindex> <pubkey> [sequence = 0xffffffff] - adds an input to a transaction from a dependency transaction.";
    }

    TransactionBuilder txBuilder;
    txBuilder.setSerialized(params[0]);

    uint32_t sequence = (params.size() == 5) ? strtoul(params[4].c_str(), NULL, 0) : 0xffffffff;
    txBuilder.addInput(params[1], strtoul(params[2].c_str(), NULL, 10), params[3], sequence);
    return txBuilder.getSerialized().getHex();
}

std::string removeinput(bool bHelp, params_t& params)
{
    if (bHelp || params.size() != 2) {
        return "removeinput <txbuilder blob> <index>";
    }

    TransactionBuilder txBuilder;
    txBuilder.setSerialized(params[0]);
    txBuilder.removeInput(strtoul(params[1].c_str(), NULL, 10));
    return txBuilder.getSerialized().getHex();
}

std::string sign(bool bHelp, params_t& params)
{
    if (bHelp || params.size() < 4 || params.size() > 5) {
        return "sign <txhex> <index> <pubkey> <privkey> [sighashtype = 0x01] - sign and add signature to input.";
    }

    Transaction tx;
    tx.setSerialized(uchar_vector(params[0]));

    SigHashType sigHashType =
        (params.size() == 5) ? static_cast<SigHashType>(strtoul(params[4].c_str(), NULL, 0)) : SIGHASH_ALL;

    TransactionBuilder txBuilder(tx);
    txBuilder.sign(strtoul(params[1].c_str(), NULL, 10), params[2], uchar_vector(params[3]), sigHashType);

    return txBuilder.getSerialized().getHex();
}

std::string getmissingsigs(bool bHelp, params_t& params)
{
    if (bHelp || params.size() != 1) {
        return "getmissingsigs <txblob>";
    }

    TransactionBuilder txBuilder;
    txBuilder.setSerialized(params[0]);
    return txBuilder.getMissingSigsJson();
}

// TODO: make sure no signatures are missing
std::string getbroadcast(bool bHelp, params_t& params)
{
    if (bHelp || params.size() != 1) {
        return "getbroadcast <txhex> - gets the transaction in broadcast format.";
    }

    TransactionBuilder txBuilder(params[0]);
    return txBuilder.getTx(SCRIPT_SIG_BROADCAST).getSerialized().getHex();
}

std::string getsign(bool bHelp, params_t& params)
{
    if (bHelp || params.size() != 1) {
        return "getsign <txhex> - gets the transaction in signing format.";
    }

    TransactionBuilder txBuilder(params[0]);
    return txBuilder.getTx(SCRIPT_SIG_SIGN).getSerialized().getHex();
}
