///////////////////////////////////////////////////////////////////////////////
//
// coindb.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.
//

#include <cli.hpp>

#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <Vault.h>
#include <Schema-odb.hxx>

#include <random.h>

#include <iostream>
#include <sstream>

using namespace std;
using namespace odb::core;
using namespace CoinDB;

// Vault operations
cli::result_t cmd_create(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 1) {
        return "create <filename> - create a new vault.";
    }

    Vault vault(params[0], true);

    stringstream ss;
    ss << "Vault " << params[0] << " created.";
    return ss.str();
}

// Keychain operations
cli::result_t cmd_keychainexists(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2) {
        return "keychainexists <filename> <keychain_name> - check if a keychain exists.";
    }

    Vault vault(params[0], false);
    bool bExists = vault.keychainExists(params[1]);

    stringstream ss;
    ss << (bExists ? "true" : "false");
    return ss.str();
}

cli::result_t cmd_newkeychain(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2) {
        return "newkeychain <filename> <keychain_name> - create a new keychain.";
    }

    Vault vault(params[0], false);
    vault.newKeychain(params[1], random_bytes(32));

    stringstream ss;
    ss << "Added keychain " << params[1] << " to vault " << params[0] << ".";
    return ss.str();
}

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

cli::result_t cmd_renamekeychain(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 3) {
        return "renamekeychain <filename> <oldname> <newname> - rename a keychain.";
    }

    Vault vault(params[0], false);
    vault.renameKeychain(params[1], params[2]);

    stringstream ss;
    ss << "Keychain " << params[1] << " renamed to " << params[2] << ".";
    return ss.str();
}

cli::result_t cmd_keychaininfo(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2) {
        return "keychaininfo <filename> <keychain_name> - display keychain information.";
    }

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

/*
cli::result_t cmd_listkeychains(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 1) {
        return "listkeychains <filename> - list all keychains in vault.";
    }

    stringstream ss;

    Vault vault(params[0], false);
    std::vector<KeychainInfo> keychains = vault.getKeychains();
    bool first = true;
    for (auto& keychain: keychains) {
        if (first) {
            first = false;
        }
        else {
            ss << endl;
        }
        ss << "id: " << keychain.id() << " name: " << keychain.name() << " hash: " << uchar_vector(keychain.hash()).getHex() << " numkeys: " << keychain.numkeys();
    }

    return ss.str();
}
*/

// Account operations
cli::result_t cmd_accountexists(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2) {
        return "accountexists <filename> <account_name> - check if an account exists.";
    }

    Vault vault(params[0], false);
    bool bExists = vault.accountExists(params[1]);

    stringstream ss;
    ss << (bExists ? "true" : "false");
    return ss.str();
}

cli::result_t cmd_newaccount(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() < 4) {
        return "newaccount <filename> <account_name> <minsigs> <keychain1> [keychain2] [keychain3] ... - create a new account using specified keychains.";
    }

    uint32_t minsigs = strtoull(params[2].c_str(), NULL, 10);
    std::vector<std::string> keychain_names;
    for (size_t i = 3; i < params.size(); i++)
        keychain_names.push_back(params[i]);

    Vault vault(params[0], false);
    vault.newAccount(params[1], minsigs, keychain_names);

    stringstream ss;
    ss << "Added account " << params[1] << " to vault " << params[0] << ".";
    return ss.str();
}

int main(int argc, char* argv[])
{
    cli::command_map cmds("CoinDB by Eric Lombrozo v0.2.0");

    // Vault operations
    cmds.add("create", &cmd_create);

    // Keychain operations
    cmds.add("keychainexists", &cmd_keychainexists);
    cmds.add("newkeychain", &cmd_newkeychain);
    cmds.add("erasekeychain", &cmd_erasekeychain);
    cmds.add("renamekeychain", &cmd_renamekeychain);
    cmds.add("keychaininfo", &cmd_keychaininfo);

    // Account operations
    cmds.add("accountexists", &cmd_accountexists);
    cmds.add("newaccount", &cmd_newaccount);

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

