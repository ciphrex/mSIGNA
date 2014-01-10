///////////////////////////////////////////////////////////////////////////////
//
// vault.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

//
// Build with:
//      g++ -o obj/vault.o -DDATABASE_SQLITE -c vault.cpp -std=c++0x -I../deps/CoinClasses/src
//      g++ -o ../build/vault obj/vault.o CoinQ_vault_db-odb.o -lodb-sqlite -lodb -lssl -lcrypto
//

#include <cli.hpp>
#include <CoinKey.h>
#include <Base58Check.h>

#include <memory>
#include <algorithm>
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include "CoinQ_database.hxx"

#include "CoinQ_vault.h"

#include "CoinQ_vault_db.hxx"
#include "CoinQ_vault_db-odb.hxx"

const unsigned char BASE58_VERSIONS[] = { 0x00, 0x05 };

using namespace std;
using namespace odb::core;
using namespace CoinQ::Script;
using namespace CoinQ::Vault;

cli::result_t cmd_create(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 1) {
        return "create <filename> - create a new vault.";
    }

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};
    unique_ptr<database> db(open_database(argc, argv, true));

    stringstream ss;
    ss << "Vault " << params[0] << " created.";
    return ss.str();
}

cli::result_t cmd_newkeychain(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 3) {
        return "newkeychain <filename> <keychain_name> <numkeys> - create a new keychain.";
    }

    long numkeys = strtol(params[2].c_str(), NULL, 0);
    if (numkeys < 1) {
        throw std::runtime_error("Keychain must contain at least one key.");
    }

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};
    unique_ptr<database> db(open_database(argc, argv));

    Vault vault(argc, argv, false);
    vault.newKeychain(params[1], numkeys);

    stringstream ss;
    ss << "Added keychain " << params[1] << " with " << numkeys << " keys to vault " << params[0] << ".";
    return ss.str();
}

cli::result_t cmd_renamekeychain(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 3) {
        return "renamekeychain <filename> <oldname> <newname> - rename a keychain.";
    }

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};

    Vault vault(argc, argv, false);
    vault.renameKeychain(params[1], params[2]);

    stringstream ss;
    ss << "Keychain " << params[1] << " renamed to " << params[2] << ".";
    return ss.str();
}

cli::result_t cmd_listkeychains(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 1) {
        return "listkeychains <filename> - list all keychains in vault.";
    }

    stringstream ss;

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};
    
    Vault vault(argc, argv, false);
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

cli::result_t cmd_listkeys(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() < 2 || params.size() > 3) {
        return "listkeys <filename> <keychain> [show_privkeys=false] - list all keys in keychains.";
    }
 
    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};
 
    bool show_privkeys = (params.size() > 2) ? (params[2] == "true") : false;

    Vault vault(argc, argv, false);

    std::shared_ptr<Keychain> keychain = vault.getKeychain(params[1]);

    std::stringstream ss;
    bool first = true;
    for (auto& key: keychain->keys()) {
        if (first) { first = false; }
        else       { ss << endl; }

        ss << "id: " << key->id() << " pubkey: " << uchar_vector(key->pubkey()).getHex();
        if (show_privkeys) { ss << " privkey: " << uchar_vector(key->privkey()).getHex(); }
    }

    return ss.str();
}

cli::result_t cmd_exportkeys(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() < 3 || params.size() > 4) {
        return "exportkeys <filename> <keychain> <exportfile> [exportprivkeys=false] - exports keys in keychain to file.";
    }

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};

    bool exportprivkeys = (params.size() > 3) ? (params[3] == "true") : false;

    Vault vault(argc, argv, false);

    bytes_t hash = vault.exportKeychain(params[1], params[2], exportprivkeys);

    std::stringstream ss;
    ss << "Keychain " << params[1] << " with hash " << uchar_vector(hash).getHex() << " exported to " << params[2] << ".";
    return ss.str();
}

cli::result_t cmd_importkeys(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 3) {
        return "importkeys <filename> <keychain> <importfile> - imports keys from file into new keychain.";
    }

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};

    Vault vault(argc, argv, false);

    bool importprivkeys = true;
    bytes_t hash = vault.importKeychain(params[1], params[2], importprivkeys);

    std::stringstream ss;
    ss << "Keychain " << params[1] << " with hash " << uchar_vector(hash).getHex() << " imported from " << params[2] << ".";
    return ss.str();
}

cli::result_t cmd_newaccount(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() < 4) {
        return "newaccount <filename> <accountname> <minsigs> <keychain1> <keychain2> ... - create a new account.";
    }

    cli::params_t keychain_names(params.begin() + 3, params.end());
    std::sort(keychain_names.begin(), keychain_names.end());
    auto it = std::unique_copy(params.begin() + 3, params.end(), keychain_names.begin());
    if (std::distance(keychain_names.begin(), it) != (size_t)(params.size() - 3)) {
        throw std::runtime_error("Keychain list cannot contain duplicates.");
    }

    if (keychain_names.size() > 16) {
        throw std::runtime_error("Account can use a maximum of 16 keychains.");
    }

    int minsigs = strtol(params[2].c_str(), NULL, 0);
    if (minsigs < 1 || (size_t)minsigs > keychain_names.size()) {
        throw std::runtime_error("Invalid minsigs.");
    }

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};

    Vault vault(argc, argv, false);
    vault.newAccount(params[1], (unsigned int)minsigs, keychain_names);

    stringstream ss;
    ss << minsigs << " of " << keychain_names.size() << " account " << params[1] << " created.";
    return ss.str();
}

cli::result_t cmd_eraseaccount(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2) {
        return "eraseaccount <filename> <accountname> - erase an account.";
    }

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};

    Vault vault(argc, argv, false);
    vault.eraseAccount(params[1]);

    stringstream ss;
    ss << "Account " << params[1] << " erased.";
    return ss.str();
}

cli::result_t cmd_renameaccount(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 3) {
        return "renameaccount <filename> <oldname> <newname> - rename an account.";
    }

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};

    Vault vault(argc, argv, false);
    vault.renameAccount(params[1], params[2]);

    stringstream ss;
    ss << "Account " << params[1] << " renamed to " << params[2] << ".";
    return ss.str();
}

cli::result_t cmd_listaccounts(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 1) {
        return "listaccounts <filename> - list all accounts in vault.";
    }

    stringstream ss;

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};

    Vault vault(argc, argv, false);
    std::vector<AccountInfo> accounts = vault.getAccounts();
    bool first = true;
    for (auto& account: accounts) {
        if (first) {
            first = false;
        }
        else {
            ss << endl;
        }
        ss << "id: " << account.id() << " name: " << account.name() << " balance: " << account.balance() << " minsigs: " << account.minsigs() << " keychains: (";
        bool first_ = true;
        for (auto& keychain_name: account.keychain_names()) {
            if (first_) {
                first_ = false;
            }
            else {
                ss << ", ";
            }
            ss << keychain_name;
        }
        ss << ")";
    }

    return ss.str();
}

cli::result_t cmd_exportaccount(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 3) {
        return "exportaccount <filename> <account> <exportfile> - exports account to file.";
    }

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};

    Vault vault(argc, argv, false);

    bytes_t hash = vault.exportAccount(params[1], params[2]);

    std::stringstream ss;
    ss << "Account " << params[1] << " with hash " << uchar_vector(hash).getHex() << " exported to " << params[2] << ".";
    return ss.str();
}

cli::result_t cmd_importaccount(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 3) {
        return "importaccount <filename> <keychain> <importfile> - imports account from file.";
    }

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};

    Vault vault(argc, argv, false);

    bytes_t hash = vault.importAccount(params[1], params[2]);

    std::stringstream ss;
    ss << "Account " << params[1] << " with hash " << uchar_vector(hash).getHex() << " imported from " << params[2] << ".";
    return ss.str();
}

cli::result_t cmd_requestpayment(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() < 3 || params.size() > 4) {
        return "requestpayment <filename> <account> <label> [value=0]- request a new payment output.";
    }

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};

    Vault vault(argc, argv, false);

    uint64_t value = (params.size() > 3) ? strtoull(params[3].c_str(), NULL, 10) : 0;

    std::shared_ptr<TxOut> txout = vault.newTxOut(params[1], params[2], value);

    using namespace CoinQ::Script;

    std::string address;
    payee_t payee = getScriptPubKeyPayee(txout->script());
    if (payee.first == SCRIPT_PUBKEY_PAY_TO_SCRIPT_HASH) {
        address = toBase58Check(payee.second, 0x05);   
    }
    else {
        address = "N/A";
    }

    stringstream ss;
    ss << "account: " << params[1] << " label: " << params[2] << " value: " << txout->value() << " script: " << uchar_vector(txout->script()).getHex() << " address: " << address;

    return ss.str();
}

cli::result_t cmd_listaddresses(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2) {
        return "listaddresses <filename> <account> - list receiving addresses for account.";
    }

    stringstream ss;

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};

    Vault vault(argc, argv, false);

    std::vector<std::shared_ptr<SigningScriptView>> views = vault.getSigningScriptViews(params[1]);
    bool first = true;
    for (auto& view: views) {
        if (first) { first = false; }
        else       { ss << endl;    }

        try {
            CoinQ::Script::Script script(view->txinscript);
            bytes_t redeemscript = script.redeemscript();
            std::string address = CoinQ::Script::getAddressForTxOutScript(view->txoutscript, BASE58_VERSIONS);
            ss << "address: " << address << " redeemscript: " << uchar_vector(redeemscript).getHex() << " label: " << (view->label.empty() ? "none" : view->label) << " status: " << SigningScript::getStatusString(view->status);
        }
        catch (const std::exception& e) {
            ss << e.what();
        }
    }

    return ss.str();
}

cli::result_t cmd_listunspent(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2) {
        return "listunspent <filename> <account> - list unspent txouts for account.";
    }
 
    stringstream ss;
 
    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};
 
    Vault vault(argc, argv, false);

    std::vector<std::shared_ptr<TxOutView>> views = vault.getTxOutViews(params[1]);
    uint64_t total = 0;
    for (auto& view: views) {
        total += view->value;
        std::string address = CoinQ::Script::getAddressForTxOutScript(view->script, BASE58_VERSIONS); //toBase58Check(view->scripthash, 0x05);
        ss << "value(btc): " << satoshisToBtcString(view->value, false) << " address: " << address
           << " outpoint: " << uchar_vector(view->txhash).getHex() << ":" << view->txindex << endl;
    }
    ss << "Account Balance: " << satoshisToBtcString(total, false);

    return ss.str();
}

cli::result_t cmd_listreceived(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2) {
        return "listreceived <filename> <account> - list all received coins.";
    }

    stringstream ss;

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};

    Vault vault(argc, argv, false);

    std::vector<std::shared_ptr<TxOutView>> views = vault.getTxOutViews(params[1], false);
    uint64_t total = 0;
    for (auto& view: views) {
        total += view->value;
        std::string address = CoinQ::Script::getAddressForTxOutScript(view->script, BASE58_VERSIONS); //toBase58Check(view->scripthash, 0x05);
        ss << "value(btc): " << satoshisToBtcString(view->value, false) << " address: " << address 
           << " label: " << (view->signingscript_label.empty() ? "none" : view->signingscript_label) << " status: " << SigningScript::getStatusString(view->signingscript_status) << endl;
    }
    ss << "Total Received: " << satoshisToBtcString(total, false);

    return ss.str();
}

cli::result_t cmd_history(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2) {
        return "history <filename> <account> - view transaction history for account.";
    }

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};

    Vault vault(argc, argv, false);
    std::vector<std::shared_ptr<AccountTxOutView>> history = vault.getAccountHistory(params[1]);

    stringstream ss;

    bytes_t last_txhash;
    uint64_t balance = 0;
    bool first = true;
    for (auto& item: history) {
        if (first) { first = false; }
        else       {  ss << endl; }

        std::stringstream fee;

        std::string description = (item->description) ? *item->description : "Unavailable";
        std::string type;
        std::stringstream amount;
        switch (item->type) {
        case TxOut::CHANGE:
            type = "Change";
            break;
        case TxOut::DEBIT:
            amount << "-";
            type = "Sent";
            balance -= item->value;
            if (item->txhash != last_txhash && item->have_fee && item->fee > 0) {
                fee << "fee: -" << item->fee;
                balance -= item->fee;
                last_txhash = item->txhash;
            }
            break;
        case TxOut::CREDIT:
            type = "Received";
            amount << "+";
            balance += item->value;
            break;
        default:
            type = "Unavailable";
        }
        amount << item->value;

        std::string address = CoinQ::Script::getAddressForTxOutScript(item->script, BASE58_VERSIONS);
        ss << item->txtimestamp << " " << description << " " << type << " " << amount.str() << " " << fee.str() << " " << balance << " " << address;
    }

    return ss.str();
}

cli::result_t cmd_scriptcount(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2) {
        return "scriptcount <filename> <account> - return a count of unused scripts in the account";
    }

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};

    Vault vault(argc, argv, false);

    unsigned long count = vault.getScriptCount(params[1], SigningScript::UNUSED);
    stringstream ss;
    ss << "Unused scripts for account " << params[1] << ": " << count;
    return ss.str();
}

cli::result_t cmd_tag(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 3) {
        return "tag <filename> <address> <description> - create a new tag for an address.";
    }

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};

    bytes_t script = CoinQ::Script::getTxOutScriptForAddress(params[1], BASE58_VERSIONS);

    Vault vault(argc, argv, false);
    if (vault.scriptTagExists(script)) {
        return "Tag already exists.";
    }

    vault.addScriptTag(script, params[2]);

    return "Tag created.";
}

cli::result_t cmd_retag(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 3) {
        return "retag <filename> <address> <description> - update tag for an address.";
    }

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};

    bytes_t script = CoinQ::Script::getTxOutScriptForAddress(params[1], BASE58_VERSIONS);

    Vault vault(argc, argv, false);
    if (!vault.scriptTagExists(script)) {
        return "Tag not found.";
    }

    vault.updateScriptTag(script, params[2]);

    return "Tag updated.";
}

cli::result_t cmd_untag(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 3) {
        return "untag <filename> <address> - delete tag for an address.";
    }

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};

    bytes_t script = CoinQ::Script::getTxOutScriptForAddress(params[1], BASE58_VERSIONS);

    Vault vault(argc, argv, false);
    if (!vault.scriptTagExists(script)) {
        return "Tag not found.";
    }

    vault.deleteScriptTag(script);

    return "Tag deleted.";
}

cli::result_t cmd_addrawtx(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2) {
        return "addrawtx <filename> <rawtxhex> - attempts to add the transaction to vault.";
    }

    stringstream ss;

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};

    Vault vault(argc, argv, false);
    std::shared_ptr<Tx> tx = std::make_shared<Tx>();
    tx->set(uchar_vector(params[1]));
    tx->timestamp(time(NULL));
    if (vault.addTx(tx)) {
        ss << "Transaction " << uchar_vector(tx->hash()).getHex() << " added to vault.";
    }
    else {
        ss << "Transaction is not part of vault.";
    }
    return ss.str();
}

cli::result_t cmd_deletetx(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2) {
        return "deletetx <filename> <txhash> - deletes transaction from vault.";
    }

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};

    uchar_vector txhash;
    txhash.setHex(params[1]);

    Vault vault(argc, argv, false);

    stringstream ss;
    if (vault.deleteTx(txhash)) {
        ss << "Transaction deleted.";
    }
    else {
        ss << "Transaction not found.";
    }
    return ss.str(); 
}

cli::result_t cmd_newrawtx(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() < 7 || (params.size() % 2 != 1)) {
        return "newrawtx <filename> <account> <tx_version> <tx_locktime> <fee> <address> <value> [<address> <value> ...]- creates a raw unsigned transaction.";
    }

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};
 
    Vault vault(argc, argv, false);

    uint32_t tx_version = strtoul(params[2].c_str(), NULL, 10);
    uint32_t tx_locktime = strtoul(params[3].c_str(), NULL, 10);

    uint64_t fee = strtoull(params[4].c_str(), NULL, 10);

    Tx::txouts_t txouts;
    size_t i = 5;
    while (i < params.size()) {
        bytes_t txoutscript = getTxOutScriptForAddress(params[i++], BASE58_VERSIONS);
        uint64_t value = strtoull(params[i++].c_str(), NULL, 10);

        std::shared_ptr<TxOut> txout(new TxOut(value, txoutscript));
        txouts.push_back(txout);
    }

    std::shared_ptr<Tx> tx = vault.newTx(params[1], tx_version, tx_locktime, txouts, fee, 1);

    return uchar_vector(tx->raw()).getHex();
}

cli::result_t cmd_signrawtx(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2) {
        return "signtx <filename> <rawtx> - sign a raw transaction using keys in database.";
    }

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};

    Vault vault(argc, argv, false);

    std::shared_ptr<Tx> tx = std::make_shared<Tx>();
    tx->set(uchar_vector(params[1]));
    vault.signTx(tx);

    return uchar_vector(tx->raw()).getHex();
}

cli::result_t cmd_getrawtx(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2) {
        return "getrawtx <filename> <hash> - get raw transaction.";
    }

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};

    Vault vault(argc, argv, false);

    uchar_vector hash;
    hash.setHex(params[1]);

    std::shared_ptr<Tx> tx = vault.getTx(hash);
    return uchar_vector(tx->raw()).getHex();
}

cli::result_t cmd_getstarttime(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 1) {
        return "getstarttime <filename> - show the time at which the first account was created.";
    }

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};

    Vault vault(argc, argv, false);

    std::stringstream ss;
    ss << vault.getFirstAccountTimeCreated();
    return ss.str();
}

cli::result_t cmd_getbestblock(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 1) {
        return "getbestblock <filename> - show block information for the best block.";
    }

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};

    Vault vault(argc, argv, false);

    std::shared_ptr<BlockHeader> header = vault.getBestBlockHeader();
    if (!header) {
        return "Vault contains no block headers.";
    }

    std::stringstream ss;
    ss << "Height: " << header->height() << " Hash: " << uchar_vector(header->hash()).getHex() << " Timestamp: " << header->timestamp();
    return ss.str();
}

cli::result_t cmd_locatorhashes(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 1) {
        return "locatorhashes <filename> - shows list of block locator hashes.";
    }

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, params[0].c_str());
    char* argv[] = {prog, opt, buf};

    Vault vault(argc, argv, false);

    std::vector<bytes_t> hashes = vault.getLocatorHashes();
    if (hashes.empty()) {
        return "No block headers found.";
    }

    std::stringstream ss;
    bool first = true;
    for (auto& hash: hashes) {
        if (first)  { first = false; }
        else        { ss << ","; }

        ss << uchar_vector(hash).getHex();
    }
    return ss.str();
}


int main(int argc, char** argv)
{
    cli::command_map cmds("CoinVault by Eric Lombrozo v0.0.1");
    cmds.add("create", &cmd_create);
    cmds.add("newkeychain", &cmd_newkeychain);
    cmds.add("renamekeychain", &cmd_renamekeychain);
    cmds.add("listkeychains", &cmd_listkeychains);
    cmds.add("listkeys", &cmd_listkeys);
    cmds.add("exportkeys", &cmd_exportkeys);
    cmds.add("importkeys", &cmd_importkeys);
    cmds.add("newaccount", &cmd_newaccount);
    cmds.add("eraseaccount", &cmd_eraseaccount);
    cmds.add("renameaccount", &cmd_renameaccount);
    cmds.add("listaccounts", &cmd_listaccounts);
    cmds.add("exportaccount", &cmd_exportaccount);
    cmds.add("importaccount", &cmd_importaccount);
    cmds.add("listaddresses", &cmd_listaddresses);
    cmds.add("listunspent", &cmd_listunspent);
    cmds.add("listreceived", &cmd_listreceived);
    cmds.add("history", &cmd_history);
    cmds.add("scriptcount", &cmd_scriptcount);
    cmds.add("tag", &cmd_tag);
    cmds.add("retag", &cmd_retag);
    cmds.add("untag", &cmd_untag);
    cmds.add("addrawtx", &cmd_addrawtx);
    cmds.add("deletetx", &cmd_deletetx);
    cmds.add("newrawtx", &cmd_newrawtx);
    cmds.add("requestpayment", &cmd_requestpayment);
    cmds.add("signrawtx", &cmd_signrawtx);
    cmds.add("getrawtx", &cmd_getrawtx);
    cmds.add("getstarttime", &cmd_getstarttime);
    cmds.add("getbestblock", &cmd_getbestblock);
    cmds.add("locatorhashes", &cmd_locatorhashes);
    try {
        return cmds.exec(argc, argv);
    }
    catch (const std::exception& e) {
        cerr << e.what() << endl;
        return 1;
    }
    return 0;
}
