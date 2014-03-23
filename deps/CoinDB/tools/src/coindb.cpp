///////////////////////////////////////////////////////////////////////////////
//
// coindb.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.
//

#include <cli.hpp>
#include <Base58Check.h>

#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <Vault.h>
#include <Schema-odb.hxx>

#include <random.h>

// TODO: create separate library
#include <stdutils/stringutils.h>

#include "logger.h"

#include <iostream>
#include <sstream>

using namespace std;
using namespace odb::core;
using namespace CoinDB;

// Vault operations
cli::result_t cmd_create(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 1)
        return "create <filename> - create a new vault.";

    Vault vault(params[0], true);

    stringstream ss;
    ss << "Vault " << params[0] << " created.";
    return ss.str();
}

// Keychain operations
cli::result_t cmd_keychainexists(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2)
        return "keychainexists <filename> <keychain_name> - check if a keychain exists.";

    Vault vault(params[0], false);
    bool bExists = vault.keychainExists(params[1]);

    stringstream ss;
    ss << (bExists ? "true" : "false");
    return ss.str();
}

cli::result_t cmd_newkeychain(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2)
        return "newkeychain <filename> <keychain_name> - create a new keychain.";

    Vault vault(params[0], false);
    vault.newKeychain(params[1], random_bytes(32));

    stringstream ss;
    ss << "Added keychain " << params[1] << " to vault " << params[0] << ".";
    return ss.str();
}
/*
cli::result_t cmd_erasekeychain(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2) {
        return "erasekeychain <filename> <keychain_name> - erase a keychain.";
    }

    Vault vault(params[0], false);
    if (!vault.keychainExists(params[1]))
        throw runtime_error("Keychain not found.");

    vault.eraseKeychain(params[1]);

    stringstream ss;
    ss << "Keychain " << params[1] << " erased.";
    return ss.str();
}
*/
cli::result_t cmd_renamekeychain(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 3)
        return "renamekeychain <filename> <oldname> <newname> - rename a keychain.";

    Vault vault(params[0], false);
    vault.renameKeychain(params[1], params[2]);

    stringstream ss;
    ss << "Keychain " << params[1] << " renamed to " << params[2] << ".";
    return ss.str();
}

cli::result_t cmd_keychaininfo(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2)
        return "keychaininfo <filename> <keychain_name> - display keychain information.";

    Vault vault(params[0], false);
    shared_ptr<Keychain> keychain = vault.getKeychain(params[1]);

    stringstream ss;
    ss << "id:        " << keychain->id() << endl
       << "name:      " << keychain->name() << endl
       << "depth:     " << keychain->depth() << endl
       << "parent_fp: " << keychain->parent_fp() << endl
       << "child_num: " << keychain->child_num() << endl
       << "pubkey:    " << uchar_vector(keychain->pubkey()).getHex() << endl
       << "hash:      " << uchar_vector(keychain->hash()).getHex();
    return ss.str();
}

cli::result_t cmd_listkeychains(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() < 1 || params.size() > 2)
        return "listkeychains <filename> [root_only = false] - display list of keychains.";

    bool root_only = params.size() > 1 ? (params[1] == "true") : false;

    Vault vault(params[0], false);
    vector<shared_ptr<Keychain>> keychains = vault.getAllKeychains(root_only);

    stringstream ss;
    bool newLine = false;
    for (auto& keychain: keychains)
    {
        if (newLine)    ss << endl;
        else            newLine = true;

        ss << "name: " << left << setw(20) << keychain->name() << " | id: " << left << setw(5) << keychain->id() << " | hash: " << uchar_vector(keychain->hash()).getHex();
    }
    return ss.str();
}

// Account operations
cli::result_t cmd_accountexists(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2)
        return "accountexists <filename> <account_name> - check if an account exists.";

    Vault vault(params[0], false);
    bool bExists = vault.accountExists(params[1]);

    stringstream ss;
    ss << (bExists ? "true" : "false");
    return ss.str();
}

cli::result_t cmd_newaccount(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() < 4)
        return "newaccount <filename> <account_name> <minsigs> <keychain1> [keychain2] [keychain3] ... - create a new account using specified keychains.";

    uint32_t minsigs = strtoull(params[2].c_str(), NULL, 10);
    size_t keychain_count = params.size() - 3;
    if (keychain_count > 15) throw std::runtime_error("Maximum number of keychains supported is 15.");
    if (minsigs > keychain_count) throw std::runtime_error("Minimum signatures cannot exceed number of keychains.");

    std::vector<std::string> keychain_names;
    for (size_t i = 3; i < params.size(); i++)
        keychain_names.push_back(params[i]);

    Vault vault(params[0], false);
    vault.newAccount(params[1], minsigs, keychain_names);

    stringstream ss;
    ss << "Added account " << params[1] << " to vault " << params[0] << ".";
    return ss.str();
}

cli::result_t cmd_renameaccount(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 3)
        return "renameaccount <filename> <old_account_name> <new_account_name> - rename an account.";

    Vault vault(params[0], false);
    vault.renameAccount(params[1], params[2]);

    stringstream ss;
    ss << "Renamed account " << params[1] << " to " << params[2] << ".";
    return ss.str();
}

cli::result_t cmd_accountinfo(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2)
        return "accountinfo <filename> <account_name> - display account info.";

    Vault vault(params[0], false);
    AccountInfo accountInfo = vault.getAccountInfo(params[1]);

    using namespace stdutils;
    stringstream ss;
    ss << "id:               " << accountInfo.id() << endl
       << "name:             " << accountInfo.name() << endl
       << "minsigs:          " << accountInfo.minsigs() << endl
       << "keychains:        " << delimited_list(accountInfo.keychain_names(), ", ") << endl
       << "unused_pool_size: " << accountInfo.unused_pool_size() << endl
       << "time_created:     " << accountInfo.time_created() << endl
       << "bins:             " << delimited_list(accountInfo.bin_names(), ", ");
    return ss.str();
}

cli::result_t cmd_listaccounts(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 1)
        return "listaccounts <filename> - display list of accounts.";

    Vault vault(params[0], false);
    vector<AccountInfo> accounts = vault.getAllAccountInfo();

    using namespace stdutils;
    stringstream ss;
    bool newLine = false;
    for (auto& account: accounts)
    {
        if (newLine)    ss << endl;
        else            newLine = true;

        ss << "name: " << left << setw(20) << account.name() << " | id: " << left << setw(5) << account.id() << " | policy: " << account.minsigs() << " of " << delimited_list(account.keychain_names(), ", ");
    }
    return ss.str();
}

cli::result_t cmd_newaccountbin(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 3)
        return "newaccountbin <filename> <account_name> <bin_name> - add new account bin.";

    Vault vault(params[0], false);
    vault.addAccountBin(params[1], params[2]);

    stringstream ss;
    ss << "Account bin " << params[2] << " added to account " << params[1] << ".";
    return ss.str(); 
}

// TODO: Get from config file
const unsigned char PAY_TO_SCRIPT_HASH_VERSION = 0x05;

cli::result_t cmd_newscript(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() < 2 || params.size() > 4)
        return "newscript <filename> <account_name> [bin_name = @default] [label = null] - get a new signing script.";

    Vault vault(params[0], false);
    std::string bin_name = params.size() > 2 ? params[2] : std::string("@default");
    std::string label = params.size() > 3 ? params[3] : std::string("");
    std::shared_ptr<SigningScript> script = vault.newSigningScript(params[1], bin_name, label);

    using namespace CoinQ::Script;
    std::string address;
    payee_t payee = getScriptPubKeyPayee(script->txoutscript());
    if (payee.first == SCRIPT_PUBKEY_PAY_TO_SCRIPT_HASH)
        address = toBase58Check(payee.second, PAY_TO_SCRIPT_HASH_VERSION);
    else
        address = "N/A";

    stringstream ss;
    ss << "account:     " << params[1] << endl
       << "account bin: " << bin_name << endl
       << "label:       " << label << endl
       << "script:      " << uchar_vector(script->txoutscript()).getHex() << endl
       << "address:     " << address;
    return ss.str(); 
}

cli::result_t cmd_listscripts(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() < 1 || params.size() > 4)
        return "listscripts <filename> [account_name = @all] [bin_name = @all] [flags = PENDING | RECEIVED] - display list of signing script. (flags: UNUSED=1, CHANGE=2, PENDING=4, RECEIVED=8, CANCELED=16)";

    std::string account_name;
    if (params.size() > 1) account_name = params[1];
    if (account_name == "@all") account_name = "";

    std::string bin_name;
    if (params.size() > 2) bin_name = params[2];
    if (bin_name == "@all") bin_name = "";

    int flags = params.size() > 3 ? (int)strtoul(params[3].c_str(), NULL, 0) : ((int)SigningScript::PENDING | (int)SigningScript::RECEIVED);
    
    Vault vault(params[0], false);
    vector<SigningScriptView> scriptViews = vault.getSigningScriptViews(account_name, bin_name, flags);

    using namespace CoinQ::Script;
    stringstream ss;
    bool newLine = false;
    for (auto& scriptView: scriptViews)
    {
        if (newLine)    ss << endl;
        else            newLine = true;

        std::string address;
        payee_t payee = getScriptPubKeyPayee(scriptView.txoutscript);
        if (payee.first == SCRIPT_PUBKEY_PAY_TO_SCRIPT_HASH)
            address = toBase58Check(payee.second, PAY_TO_SCRIPT_HASH_VERSION);
        else
            address = "N/A";

        ss << "account: " << left << setw(15) << scriptView.account_name << " | bin: " << left << setw(15) << scriptView.account_bin_name << " | script: " << left << setw(50) << uchar_vector(scriptView.txoutscript).getHex() << " | address: " << left << setw(36) << address << " | status: " << SigningScript::getStatusString(scriptView.status);
    }
    return ss.str();
}

cli::result_t cmd_insertrawtx(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2)
        return "insertrawtx <filename> <raw tx hex> - inserts a raw transaction into vault.";

    Vault vault(params[0], false);

    std::shared_ptr<Tx> tx(new Tx());
    tx->set(uchar_vector(params[1]));
    tx = vault.insertTx(tx);

    stringstream ss;
    if (tx)
    {
        ss << "Tx inserted. unsigned hash: " << uchar_vector(tx->unsigned_hash()).getHex();
        if (tx->status() != Tx::UNSIGNED)
            ss << " hash: " << uchar_vector(tx->hash()).getHex();
    }
    else
    {
        ss << "Tx not inserted.";
    }
    return ss.str();
}

int main(int argc, char* argv[])
{
    INIT_LOGGER("debug.log");

    cli::command_map cmds("CoinDB by Eric Lombrozo v0.2.0");

    // Vault operations
    cmds.add("create", &cmd_create);

    // Keychain operations
    cmds.add("keychainexists", &cmd_keychainexists);
    cmds.add("newkeychain", &cmd_newkeychain);
    //cmds.add("erasekeychain", &cmd_erasekeychain);
    cmds.add("renamekeychain", &cmd_renamekeychain);
    cmds.add("keychaininfo", &cmd_keychaininfo);
    cmds.add("listkeychains", &cmd_listkeychains);

    // Account operations
    cmds.add("accountexists", &cmd_accountexists);
    cmds.add("newaccount", &cmd_newaccount);
    cmds.add("renameaccount", &cmd_renameaccount);
    cmds.add("accountinfo", &cmd_accountinfo);
    cmds.add("listaccounts", &cmd_listaccounts);
    cmds.add("newaccountbin", &cmd_newaccountbin);
    cmds.add("newscript", &cmd_newscript);
    cmds.add("listscripts", &cmd_listscripts);

    // Tx operations
    cmds.add("insertrawtx", &cmd_insertrawtx);

    try 
    {
        return cmds.exec(argc, argv);
    }
    catch (const std::exception& e)
    {
        cerr << e.what() << endl;
        return 1;
    }
    return 0;
}

