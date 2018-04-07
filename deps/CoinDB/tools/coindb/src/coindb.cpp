///////////////////////////////////////////////////////////////////////////////
//
// coindb.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "formatting.h"

#include <CoinDBConfig.h>

#include <cli.hpp>

#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <Vault.h>
#include <Passphrase.h>

#include <CoinCore/Base58Check.h>
#include <CoinCore/random.h>
#include <CoinQ/CoinQ_coinparams.h>

#include <logger/logger.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <ctime>
#include <functional>

#include <boost/algorithm/string.hpp>

const std::string COINDB_VERSION = "v0.8.0";
const std::string DEFAULT_NETWORK = "bitcoin";
const bool USE_WITNESS_P2SH = true;

using namespace std;
using namespace odb::core;
using namespace CoinDB;

std::string g_dbuser;
std::string g_dbpasswd;

// Global operations
cli::result_t cmd_create(const cli::params_t& params)
{
    string network = params.size() > 1 ? params[1] : DEFAULT_NETWORK;

    Vault vault(g_dbuser, g_dbpasswd, params[0], true, SCHEMA_VERSION, network);

    stringstream ss;
    ss << "Vault " << params[0] << " created.";
    return ss.str();
}

cli::result_t cmd_info(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    string network = vault.getNetwork();
    uint32_t schema_version = vault.getSchemaVersion();
    uint32_t horizon_timestamp = vault.getHorizonTimestamp();

    stringstream ss;
    ss << "db file:             " << params[0] << endl
       << "network:             " << network << endl
       << "schema version:      " << schema_version << endl
       << "horizon timestamp:   " << horizon_timestamp; 
    return ss.str();
}

cli::result_t cmd_migrate(const cli::params_t& params)
{
    Vault vault;
    try
    {
        vault.open(g_dbuser, g_dbpasswd, params[0], false, SCHEMA_VERSION, string(), false);
    }
    catch (const VaultNeedsSchemaMigrationException& e)
    {
        vault.open(g_dbuser, g_dbpasswd, params[0], false, SCHEMA_VERSION, string(), true);
        stringstream ss;
        ss << "Vault " << params[0] << " migrated from schema " << e.schema_version() << " to schema " << e.current_version() << ".";
        return ss.str();
    }

    return "Schema is already current.";
}

cli::result_t cmd_exportvault(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);

    bool exportprivkeys = params.size() <= 1 || params[1] == "true";

    std::string output_file = params.size() > 2 ? params[2] : (params[0] + ".portable");
    vault.exportVault(output_file, exportprivkeys);

    stringstream ss;
    ss << "Vault " << params[0] << " exported to " << output_file << ".";
    return ss.str();
}

cli::result_t cmd_importvault(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], true);

    bool importprivkeys = params.size() <= 2 || params[2] == "true";

    vault.importVault(params[1], importprivkeys);

    stringstream ss;
    ss << "Vault " << params[0] << " imported from " << params[1] << ".";
    return ss.str();
}

// Contact operations
cli::result_t cmd_contactinfo(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);

    shared_ptr<Contact> contact = vault.getContact(params[1]);
    stringstream ss;
    ss << "Contact Information" << endl
       << "===================" << endl
       << "  username: " << contact->username();
    return ss.str(); 
}

cli::result_t cmd_newcontact(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);

    shared_ptr<Contact> contact = vault.newContact(params[1]);
    stringstream ss;
    ss << "Added contact " << contact->username() << ".";
    return ss.str(); 
}

cli::result_t cmd_renamecontact(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);

    shared_ptr<Contact> contact = vault.renameContact(params[1], params[2]);
    stringstream ss;
    ss << "Renamed contact " << params[1] << " to " << contact->username() << ".";
    return ss.str(); 
}

cli::result_t cmd_listcontacts(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);

    ContactVector contacts = vault.getAllContacts();

    // TODO: format output
    stringstream ss;
    ss << "Contacts" << endl
       << "========";

    for (auto& contact: contacts)
    {
        ss << endl << contact->username();
    }

    return ss.str();
}

// Keychain operations
cli::result_t cmd_keychainexists(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    bool bExists = vault.keychainExists(params[1]);

    stringstream ss;
    ss << (bExists ? "true" : "false");
    return ss.str();
}

cli::result_t cmd_newkeychain(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);

    secure_bytes_t lock_key;
    if (params.size() > 2) { lock_key = passphraseHash(params[2]); }
    vault.newKeychain(params[1], secure_random_bytes(32), lock_key);

    stringstream ss;
    ss << "Added keychain " << params[1] << " to vault " << params[0] << ".";
    return ss.str();
}
/*
cli::result_t cmd_erasekeychain(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2) {
        return "erasekeychain <db file> <keychain_name> - erase a keychain.";
    }

    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    if (!vault.keychainExists(params[1]))
        throw runtime_error("Keychain not found.");

    vault.eraseKeychain(params[1]);

    stringstream ss;
    ss << "Keychain " << params[1] << " erased.";
    return ss.str();
}
*/
cli::result_t cmd_renamekeychain(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    vault.renameKeychain(params[1], params[2]);

    stringstream ss;
    ss << "Keychain " << params[1] << " renamed to " << params[2] << ".";
    return ss.str();
}

cli::result_t cmd_encryptkeychain(const cli::params_t& params)
{
    if (params[2].empty()) throw runtime_error("Passphrase is empty.");

    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    shared_ptr<Keychain> keychain = vault.getKeychain(params[1]);
    if (!keychain->isPrivate()) throw runtime_error("Keychain is nonprivate.");
    if (keychain->isEncrypted()) throw runtime_error("Keychain is already encrypted.");

    vault.unlockKeychain(keychain->name());
    vault.encryptKeychain(keychain->name(), passphraseHash(params[2]));

    stringstream ss;
    ss << "Keychain " << keychain->name() << " encrypted.";
    return ss.str();
}

cli::result_t cmd_decryptkeychain(const cli::params_t& params)
{
    if (params[2].empty()) throw runtime_error("Passphrase is empty.");

    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    shared_ptr<Keychain> keychain = vault.getKeychain(params[1]);
    if (!keychain->isPrivate()) throw runtime_error("Keychain is nonprivate.");
    if (!keychain->isEncrypted()) throw runtime_error("Keychain is not encrypted.");

    vault.unlockKeychain(keychain->name(), passphraseHash(params[2]));
    vault.decryptKeychain(keychain->name());

    stringstream ss;
    ss << "Keychain " << keychain->name() << " decrypted.";
    return ss.str();
}

cli::result_t cmd_reencryptkeychain(const cli::params_t& params)
{
    if (params[2].empty()) throw runtime_error("Old passphrase is empty.");
    if (params[3].empty()) throw runtime_error("New passphrase is empty.");

    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    shared_ptr<Keychain> keychain = vault.getKeychain(params[1]);
    if (!keychain->isPrivate()) throw runtime_error("Keychain is nonprivate.");
    if (!keychain->isEncrypted()) throw runtime_error("Keychain is not encrypted.");

    vault.unlockKeychain(keychain->name(), passphraseHash(params[2]));
    vault.encryptKeychain(keychain->name(), passphraseHash(params[3]));

    stringstream ss;
    ss << "Keychain " << keychain->name() << " reencrypted.";
    return ss.str();
}

cli::result_t cmd_keychaininfo(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
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

cli::result_t cmd_keychains(const cli::params_t& params)
{
    std::string account_name;
    if (params.size() > 1 && params[1] != "@all") account_name = params[1];

    bool show_hidden = params.size() > 2 && params[2] == "true";

    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    vector<KeychainView> views = vault.getRootKeychainViews(account_name, show_hidden);

    stringstream ss;
    ss << formattedKeychainViewHeader();
    for (auto& view: views)
        ss << endl << formattedKeychainView(view);
    return ss.str();
}
/*
cli::result_t cmd_listkeychains(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() < 1 || params.size() > 2)
        return "listkeychains <db file> [root_only = false] - display list of keychains.";

    bool root_only = params.size() > 1 ? (params[1] == "true") : false;

    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    vector<shared_ptr<Keychain>> keychains = vault.getAllKeychains(root_only);

    stringstream ss;
    ss << formattedKeychainHeader();
    for (auto& keychain: keychains)
        ss << endl << formattedKeychain(keychain);
    return ss.str();
}
*/

cli::result_t cmd_exportkeychain(const cli::params_t& params)
{
    bool export_privkey = params.size() > 2 ? (params[2] == "true") : false;

    std::string output_file;
    if (params.size() > 3)  { output_file = params[3]; }
    else                    { output_file = params[1] + (export_privkey ? ".priv" : ".pub"); }

    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    vault.exportKeychain(params[1], output_file, export_privkey);

    stringstream ss;
    ss << (export_privkey ? "Private" : "Public") << " keychain " << params[1] << " exported to " << output_file << ".";
    return ss.str();
}

cli::result_t cmd_importkeychain(const cli::params_t& params)
{
    bool import_privkey = params.size() > 2 ? (params[2] == "true") : true;

    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    std::shared_ptr<Keychain> keychain = vault.importKeychain(params[1], import_privkey);

    stringstream ss;
    ss << (import_privkey ? "Private" : "Public") << " keychain " << keychain->name() << " imported from " << params[1] << ".";
    return ss.str();
}

cli::result_t cmd_exportbip32(const cli::params_t& params)
{
    bool export_privkey = params.size() > 2;

    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    if (export_privkey)
    {
        secure_bytes_t unlock_key = sha256_2(params[2]);
        vault.unlockKeychain(params[1], unlock_key);
    }
    secure_bytes_t extkey = vault.exportBIP32(params[1], export_privkey);

    stringstream ss;
    ss << toBase58Check(extkey);
    return ss.str();
}

cli::result_t cmd_importbip32(const cli::params_t& params)
{
    bool import_privkey = params.size() > 3;
    secure_bytes_t lock_key;
    bytes_t salt;
    if (import_privkey)
    {
        lock_key = sha256_2(params[3]);
        // TODO: add salt
    }

    secure_bytes_t extkey;
    if (!fromBase58Check(params[2], extkey)) throw std::runtime_error("Invalid BIP32.");

    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    std::shared_ptr<Keychain> keychain = vault.importBIP32(params[1], extkey, lock_key);

    stringstream ss;
    ss << (keychain->isPrivate() ? "Private" : "Public") << " keychain " << keychain->name() << " imported from BIP32.";
    return ss.str();
}

// Account operations
cli::result_t cmd_accountexists(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    bool bExists = vault.accountExists(params[1]);

    stringstream ss;
    ss << (bExists ? "true" : "false");
    return ss.str();
}

cli::result_t cmd_newaccount(const cli::params_t& params)
{
    uint32_t minsigs = strtoull(params[2].c_str(), NULL, 10);
    size_t keychain_count = params.size() - 3;
    if (keychain_count > 15) throw std::runtime_error("Maximum number of keychains supported is 15.");
    if (minsigs > keychain_count) throw std::runtime_error("Minimum signatures cannot exceed number of keychains.");

    std::vector<std::string> keychain_names;
    for (size_t i = 3; i < params.size(); i++)
        keychain_names.push_back(params[i]);

    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    CoinQ::NetworkSelector networkSelector(vault.getNetwork());
    const CoinQ::CoinParams& coinParams = networkSelector.getCoinParams();
    vault.newAccount(coinParams.segwit_enabled(), USE_WITNESS_P2SH, params[1], minsigs, keychain_names, 25, time(NULL));

    stringstream ss;
    ss << "Added account " << params[1] << " to vault " << params[0] << ".";
    return ss.str();
}

cli::result_t cmd_newuncompressedaccount(const cli::params_t& params)
{
    uint32_t minsigs = strtoull(params[2].c_str(), NULL, 10);
    size_t keychain_count = params.size() - 3;
    if (keychain_count > 15) throw std::runtime_error("Maximum number of keychains supported is 15.");
    if (minsigs > keychain_count) throw std::runtime_error("Minimum signatures cannot exceed number of keychains.");

    std::vector<std::string> keychain_names;
    for (size_t i = 3; i < params.size(); i++)
        keychain_names.push_back(params[i]);

    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    CoinQ::NetworkSelector networkSelector(vault.getNetwork());
    const CoinQ::CoinParams& coinParams = networkSelector.getCoinParams();
    vault.newAccount(coinParams.segwit_enabled(), USE_WITNESS_P2SH, params[1], minsigs, keychain_names, 25, time(NULL), false);

    stringstream ss;
    ss << "Added account " << params[1] << " to vault " << params[0] << ".";
    return ss.str();
}

cli::result_t cmd_renameaccount(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    vault.renameAccount(params[1], params[2]);

    stringstream ss;
    ss << "Renamed account " << params[1] << " to " << params[2] << ".";
    return ss.str();
}

cli::result_t cmd_accountinfo(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    AccountInfo accountInfo = vault.getAccountInfo(params[1]);
    uint64_t balance = vault.getAccountBalance(params[1], 0);
    uint64_t confirmed_balance = vault.getAccountBalance(params[1], 1);

    using namespace stdutils;
    stringstream ss;
    ss << "id:                  " << accountInfo.id() << endl
       << "name:                " << accountInfo.name() << endl
       << "minsigs:             " << accountInfo.minsigs() << endl
       << "keychains:           " << delimited_list(accountInfo.keychain_names(), ", ") << endl
       << "issued_script_count: " << accountInfo.issued_script_count() << endl
       << "unused_pool_size:    " << accountInfo.unused_pool_size() << endl
       << "time_created:        " << accountInfo.time_created() << endl
       << "bins:                " << delimited_list(accountInfo.bin_names(), ", ") << endl
       << "balance:             " << balance << endl
       << "confirmed balance:   " << confirmed_balance;
    return ss.str();
}

cli::result_t cmd_listaccounts(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    vector<AccountInfo> accounts = vault.getAllAccountInfo();

    stringstream ss;
    ss << formattedAccountHeader();
    for (auto& account: accounts)
        ss << endl << formattedAccount(account);
    return ss.str();
}

cli::result_t cmd_exportaccount(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);

    std::string output_file = params.size() > 2 ? params[2] : (params[1] + ".acct");
    vault.exportAccount(params[1], output_file, true);

    stringstream ss;
    ss << "Account " << params[1] << " exported to " << output_file << ".";
    return ss.str();
}

cli::result_t cmd_exportsharedaccount(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);

    std::string output_file = params.size() > 2 ? params[2] : (params[1] + ".sharedacct");
    vault.exportAccount(params[1], output_file, false);

    stringstream ss;
    ss << "Account " << params[1] << " exported to " << output_file << ".";
    return ss.str();
}

cli::result_t cmd_importaccount(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);

    unsigned int privkeycount = 1;

    std::shared_ptr<Account> account = vault.importAccount(params[1], privkeycount);

    stringstream ss;
    ss << "Account " << account->name() << " imported from " << params[1] << ".";
    return ss.str();
}

cli::result_t cmd_newaccountbin(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    AccountInfo accountInfo = vault.getAccountInfo(params[1]);
    vault.addAccountBin(params[1], params[2]);

    stringstream ss;
    ss << "Account bin " << params[2] << " added to account " << params[1] << ".";
    return ss.str(); 
}

cli::result_t cmd_listbins(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    vector<AccountBinView> bins = vault.getAllAccountBinViews();

    stringstream ss;
    ss << formattedAccountBinViewHeader();
    for (auto& bin: bins)
        ss << endl << formattedAccountBinView(bin);
    return ss.str();
}

cli::result_t cmd_issuescript(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    CoinQ::NetworkSelector networkSelector(vault.getNetwork());
    const CoinQ::CoinParams& coinParams = networkSelector.getCoinParams();

    std::string account_name;
    if (params[1] != "@null") account_name = params[1];
    std::string label = params.size() > 2 ? params[2] : std::string("");
    std::string bin_name = params.size() > 3 ? params[3] : std::string(DEFAULT_BIN_NAME);
    uint32_t index = params.size() > 4 ? strtoull(params[4].c_str(), NULL, 10) : 0;
    std::shared_ptr<SigningScript> script = vault.issueSigningScript(account_name, bin_name, label, index);

    std::string address = CoinQ::Script::getAddressForTxOutScript(script->txoutscript(), coinParams.address_versions());

    stringstream ss;
    ss << "account:     " << params[1] << endl
       << "account bin: " << bin_name << endl
       << "index:       " << script->index() << endl
       << "label:       " << label << endl
       << "script:      " << uchar_vector(script->txoutscript()).getHex() << endl
       << "address:     " << address;
    return ss.str(); 
}

cli::result_t cmd_invoicecontact(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    CoinQ::NetworkSelector networkSelector(vault.getNetwork());
    const CoinQ::CoinParams& coinParams = networkSelector.getCoinParams();

    std::string account_name;
    if (params[1] != "@null") account_name = params[1];
    std::string username = params[2];
    std::string label = params.size() > 3 ? params[3] : std::string("");
    std::string bin_name = params.size() > 4 ? params[4] : std::string(DEFAULT_BIN_NAME);
    uint32_t index = params.size() > 5 ? strtoull(params[5].c_str(), NULL, 10) : 0;
    std::shared_ptr<SigningScript> script = vault.issueSigningScript(account_name, bin_name, label, index, username);

    std::string address = CoinQ::Script::getAddressForTxOutScript(script->txoutscript(), coinParams.address_versions());

    stringstream ss;
    ss << "account:     " << params[1] << endl
       << "account bin: " << bin_name << endl
       << "index:       " << script->index() << endl
       << "label:       " << label << endl
       << "script:      " << uchar_vector(script->txoutscript()).getHex() << endl
       << "address:     " << address << endl
       << "username:    " << username;
    return ss.str(); 
}

cli::result_t cmd_listscripts(const cli::params_t& params)
{
    std::string account_name = params.size() > 1 ? params[1] : std::string("@all");
    if (account_name == "@all") account_name = "";

    std::string bin_name = params.size() > 2 ? params[2] : std::string("@all");
    if (bin_name == "@all") bin_name = "";

    int flags = params.size() > 3 ? (int)strtoul(params[3].c_str(), NULL, 0) : ((int)SigningScript::ISSUED | (int)SigningScript::USED);
    
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    CoinQ::NetworkSelector networkSelector(vault.getNetwork());
    const CoinQ::CoinParams& coinParams = networkSelector.getCoinParams();

    vector<SigningScriptView> scriptViews = vault.getSigningScriptViews(account_name, bin_name, flags);

    stringstream ss;
    ss << formattedScriptHeader();
    for (auto& scriptView: scriptViews)
        ss << endl << formattedScript(scriptView, coinParams);
    return ss.str();
}

cli::result_t cmd_history(const cli::params_t& params)
{
    std::string account_name = params.size() > 1 ? params[1] : std::string("@all");
    if (account_name == "@all") account_name = "";

    std::string bin_name = params.size() > 2 ? params[2] : std::string("@all");
    if (bin_name == "@all") bin_name = "";

    bool hide_change = params.size() > 3 ? params[3] == "true" : true;
    
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    CoinQ::NetworkSelector networkSelector(vault.getNetwork());
    const CoinQ::CoinParams& coinParams = networkSelector.getCoinParams();

    uint32_t best_height = vault.getBestHeight();
    vector<TxOutView> txOutViews = vault.getTxOutViews(account_name, bin_name, TxOut::ROLE_BOTH, TxOut::BOTH, Tx::ALL, hide_change);
    stringstream ss;
    ss << formattedTxOutViewHeader();
    for (auto& txOutView: txOutViews)
        ss << endl << formattedTxOutView(txOutView, best_height, coinParams);
    return ss.str();
}

cli::result_t cmd_historycsv(const cli::params_t& params)
{
    std::string account_name = params.size() > 1 ? params[1] : std::string("@all");
    if (account_name == "@all") account_name = "";

    std::string bin_name = params.size() > 2 ? params[2] : std::string("@all");
    if (bin_name == "@all") bin_name = "";

    bool hide_change = params.size() > 3 ? params[3] == "true" : true;
    
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    CoinQ::NetworkSelector networkSelector(vault.getNetwork());
    const CoinQ::CoinParams& coinParams = networkSelector.getCoinParams();

    uint32_t best_height = vault.getBestHeight();
    vector<TxOutView> txOutViews = vault.getTxOutViews(account_name, bin_name, TxOut::ROLE_BOTH, TxOut::BOTH, Tx::ALL, hide_change);
    stringstream ss;
    bool bNewLine = false;
    for (auto& txOutView: txOutViews)
    {
        if (bNewLine)   { ss << endl; }
        else            { bNewLine = true; }
        ss << formattedTxOutViewCSV(txOutView, best_height, coinParams);
    }
    return ss.str();
}

cli::result_t cmd_unspent(const cli::params_t& params)
{
    std::string account_name = params[1];

    uint32_t min_confirmations = params.size() > 2 ? strtoul(params[2].c_str(), NULL, 0) : 0;

    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    CoinQ::NetworkSelector networkSelector(vault.getNetwork());
    const CoinQ::CoinParams& coinParams = networkSelector.getCoinParams();

    uint32_t best_height = vault.getBestHeight();
    vector<TxOutView> txOutViews = vault.getUnspentTxOutViews(account_name, min_confirmations);
    stringstream ss;
    ss << formattedUnspentTxOutViewHeader();
    for (auto& txOutView: txOutViews)
        ss << endl << formattedUnspentTxOutView(txOutView, best_height, coinParams);
    return ss.str();
}

cli::result_t cmd_unsigned(const cli::params_t& params)
{
    std::string account_name = params.size() > 1 ? params[1] : std::string("@all");
    if (account_name == "@all") account_name = "";

    std::string bin_name = params.size() > 2 ? params[2] : std::string("@all");
    if (bin_name == "@all") bin_name = "";

    bool hide_change = params.size() > 3 ? params[3] == "true" : true;
    
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    CoinQ::NetworkSelector networkSelector(vault.getNetwork());
    const CoinQ::CoinParams& coinParams = networkSelector.getCoinParams();

    uint32_t best_height = vault.getBestHeight();
    vector<TxOutView> txOutViews = vault.getTxOutViews(account_name, bin_name, TxOut::ROLE_BOTH, TxOut::BOTH, Tx::UNSIGNED, hide_change);
    stringstream ss;
    ss << formattedTxOutViewHeader();
    for (auto& txOutView: txOutViews)
        ss << endl << formattedTxOutView(txOutView, best_height, coinParams);
    return ss.str();
}

cli::result_t cmd_unsignedhashes(const cli::params_t& params)
{
    std::string account_name = params.size() > 1 ? params[1] : std::string("@all");
    if (account_name == "@all") account_name = "";

    std::string bin_name = params.size() > 2 ? params[2] : std::string("@all");
    if (bin_name == "@all") bin_name = "";

    bool hide_change = params.size() > 3 ? params[3] == "true" : true;
    
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    vector<TxOutView> txOutViews = vault.getTxOutViews(account_name, bin_name, TxOut::ROLE_BOTH, TxOut::BOTH, Tx::UNSIGNED, hide_change);
    stringstream ss;
    std::set<std::string> hashes;
    for (auto& txOutView: txOutViews)
    {
        if (txOutView.tx_status != Tx::UNSIGNED) continue;
        bytes_t txhash = txOutView.tx_unsigned_hash;
        hashes.insert(uchar_vector(txhash).getHex());
    }
    for (auto& hash: hashes)
    {
        ss << hash << endl;
    }
 
    return ss.str();
}

cli::result_t cmd_refillaccountpool(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    AccountInfo accountInfo = vault.getAccountInfo(params[1]);
    vault.refillAccountPool(params[1]);

    stringstream ss;
    ss << "Refilled account pool for account " << params[1] << ".";
    return ss.str();
}

// Account bin operations
cli::result_t cmd_exportbin(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);

    string export_name = params.size() > 3 ? params[3] : (params[2].empty() ? params[1] : params[1] + "-" + params[2]);
    string output_file = params.size() > 4 ? params[4] : (export_name + ".bin");
    vault.exportAccountBin(params[1], params[2], export_name, output_file);

    stringstream ss;
    ss << "Account bin " << export_name << " exported to " << output_file << ".";
    return ss.str();
}

cli::result_t cmd_importbin(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);

    secure_bytes_t importChainCodeUnlockKey;
    std::shared_ptr<AccountBin> bin = vault.importAccountBin(params[1]);

    stringstream ss;
    ss << "Account bin " << bin->name() << " imported from " << params[1] << ".";
    return ss.str();
}

// Tx operations
cli::result_t cmd_listtxs(const cli::params_t& params)
{
    int tx_status_flags = params.size() > 1 && params[1] == "unsigned" ? Tx::UNSIGNED : Tx::ALL;
    uint32_t minheight = params.size() > 2 ? strtoul(params[2].c_str(), NULL, 0) : 0;
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    uint32_t best_height = vault.getBestHeight();
    std::vector<TxView> txViews = vault.getTxViews(tx_status_flags, 0, -1, minheight);
    stringstream ss;
    ss << formattedTxViewHeader();
    for (auto& txView: txViews)
        ss << endl << formattedTxView(txView, best_height);
    return ss.str();
}

cli::result_t cmd_txinfo(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    CoinQ::NetworkSelector networkSelector(vault.getNetwork());
    const CoinQ::CoinParams& coinParams = networkSelector.getCoinParams();

    std::shared_ptr<Tx> tx;
    bytes_t hash = uchar_vector(params[1]);
    if (hash.size() == 32)
    {
        tx = vault.getTx(hash);
    }
    else
    {
        unsigned long tx_id = strtoul(params[1].c_str(), NULL, 0);
        tx = vault.getTx(tx_id);
    }

    stringstream ss;
    ss << "status:        " << Tx::getStatusString(tx->status()) << endl
       << "confirmations: " << vault.getTxConfirmations(tx) << endl
       << "hash:          " << uchar_vector(tx->hash()).getHex() << endl
       << "version:       " << tx->version() << endl
       << "locktime:      " << tx->locktime() << endl
       << "timestamp:     " << tx->timestamp();

    ss << endl << endl << formattedTxInHeader();
    for (auto& txin: tx->txins())
        ss << endl << formattedTxIn(txin);

    ss << endl << endl << formattedTxOutHeader();
    for (auto& txout: tx->txouts())
        ss << endl << formattedTxOut(txout, coinParams);
    
    return ss.str();
}

cli::result_t cmd_txconf(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    uint32_t confirmations;
    bytes_t hash = uchar_vector(params[1]);
    if (hash.size() == 32)
    {
        confirmations = vault.getTxConfirmations(hash);
    }
    else
    {
        unsigned long tx_id = strtoul(params[1].c_str(), NULL, 0);
        confirmations = vault.getTxConfirmations(tx_id);
    }

    stringstream ss;
    ss << "confirmations: " << confirmations;
    return ss.str();
}

cli::result_t cmd_rawtx(const cli::params_t& params)
{
    bool to_file = params.size() > 2 && params[2] == "true";

    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    std::shared_ptr<Tx> tx;
    bytes_t hash = uchar_vector(params[1]);
    if (hash.size() == 32)
    {
        tx = vault.getTx(hash);
    }
    else
    {
        unsigned long tx_id = strtoul(params[1].c_str(), NULL, 0);
        tx = vault.getTx(tx_id);
    }

    std::string rawhex = uchar_vector(tx->raw()).getHex();
    if (to_file)
    {
        string filename = uchar_vector(tx->hash()).getHex() + ".tx";
        ofstream ofs(filename, ofstream::out);
        ofs << rawhex << endl;
        ofs.close();

        stringstream ss;
        ss << "Exported raw transaction to " << filename;
        return ss.str();
    }
    else
    {
        return rawhex;
    }
}

cli::result_t cmd_insertrawtx(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);

    std::shared_ptr<Tx> tx(new Tx());
    string rawhex;
    if (params[1].size() > 3 && params[1].substr(params[1].size() - 3, 3) == ".tx")
    {
        ifstream ifs(params[1], ifstream::in);
        if (ifs.bad()) throw std::runtime_error("Error opening file.");
        ifs >> rawhex;
        if (!ifs.good()) throw std::runtime_error("Error reading file.");
    }
    else
    {
        rawhex = params[1];
    }
    tx->set(uchar_vector(rawhex));
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

cli::result_t cmd_newrawtx(const cli::params_t& params)
{
    using namespace CoinQ::Script;
    const size_t MAX_VERSION_LEN = 2;

    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    CoinQ::NetworkSelector networkSelector(vault.getNetwork());
    const CoinQ::CoinParams& coinParams = networkSelector.getCoinParams();

    // Get outputs
    size_t i = 2;
    txouts_t txouts;
    do
    {
        bytes_t txoutscript  = getTxOutScriptForAddress(params[i++], coinParams.address_versions());
        uint64_t value = strtoull(params[i++].c_str(), NULL, 0);
        std::shared_ptr<TxOut> txout(new TxOut(value, txoutscript));
        txouts.push_back(txout);
         
    } while (i < (params.size() - 1) && params[i].size() > MAX_VERSION_LEN);

    uint64_t fee = i < params.size() ? strtoull(params[i++].c_str(), NULL, 0) : 0;
    uint32_t version = i < params.size() ? strtoul(params[i++].c_str(), NULL, 0) : 1;
    uint32_t locktime = i < params.size() ? strtoul(params[i++].c_str(), NULL, 0) : 0;

    std::shared_ptr<Tx> tx = vault.createTx(params[1], version, locktime, txouts, fee, 1, true);
    return uchar_vector(tx->raw()).getHex();
}

cli::result_t cmd_createtx(const cli::params_t& params)
{
    using namespace CoinQ::Script;
    const size_t MAX_VERSION_LEN = 2;

    Vault vault(g_dbuser, g_dbpasswd, params[0], false);

    // Get txout ids (inputs)
    ids_t coin_ids;
    string coin_ids_param(params[2]);
    boost::trim(coin_ids_param);
    if (!coin_ids_param.empty())
    {
        vector<string> str_coin_ids;
        boost::split(str_coin_ids, coin_ids_param, boost::is_any_of(","));
        for (auto& str_coin_id: str_coin_ids)
        {
            boost::trim(str_coin_id);
            if (str_coin_id.empty() || !all_of(str_coin_id.begin(), str_coin_id.end(), ::isdigit))
                throw runtime_error("Invalid txout ids.");
            coin_ids.push_back(strtoul(str_coin_id.c_str(), NULL, 10));
        }
    }

    // Get outputs
    size_t i = 3;
    txouts_t txouts;
    do
    {
        string address(params[i++]);
        bytes_t txoutscript;
        if (address != "change") { txoutscript = getTxOutScriptForAddress(address, BASE58_VERSIONS); }
        uint64_t value = strtoull(params[i++].c_str(), NULL, 0);
        std::shared_ptr<TxOut> txout(new TxOut(value, txoutscript));
        txouts.push_back(txout);
         
    } while (i < (params.size() - 1) && params[i].size() > MAX_VERSION_LEN);

    uint64_t fee = i < params.size() ? strtoull(params[i++].c_str(), NULL, 0) : 0;
    uint64_t min_confirmations = i < params.size() ? strtoull(params[i++].c_str(), NULL, 0) : 1;
    uint32_t version = i < params.size() ? strtoul(params[i++].c_str(), NULL, 0) : 1;
    uint32_t locktime = i < params.size() ? strtoul(params[i++].c_str(), NULL, 0) : 0;

    std::shared_ptr<Tx> tx = vault.createTx(params[1], version, locktime, coin_ids, txouts, fee, min_confirmations, true);
    return tx->toJson();
}

cli::result_t cmd_deletetx(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);

    bytes_t hash = uchar_vector(params[1]);
    std::shared_ptr<Tx> tx;
    if (hash.size() == 32)
    {
        vault.deleteTx(hash);
    }
    else
    {
        unsigned long tx_id = strtoul(params[1].c_str(), NULL, 0);
        vault.deleteTx(tx_id);
    }

    stringstream ss;
    ss << "Tx deleted.";
    return ss.str();
}

cli::result_t cmd_consolidate(const cli::params_t& params)
{
    using namespace CoinQ::Script;

    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    CoinQ::NetworkSelector networkSelector(vault.getNetwork());
    const CoinQ::CoinParams& coinParams = networkSelector.getCoinParams();

    string account_name(params[1]);
    uint32_t max_tx_size = strtoul(params[2].c_str(), NULL, 0);
    bytes_t txoutscript = getTxOutScriptForAddress(params[3], coinParams.address_versions());
    uint64_t min_fee = params.size() > 4 ? strtoull(params[4].c_str(), NULL, 0) : 0;
    uint32_t min_confirmations = params.size() > 5 ? strtoul(params[5].c_str(), NULL, 0) : 1;
    uint32_t tx_version = params.size() > 6 ? strtoul(params[6].c_str(), NULL, 0) : 1;
    uint32_t tx_locktime = params.size() > 7 ? strtoul(params[7].c_str(), NULL, 0) : 0;

    txs_t txs = vault.consolidateTxOuts(account_name, max_tx_size, tx_version, tx_locktime, ids_t(), txoutscript, min_fee, min_confirmations, false);

    stringstream ss;
    for (auto& tx: txs) { ss << uchar_vector(tx->raw()).getHex() << endl; }
    return ss.str();
} 

cli::result_t cmd_signingrequest(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);

    SigningRequest req;
    bytes_t hash = uchar_vector(params[1]);
    if (hash.size() == 32)
    {
        req = vault.getSigningRequest(hash, true);
    }
    else
    {
        unsigned long tx_id = strtoul(params[1].c_str(), NULL, 0);
        req = vault.getSigningRequest(tx_id, true);
    }

    vector<string> keychain_names;
    vector<string> keychain_hashes;
    for (auto& keychain_pair: req.keychain_info())
    {
        keychain_names.push_back(keychain_pair.first);
        keychain_hashes.push_back(uchar_vector(keychain_pair.second).getHex());
    }
    string hash_str = uchar_vector(req.hash()).getHex();
    string rawtx_str = uchar_vector(req.rawtx()).getHex();

    stringstream ss;
    ss << "Signing Request" << endl
       << "===============" << endl
       << "  hash:              " << hash_str << endl
       << "  signatures needed: " << req.sigs_needed() << endl
       << "  keychain names:    " << stdutils::delimited_list(keychain_names, ", ") << endl
       << "  keychain hashes:   " << stdutils::delimited_list(keychain_hashes, ", ") << endl
       << "  raw tx:            " << rawtx_str;
    return ss.str();
}

cli::result_t cmd_signtx(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);

    secure_bytes_t lock_key;
    if (params.size() > 3) { lock_key = passphraseHash(params[3]); }
    vault.unlockKeychain(params[2], lock_key);

    stringstream ss;
    std::vector<std::string> keychain_names;
    keychain_names.push_back(params[2]);

    bytes_t unsigned_hash = uchar_vector(params[1]);
    std::shared_ptr<Tx> tx;
    if (unsigned_hash.size() == 32)
    {
        tx = vault.signTx(unsigned_hash, keychain_names, true);
    }
    else
    {
        unsigned long tx_id = strtoul(params[1].c_str(), NULL, 0);
        tx = vault.signTx(tx_id, keychain_names, true);
    }

    if (!keychain_names.empty())
    {
        ss << "Signatures added.";
    }
    else
    {
        ss << "No signatures were added.";
    }
    return ss.str();
}

cli::result_t cmd_exporttxs(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);

    uint32_t minheight = params.size() > 1 ? strtoul(params[1].c_str(), NULL, 0) : 0;
    std::string output_file = params.size() > 2 ? params[2] : (params[0] + ".txs");
    vault.exportTxs(output_file, minheight);

    stringstream ss;
    ss << "Transactions exported to " << output_file << ".";
    return ss.str();
}

cli::result_t cmd_importtxs(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);

    vault.importTxs(params[1]);

    stringstream ss;
    ss << "Transactions imported from " << params[1] << ".";
    return ss.str();
}


// Blockchain operations
cli::result_t cmd_bestheight(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    uint32_t best_height = vault.getBestHeight();

    stringstream ss;
    ss << best_height;
    return ss.str();
}

cli::result_t cmd_horizonheight(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    uint32_t horizon_height = vault.getHorizonHeight();

    stringstream ss;
    ss << horizon_height;
    return ss.str();
}

cli::result_t cmd_horizontimestamp(const cli::params_t& params)
{
    bool use_gmt = params.size() > 1 && params[1] == "true";

    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    long timestamp = vault.getHorizonTimestamp();

    std::function<struct tm*(const time_t*)> fConvert = use_gmt ? &gmtime : &localtime;
    string formatted_timestamp = asctime(fConvert((const time_t*)&timestamp));
    formatted_timestamp = formatted_timestamp.substr(0, formatted_timestamp.size() - 1);

    stringstream ss;
    ss << timestamp << " (" << formatted_timestamp << " " << (use_gmt ? "GMT" : "LOCAL") << ")";
    return ss.str();
}

cli::result_t cmd_blockinfo(const cli::params_t& params)
{
    uint32_t height = strtoul(params[1].c_str(), NULL, 0);

    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    std::shared_ptr<BlockHeader> blockheader = vault.getBlockHeader(height);

    return blockheader->toCoinCore().toIndentedString();
}

cli::result_t cmd_rawblockheader(const cli::params_t& params)
{
    uint32_t version = strtoull(params[0].c_str(), NULL, 0);
    uchar_vector prevblockhash(params[1]);
    uchar_vector merkleroot(params[2]);
    uint32_t timestamp = strtoull(params[3].c_str(), NULL, 0);
    uint32_t bits = strtoull(params[4].c_str(), NULL, 0);
    uint32_t nonce = strtoull(params[5].c_str(), NULL, 0);
    Coin::CoinBlockHeader header(version, prevblockhash, merkleroot, timestamp, bits, nonce);

    return header.getSerialized().getHex();
}

cli::result_t cmd_rawmerkleblock(const cli::params_t& params)
{
    uchar_vector rawblockheader(params[0]);
    Coin::CoinBlockHeader header(rawblockheader);

    uchar_vector flags(params[1]);
    uint32_t nTxs = strtoull(params[2].c_str(), NULL, 0);
    uint32_t nHashes = strtoull(params[3].c_str(), NULL, 0);
    if (params.size() != nHashes + 4)
        return "Invalid hash count.";

    vector<uchar_vector> hashes;
    for (size_t i = 4; i < params.size(); i++)
    {
        uchar_vector hash(params[i]);
        hash.reverse();
        hashes.push_back(hash); 
    }

    Coin::MerkleBlock merkleblock(header, nTxs, hashes, flags);

    return merkleblock.getSerialized().getHex();
}

cli::result_t cmd_insertrawmerkleblock(const cli::params_t& params)
{
    uint32_t height = params.size() > 2 ? strtoull(params[2].c_str(), NULL, 0) : 0;

    uchar_vector rawmerkleblock(params[1]);
    std::shared_ptr<MerkleBlock> merkleblock(new MerkleBlock());
    merkleblock->fromCoinCore(Coin::MerkleBlock(rawmerkleblock), height);

    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    bool rval = (bool)vault.insertMerkleBlock(merkleblock);

    stringstream ss;
    ss << "Merkle block " << uchar_vector(merkleblock->blockheader()->hash()).getHex() << (rval ? " " : " not ") << "inserted.";
    return ss.str();
}

cli::result_t cmd_deleteblock(const cli::params_t& params)
{
    uint32_t height = strtoull(params[1].c_str(), NULL, 0);
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);
    unsigned int count = vault.deleteMerkleBlock(height);

    stringstream ss;
    ss << count << " merkle blocks deleted.";
    return ss.str();
}

cli::result_t cmd_exportmerkleblocks(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);

    std::string output_file = params.size() > 1 ? params[1] : (params[0] + ".chain");
    vault.exportMerkleBlocks(output_file);

    stringstream ss;
    ss << "Merkle blocks exported to " << output_file << ".";
    return ss.str();
}

cli::result_t cmd_importmerkleblocks(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);

    std::string input_file = params[1];
    vault.importMerkleBlocks(input_file);

    stringstream ss;
    ss << "Merkle blocks imported from " << input_file << ".";
    return ss.str();
}

cli::result_t cmd_incompleteblocks(const cli::params_t& params)
{
    Vault vault(g_dbuser, g_dbpasswd, params[0], false);

    hashvector_t hashes = vault.getIncompleteBlockHashes();

    stringstream ss;
    for (auto& hash: hashes)
    {
        ss << uchar_vector(hash).getHex() << endl;
    }
    return ss.str();
}

cli::result_t cmd_randombytes(const cli::params_t& params)
{
    uchar_vector bytes = random_bytes(strtoul(params[0].c_str(), NULL, 0));
    return bytes.getHex();
}

int main(int argc, char* argv[])
{
    stringstream helpMessage;
    helpMessage << "CoinDB by Eric Lombrozo " << COINDB_VERSION << " - Schema " << SCHEMA_VERSION << " - " << DBMS;

    using namespace cli;
    Shell shell(helpMessage.str());

    // Global operations
    shell.add(command(
        &cmd_create,
        "create",
        "create a new vault",
        command::params(1, "db file"),
        command::params(1, "network = bitcoin")));
    shell.add(command(
        &cmd_info,
        "info",
        "display general information about file",
        command::params(1, "db file")));
    shell.add(command(
        &cmd_migrate,
        "migrate",
        "migrate schema version",
        command::params(1, "db file")));
    shell.add(command(
        &cmd_exportvault,
        "exportvault",
        "export vault contents to portable file",
        command::params(1, "db file"),
        command::params(2, "export private keys = true", "output file = *.portable")));
    shell.add(command(
        &cmd_importvault,
        "importvault",
        "import vault contents from portable file",
        command::params(2, "db file", "portable file"),
        command::params(1, "import private keys = true")));

    // Contact operations
    shell.add(command(
        &cmd_contactinfo,
        "contactinfo",
        "display contact information",
        command::params(2,"db file", "username")));
    shell.add(command(
        &cmd_newcontact,
        "newcontact",
        "add a new contact",
        command::params(2, "db file", "username")));
    shell.add(command(
        &cmd_renamecontact,
        "renamecontact",
        "change a contact's username",
        command::params(3, "db file", "old username", "new username")));
    shell.add(command(
        &cmd_listcontacts,
        "listcontacts",
        "display list of contacts",
        command::params(1, "db file")));

    // Keychain operations
    shell.add(command(
        &cmd_keychainexists,
        "keychainexists",
        "check if a keychain exists",
        command::params(1, "db file")));
    shell.add(command(
        &cmd_newkeychain,
        "newkeychain",
        "create a new keychain",
        command::params(2, "db file", "keychain name"),
        command::params(1, "passphrase")));
    shell.add(command(
        &cmd_renamekeychain,
        "renamekeychain",
        "rename a keychain",
        command::params(3, "db file", "old name", "new name")));
    shell.add(command(
        &cmd_encryptkeychain,
        "encryptkeychain",
        "encrypt a keychain",
        command::params(3, "db file", "keychain name", "passphrase")));
    shell.add(command(
        &cmd_decryptkeychain,
        "decryptkeychain",
        "decrypt a keychain",
        command::params(3, "db file", "keychain name", "passphrase")));
    shell.add(command(
        &cmd_reencryptkeychain,
        "reencryptkeychain",
        "reencrypt a keychain",
        command::params(4, "db file", "keychain name", "old passphrase", "new passphrase")));
    shell.add(command(
        &cmd_keychaininfo,
        "keychaininfo",
        "display keychain information about a specific keychain",
        command::params(2, "db file", "keychain name")));
    shell.add(command(
        &cmd_keychains,
        "keychains",
        "display keychains",
        command::params(1, "db file"),
        command::params(2, "account = @all", "show hidden = false")));
    shell.add(command(
        &cmd_exportkeychain,
        "exportkeychain",
        "export a keychain to file",
        command::params(2, "db file", "keychain name"),
        command::params(1, "export private key = false")));
    shell.add(command(
        &cmd_importkeychain,
        "importkeychain",
        "import a keychain from file",
        command::params(2, "db file", "keychain file"),
        command::params(1, "import private key = true")));
    shell.add(command(
        &cmd_exportbip32,
        "exportbip32",
        "export a keychain in BIP32 extended key format",
        command::params(2, "db file", "keychain name"),
        command::params(1, "passphrase")));
    shell.add(command(
        &cmd_importbip32,
        "importbip32",
        "import a keychain in BIP32 extended key format",
        command::params(3, "db file", "keychain name", "BIP32"),
        command::params(1, "passphrase")));

    // Account operations
    shell.add(command(
        &cmd_accountexists,
        "accountexists",
        "check if an account exists",
        command::params(2, "db file", "account name")));
    shell.add(command(
        &cmd_newaccount,
        "newaccount",
        "create a new account using specified keychains",
        command::params(4, "db file", "account name", "minsigs", "keychain 1"),
        command::params(3, "keychain 2", "keychain 3", "...")));
    shell.add(command(
        &cmd_newuncompressedaccount,
        "newuncompressedaccount",
        "create a new account with uncompressed public keys using specified keychains",
        command::params(4, "db file", "account name", "minsigs", "keychain 1"),
        command::params(3, "keychain 2", "keychain 3", "...")));
    shell.add(command(
        &cmd_renameaccount,
        "renameaccount",
        "rename an account",
        command::params(3, "db file", "old name", "new name")));
    shell.add(command(
        &cmd_accountinfo,
        "accountinfo",
        "display account information",
        command::params(2, "db file", "account name")));
    shell.add(command(
        &cmd_listaccounts,
        "listaccounts",
        "display list of accounts",
        command::params(1, "db file")));
    shell.add(command(
        &cmd_exportaccount,
        "exportaccount",
        "export account to file",
        command::params(2, "db file", "account name"),
        command::params(1, "output file = *.acct")));
    shell.add(command(
        &cmd_exportsharedaccount,
        "exportsharedaccount",
        "export shared account to file",
        command::params(2, "db file", "account name"),
        command::params(1, "output file = *.sharedacct")));
    shell.add(command(
        &cmd_importaccount,
        "importaccount",
        "import account from file",
        command::params(2, "db file", "account file")));
    shell.add(command(
        &cmd_newaccountbin,
        "newaccountbin",
        "add a new account bin",
        command::params(3, "db file", "account name", "bin name")));
    shell.add(command(
        &cmd_issuescript,
        "issuescript",
        "issue a new signing script",
        command::params(2, "db file", "account name"),
        command::params(3, "label", (std::string("bin name = ") + DEFAULT_BIN_NAME).c_str(), "index = 0")));
    shell.add(command(
        &cmd_invoicecontact,
        "invoicecontact",
        "create an invoice for a contact ",
        command::params(3, "db file", "account name", "username"),
        command::params(3, "label", (std::string("bin name = ") + DEFAULT_BIN_NAME).c_str(), "index = 0")));
    shell.add(command(
        &cmd_listscripts,
        "listscripts",
        "display list of signing scripts (flags: UNUSED=1, CHANGE=2, PENDING=4, RECEIVED=8, CANCELED=16)",
        command::params(1, "db file"),
        command::params(3, "account name = @all", "bin name = @all", "flags = PENDING | RECEIVED")));
    shell.add(command(
        &cmd_history,
        "history",
        "display transaction history",
        command::params(1, "db file"),
        command::params(3, "account name = @all", "bin name = @all", "hide change = true")));
    shell.add(command(
        &cmd_historycsv,
        "historycsv",
        "display transaction history in csv format",
        command::params(1, "db file"),
        command::params(3, "account name = @all", "bin name = @all", "hide change = true")));
    shell.add(command(
        &cmd_unspent,
        "unspent",
        "display unspent outputs",
        command::params(2, "db file", "account name"),
        command::params(1, "minimum confirmations = 0")));
    shell.add(command(
        &cmd_unsigned,
        "unsigned",
        "display unsigned transactions",
        command::params(1, "db file"),
        command::params(3, "account name = @all", "bin name = @all", "hide change = true")));
    shell.add(command(
        &cmd_unsignedhashes,
        "unsignedhashes",
        "display unsigned transaction hashes",
        command::params(1, "db file"),
        command::params(3, "account name = @all", "bin name = @all", "hide change = true")));
    shell.add(command(
        &cmd_refillaccountpool,
        "refillaccountpool",
        "refill signing script pool for account",
        command::params(2, "db file", "account name")));

    // Account bin operations
    shell.add(command(
        &cmd_listbins,
        "listbins",
        "display list of bins",
        command::params(1, "db file")));
    shell.add(command(
        &cmd_exportbin,
        "exportbin",
        "export account bin to file",
        command::params(3, "db file", "account name", "bin name"),
        command::params(2, "export name = account_name-bin_name", "output file = *.bin")));
    shell.add(command(
        &cmd_importbin,
        "importbin",
        "import account bin from file",
        command::params(2, "db file", "bin file")));

    // Tx operations
    shell.add(command(
        &cmd_listtxs,
        "listtxs",
        "list transactions",
        command::params(1, "db file"),
        command::params(2, "all | unsigned (default: all)", "minheight (default:0)")));
    shell.add(command(
        &cmd_txinfo,
        "txinfo",
        "display transaction information",
        command::params(2, "db file", "tx hash or id")));
    shell.add(command(
        &cmd_txconf,
        "txconf",
        "display the number of confirmations a transaction has received",
        command::params(2, "db file", "tx hash or id")));
    shell.add(command(
        &cmd_rawtx,
        "rawtx",
        "get transaction in raw hex",
        command::params(2, "db file", "tx hash or id"),
        command::params(1, "export to file = false")));
    shell.add(command(
        &cmd_insertrawtx,
        "insertrawtx",
        "insert a raw hex transaction into database",
        command::params(2, "db file", "tx raw hex or tx file name")));
    shell.add(command(
        &cmd_newrawtx,
        "newrawtx",
        "create a new raw transaction",
        command::params(4, "db file", "account name", "address 1", "value 1"),
        command::params(6, "address 2", "value 2", "...", "fee = 0", "version = 1", "locktime = 0")));
    shell.add(command(
        &cmd_createtx,
        "createtx",
        "create a new transaction",
        command::params(5, "db file", "account name", "txout index list", "address 1", "value 1"),
        command::params(7, "address 2", "value 2", "...", "fee = 0", "min confirmations = 1", "version = 1", "locktime = 0")));
    shell.add(command(
        &cmd_deletetx,
        "deletetx",
        "delete a transaction",
        command::params(2, "db file", "tx hash or id")));
    shell.add(command(
        &cmd_consolidate,
        "consolidate",
        "consolidate transaction outputs",
        command::params(4, "db file", "account name", "max tx size(bytes)", "address"),
        command::params(4, "min fee = 0", "min confirmations = 1", "version = 1", "locktime = 0")));
    shell.add(command(
        &cmd_signingrequest,
        "signingrequest",
        "gets signing request for transaction with missing signatures",
        command::params(2, "db file", "tx hash or id")));
    shell.add(command(
        &cmd_signtx,
        "signtx",
        "add signatures to transaction for specified keychain",
        command::params(3, "db file", "tx hash or id", "keychain name"),
        command::params(1, "passphrase")));
    shell.add(command(
        &cmd_exporttxs,
        "exporttxs",
        "export transactions to file",
        command::params(1, "db file"),
        command::params(2, "minheight = 0", "output file = *.txs")));
    shell.add(command(
        &cmd_importtxs,
        "importtxs",
        "import transactions from file",
        command::params(2, "db file", "account file")));

    // Blockchain operations
    shell.add(command(
        &cmd_bestheight,
        "bestheight",
        "display the best block height",
        command::params(1, "db file")));
    shell.add(command(
        &cmd_horizonheight,
        "horizonheight",
        "display height of first stored block",
        command::params(1, "db file")));
    shell.add(command(
        &cmd_horizontimestamp,
        "horizontimestamp",
        "display timestamp minimum for first stored block",
        command::params(1, "db file"),
        command::params(1, "use gmt = false")));
    shell.add(command(
        &cmd_blockinfo,
        "blockinfo",
        "display block information",
        command::params(2, "db file", "height")));    
    shell.add(command(
        &cmd_rawblockheader,
        "rawblockheader",
        "construct a raw block header",
        command::params(6, "version", "previous block hash", "merkle root", "timestamp", "bits", "nonce")));
    shell.add(command(
        &cmd_rawmerkleblock,
        "rawmerkleblock",
        "construct a raw merkle block",
        command::params(4, "raw block header", "flags", "nTxs", "nHashes"),
        command::params(3, "hash 1", "hash 2", "...")));
    shell.add(command(
        &cmd_insertrawmerkleblock,
        "insertrawmerkleblock",
        "insert raw merkle block into database",
        command::params(2, "db file", "raw merkle block"),
        command::params(1, "height = 0")));
    shell.add(command(
        &cmd_deleteblock,
        "deleteblock",
        "delete merkle block including all descendants",
        command::params(1, "db file"),
        command::params(1, "height = 0")));
    shell.add(command(
        &cmd_exportmerkleblocks,
        "exportmerkleblocks",
        "export all merkle blocks to file",
        command::params(1, "db file"),
        command::params(1, "output file = *.chain")));
    shell.add(command(
        &cmd_importmerkleblocks,
        "importmerkleblocks",
        "import merkle blocks from file",
        command::params(2, "db file", "input file")));
    shell.add(command(
        &cmd_incompleteblocks,
        "incompleteblocks",
        "display hashes of blocks for which we do not have all our transactions",
        command::params(1, "db file")));

    // Miscellaneous
    shell.add(command(
        &cmd_randombytes,
        "randombytes",
        "output random bytes in hex",
        command::params(1, "length")));

    try 
    {
        CoinDBConfig config;
        if (!config.parseParams(argc, argv))
        {
            cout << config.getHelpOptions();
            return 0;
        }

        g_dbuser = config.getDatabaseUser();
        g_dbpasswd = config.getDatabasePassword();
        string logfile = config.getDataDir() + "/coindb.log";

        INIT_LOGGER(logfile.c_str());

        return shell.exec(argc, argv);
    }
    catch (const std::exception& e)
    {
        cerr << e.what() << endl;
        return 1;
    }
    return 0;
}

