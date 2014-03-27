///////////////////////////////////////////////////////////////////////////////
//
// coindb.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.
//

#include "formatting.h"

#include <cli.hpp>
#include <Base58Check.h>

#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <Vault.h>
#include <Schema-odb.hxx>

#include <random.h>

#include "logger.h"

#include <iostream>
#include <sstream>

using namespace std;
using namespace odb::core;
using namespace CoinDB;

// Global operations
cli::result_t cmd_create(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 1)
        return "create <db file> - create a new vault.";

    Vault vault(params[0], true);

    stringstream ss;
    ss << "Vault " << params[0] << " created.";
    return ss.str();
}

cli::result_t cmd_info(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 1)
        return "info <db file> - display general information about vault.";

    Vault vault(params[0], false);
    uint32_t horizon_timestamp = vault.getHorizonTimestamp();

    stringstream ss;
    ss << "filename:            " << params[0] << endl
       << "horizon timestamp:   " << horizon_timestamp; 
    return ss.str();
}

// Keychain operations
cli::result_t cmd_keychainexists(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2)
        return "keychainexists <db file> <keychain_name> - check if a keychain exists.";

    Vault vault(params[0], false);
    bool bExists = vault.keychainExists(params[1]);

    stringstream ss;
    ss << (bExists ? "true" : "false");
    return ss.str();
}

cli::result_t cmd_newkeychain(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2)
        return "newkeychain <db file> <keychain_name> - create a new keychain.";

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
        return "erasekeychain <db file> <keychain_name> - erase a keychain.";
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
        return "renamekeychain <db file> <oldname> <newname> - rename a keychain.";

    Vault vault(params[0], false);
    vault.renameKeychain(params[1], params[2]);

    stringstream ss;
    ss << "Keychain " << params[1] << " renamed to " << params[2] << ".";
    return ss.str();
}

cli::result_t cmd_keychaininfo(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2)
        return "keychaininfo <db file> <keychain_name> - display keychain information.";

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
        return "listkeychains <db file> [root_only = false] - display list of keychains.";

    bool root_only = params.size() > 1 ? (params[1] == "true") : false;

    Vault vault(params[0], false);
    vector<shared_ptr<Keychain>> keychains = vault.getAllKeychains(root_only);

    stringstream ss;
    ss << formattedKeychainHeader();
    for (auto& keychain: keychains)
        ss << endl << formattedKeychain(keychain);
    return ss.str();
}

cli::result_t cmd_exportkeychain(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() < 2 || params.size() > 4)
        return "exportkeychain <db file> <keychain> [try export private key = false] [output file = keychain.(pub|priv)] - export a keychain to file.";

    bool export_privkey = params.size() > 2 ? (params[2] == "true") : false;

    std::string output_file;
    if (params.size() > 3)  { output_file = params[3]; }
    else                    { output_file = params[1] + (export_privkey ? ".priv" : ".pub"); }

    Vault vault(params[0], false);
    vault.exportKeychain(params[1], output_file, export_privkey);

    stringstream ss;
    ss << (export_privkey ? "Private" : "Public") << " keychain " << params[1] << " exported to " << output_file << ".";
    return ss.str();
}

cli::result_t cmd_importkeychain(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() < 2 || params.size() > 3)
        return "importkeychain <db file> <input file> [try import private key = true] - import a keychain from file.";

    bool import_privkey = params.size() > 2 ? (params[2] == "true") : true;

    Vault vault(params[0], false);
    std::shared_ptr<Keychain> keychain = vault.importKeychain(params[1], import_privkey);

    stringstream ss;
    ss << (import_privkey ? "Private" : "Public") << " keychain " << keychain->name() << " imported from " << params[1] << ".";
    return ss.str();
}

// Account operations
cli::result_t cmd_accountexists(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2)
        return "accountexists <db file> <account_name> - check if an account exists.";

    Vault vault(params[0], false);
    bool bExists = vault.accountExists(params[1]);

    stringstream ss;
    ss << (bExists ? "true" : "false");
    return ss.str();
}

cli::result_t cmd_newaccount(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() < 4)
        return "newaccount <db file> <account_name> <minsigs> <keychain1> [keychain2] [keychain3] ... - create a new account using specified keychains.";

    uint32_t minsigs = strtoull(params[2].c_str(), NULL, 10);
    size_t keychain_count = params.size() - 3;
    if (keychain_count > 15) throw std::runtime_error("Maximum number of keychains supported is 15.");
    if (minsigs > keychain_count) throw std::runtime_error("Minimum signatures cannot exceed number of keychains.");

    std::vector<std::string> keychain_names;
    for (size_t i = 3; i < params.size(); i++)
        keychain_names.push_back(params[i]);

    Vault vault(params[0], false);
    for (auto& keychain_name: keychain_names)
        vault.unlockKeychainChainCode(keychain_name, secure_bytes_t());
    vault.newAccount(params[1], minsigs, keychain_names);

    stringstream ss;
    ss << "Added account " << params[1] << " to vault " << params[0] << ".";
    return ss.str();
}

cli::result_t cmd_renameaccount(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 3)
        return "renameaccount <db file> <old_account_name> <new_account_name> - rename an account.";

    Vault vault(params[0], false);
    vault.renameAccount(params[1], params[2]);

    stringstream ss;
    ss << "Renamed account " << params[1] << " to " << params[2] << ".";
    return ss.str();
}

cli::result_t cmd_accountinfo(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2)
        return "accountinfo <db file> <account_name> - display account info.";

    Vault vault(params[0], false);
    AccountInfo accountInfo = vault.getAccountInfo(params[1]);
    uint64_t balance = vault.getAccountBalance(params[1], 0);
    uint64_t confirmed_balance = vault.getAccountBalance(params[1], 1);

    using namespace stdutils;
    stringstream ss;
    ss << "id:                " << accountInfo.id() << endl
       << "name:              " << accountInfo.name() << endl
       << "minsigs:           " << accountInfo.minsigs() << endl
       << "keychains:         " << delimited_list(accountInfo.keychain_names(), ", ") << endl
       << "unused_pool_size:  " << accountInfo.unused_pool_size() << endl
       << "time_created:      " << accountInfo.time_created() << endl
       << "bins:              " << delimited_list(accountInfo.bin_names(), ", ") << endl
       << "balance:           " << balance << endl
       << "confirmed balance: " << confirmed_balance;
    return ss.str();
}

cli::result_t cmd_listaccounts(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 1)
        return "listaccounts <db file> - display list of accounts.";

    Vault vault(params[0], false);
    vector<AccountInfo> accounts = vault.getAllAccountInfo();

    stringstream ss;
    ss << formattedAccountHeader();
    for (auto& account: accounts)
        ss << endl << formattedAccount(account);
    return ss.str();
}

cli::result_t cmd_exportaccount(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() < 3 || params.size() > 4)
        return "exportaccount <db file> <account> <passphrase> [output file = *.account]- export account to file.";

    std::string output_file = params.size() > 3 ? params[3] : (params[1] + ".account");

    Vault vault(params[0], false);
    AccountInfo accountInfo = vault.getAccountInfo(params[1]);
    for (auto& keychain_name: accountInfo.keychain_names())
        vault.unlockKeychainChainCode(keychain_name, secure_bytes_t());

    vault.exportAccount(params[1], output_file, secure_bytes_t(), bytes_t());

    stringstream ss;
    ss << "Account " << params[1] << " exported to " << output_file << ".";
    return ss.str();
}

cli::result_t cmd_importaccount(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 3)
        return "importaccount <db file> <input fle> <passphrase> - input account to file.";

    unsigned int privkeycount = 1;
    Vault vault(params[0], false);
    std::shared_ptr<Account> account = vault.importAccount(params[1], secure_bytes_t(), privkeycount);

    stringstream ss;
    ss << "Account " << account->name() << " imported from " << params[1] << ".";
    return ss.str();
}

cli::result_t cmd_newaccountbin(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 3)
        return "newaccountbin <db file> <account_name> <bin_name> - add new account bin.";

    Vault vault(params[0], false);
    AccountInfo accountInfo = vault.getAccountInfo(params[1]);
    for (auto& keychain_name: accountInfo.keychain_names())
        vault.unlockKeychainChainCode(keychain_name, secure_bytes_t());
    vault.addAccountBin(params[1], params[2]);

    stringstream ss;
    ss << "Account bin " << params[2] << " added to account " << params[1] << ".";
    return ss.str(); 
}

cli::result_t cmd_issuescript(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() < 2 || params.size() > 4)
        return std::string("issuescript <db file> <account_name> [bin_name = ") + DEFAULT_BIN_NAME + "] [label = null] - issue a new signing script.";

    Vault vault(params[0], false);
    std::string bin_name = params.size() > 2 ? params[2] : std::string(DEFAULT_BIN_NAME);
    std::string label = params.size() > 3 ? params[3] : std::string("");
    std::shared_ptr<SigningScript> script = vault.issueSigningScript(params[1], bin_name, label);

    std::string address = getAddressFromScript(script->txoutscript());

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
        return "listscripts <db file> [account_name = @all] [bin_name = @all] [flags = PENDING | RECEIVED] - display list of signing script. (flags: UNUSED=1, CHANGE=2, PENDING=4, RECEIVED=8, CANCELED=16)";

    std::string account_name = params.size() > 1 ? params[1] : std::string("@all");
    std::string bin_name = params.size() > 2 ? params[2] : std::string("@all");
    int flags = params.size() > 3 ? (int)strtoul(params[3].c_str(), NULL, 0) : ((int)SigningScript::ISSUED | (int)SigningScript::USED);
    
    Vault vault(params[0], false);
    vector<SigningScriptView> scriptViews = vault.getSigningScriptViews(account_name, bin_name, flags);

    stringstream ss;
    ss << formattedScriptHeader();
    for (auto& scriptView: scriptViews)
        ss << endl << formattedScript(scriptView);
    return ss.str();
}

cli::result_t cmd_listtxouts(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() < 1 || params.size() > 4)
        return "listtxouts <db file> [account_name = @all] [bin_name = @all] - display list of transaction outputs.";

    std::string account_name = params.size() > 1 ? params[1] : std::string("@all");
    std::string bin_name = params.size() > 2 ? params[2] : std::string("@all");
    
    Vault vault(params[0], false);
    uint32_t best_height = vault.getBestHeight();
    vector<TxOutView> txOutViews = vault.getTxOutViews(account_name, bin_name);

    stringstream ss;
    ss << formattedTxOutViewHeader();
    for (auto& txOutView: txOutViews)
    {
        if (!txOutView.receiving_account_name.empty())
            ss << endl << formattedTxOutView(txOutView, RECEIVE, best_height);

        if (!txOutView.sending_account_name.empty())
            ss << endl << formattedTxOutView(txOutView, SEND, best_height);
    }
    return ss.str();
}

cli::result_t cmd_refillaccountpool(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() < 1 || params.size() != 2 )
        return "refillaccountpool <db file> <account_name> - refill signing script pool for account.";

    Vault vault(params[0], false);
    AccountInfo accountInfo = vault.getAccountInfo(params[1]);
    for (auto& keychain_name: accountInfo.keychain_names())
        vault.unlockKeychainChainCode(keychain_name, secure_bytes_t());

    vault.refillAccountPool(params[1]);

    stringstream ss;
    ss << "Refilled account pool for account " << params[1] << ".";
    return ss.str();
}

cli::result_t cmd_txinfo(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() < 2 || params.size() > 3)
        return "txinfo <db file> <hash> [raw = false] - display transaction information.";

    bool raw = params.size() > 2 ? params[2] == "true" : false;

    Vault vault(params[0], false);
    std::shared_ptr<Tx> tx = vault.getTx(uchar_vector(params[1]));

    if (raw) return uchar_vector(tx->raw()).getHex();

    bytes_t hash = tx->status() == Tx::UNSIGNED ? tx->unsigned_hash() : tx->hash();

    stringstream ss;
    ss << "status:      " << Tx::getStatusString(tx->status()) << endl
       << "hash:        " << uchar_vector(hash).getHex() << endl
       << "version:     " << tx->version() << endl
       << "locktime:    " << tx->locktime() << endl
       << "timestamp:   " << tx->timestamp();

    ss << endl << endl << formattedTxInHeader();
    for (auto& txin: tx->txins())
        ss << endl << formattedTxIn(txin);

    ss << endl << endl << formattedTxOutHeader();
    for (auto& txout: tx->txouts())
        ss << endl << formattedTxOut(txout);
    
    return ss.str();
}

cli::result_t cmd_insertrawtx(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2)
        return "insertrawtx <db file> <raw tx hex> - inserts a raw transaction into vault.";

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

cli::result_t cmd_newrawtx(bool bHelp, const cli::params_t& params)
{
    if (!bHelp && params.size() >= 4)
    {
        while (true)
        {
            Vault vault(params[0], false);

            // Get outputs
            using namespace CoinQ::Script;
            const size_t MAX_VERSION_LEN = 2;
            size_t i = 2;
            txouts_t txouts;
            do
            {
                bytes_t txoutscript  = getTxOutScriptForAddress(params[i++], BASE58_VERSIONS);
                uint64_t value = strtoull(params[i++].c_str(), NULL, 0);
                std::shared_ptr<TxOut> txout(new TxOut(value, txoutscript));
                txouts.push_back(txout);
                 
            } while (i < (params.size() - 1) && params[i].size() > MAX_VERSION_LEN);

            uint64_t fee = i < params.size() ? strtoull(params[i++].c_str(), NULL, 0) : 0;
            uint32_t version = i < params.size() ? strtoul(params[i++].c_str(), NULL, 0) : 1;
            uint32_t locktime = i < params.size() ? strtoul(params[i++].c_str(), NULL, 0) : 0;
            if (i < params.size()) break;

            std::shared_ptr<Tx> tx = vault.createTx(params[1], version, locktime, txouts, fee, 1, false);
            return uchar_vector(tx->raw()).getHex();
        }
    }

    return "newrawtx <db file> <account> <address 1> <value 1> [address 2] [value 2] ... [fee = 0] [version = 1] [locktime = 0] - create a new raw transaction.";
}

cli::result_t cmd_deletetx(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2)
        return "deletetx <db file> <tx hash> - deletes transaction by hash.";

    Vault vault(params[0], false);
    uchar_vector hash(params[1]);
    vault.deleteTx(hash);

    stringstream ss;
    ss << "Tx deleted. hash: " << hash.getHex();
    return ss.str();
}

cli::result_t cmd_signingrequest(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 2)
        return "signingrequest <db file> <tx hash> - gets signing request for transaction with missing signatures.";

    Vault vault(params[0], false);
    uchar_vector hash(params[1]);

    SigningRequest req = vault.getSigningRequest(hash, true);
    vector<string>keychain_names;
    vector<string>keychain_hashes;
    for (auto& keychain_pair: req.keychain_info())
    {
        keychain_names.push_back(keychain_pair.first);
        keychain_hashes.push_back(uchar_vector(keychain_pair.second).getHex());
    }
    string rawtx_str = uchar_vector(req.rawtx()).getHex();

    stringstream ss;
    ss << "signatures needed: " << req.sigs_needed() << endl
       << "keychain names:    " << stdutils::delimited_list(keychain_names, ", ") << endl
       << "keychain hashes:   " << stdutils::delimited_list(keychain_hashes, ", ") << endl
       << "raw tx:            " << rawtx_str;
    return ss.str();
}

// TODO: do something with passphrase
cli::result_t cmd_signtx(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 4)
        return "signtx <db file> <tx hash> <keychain> <passphrase> - add signatures to transaction for specified keychain.";

    Vault vault(params[0], false);
    vault.unlockKeychainChainCode(params[2], secure_bytes_t());
    vault.unlockKeychainPrivateKey(params[2], secure_bytes_t());

    stringstream ss;
    if (vault.signTx(uchar_vector(params[1]), true))
    {
        ss << "Signatures added.";
    }
    else
    {
        ss << "No signatures were added.";
    }
    return ss.str();
}

// Blockchain operations
cli::result_t cmd_rawblockheader(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 6)
        return "rawblockheader <version> <previous blocks> <merkle root> <timestamp> <bits> <nonce> - construct a raw block header.";

    uint32_t version = strtoull(params[0].c_str(), NULL, 0);
    uchar_vector prevblockhash(params[1]); prevblockhash.reverse();
    uchar_vector merkleroot(params[2]); merkleroot.reverse();
    uint32_t timestamp = strtoull(params[3].c_str(), NULL, 0);
    uint32_t bits = strtoull(params[4].c_str(), NULL, 0);
    uint32_t nonce = strtoull(params[5].c_str(), NULL, 0);
    Coin::CoinBlockHeader header(version, prevblockhash, merkleroot, timestamp, bits, nonce);

    return header.getSerialized().getHex();
}

cli::result_t cmd_rawmerkleblock(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() < 4)
        return "rawmerkleblock <raw block header> <nTxs> <nHashes> [<hash 1> <hash 2> ...] <flags> - construct a raw merkle block.";

    uchar_vector rawblockheader(params[0]);
    Coin::CoinBlockHeader header(rawblockheader);

    uint32_t nTxs = strtoull(params[1].c_str(), NULL, 0);
    uint32_t nHashes = strtoull(params[2].c_str(), NULL, 0);
    if (params.size() != nHashes + 4)
        return "Invalid hash count.";

    vector<uchar_vector> hashes;
    for (size_t i = 3; i < nHashes + 3; i++)
    {
        uchar_vector hash(params[i]);
        hash.reverse();
        hashes.push_back(hash); 
    }

    uchar_vector flags(params[params.size() - 1]);

    Coin::MerkleBlock merkleblock(header, nTxs, hashes, flags);

    return merkleblock.getSerialized().getHex();
}

cli::result_t cmd_insertrawmerkleblock(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() < 2 || params.size() > 3)
        return "insertrawmerkleblock <db file> <raw merkleblock> <height = 0> - insert raw merkleblock into database.";

    uint32_t height = params.size() > 2 ? strtoull(params[2].c_str(), NULL, 0) : 0;

    uchar_vector rawmerkleblock(params[1]);
    std::shared_ptr<MerkleBlock> merkleblock(new MerkleBlock());
    merkleblock->fromCoinClasses(rawmerkleblock, height);

    Vault vault(params[0], false);
    bool rval = (bool)vault.insertMerkleBlock(merkleblock);

    stringstream ss;
    ss << "Merkleblock " << uchar_vector(merkleblock->blockheader()->hash()).getHex() << (rval ? " " : " not ") << "inserted.";
    return ss.str();
}

cli::result_t cmd_randombytes(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 1)
        return "randombytes <length> - output random bytes in hex.";

    uchar_vector bytes = random_bytes(strtoul(params[0].c_str(), NULL, 0));
    return bytes.getHex();
}

int main(int argc, char* argv[])
{
    INIT_LOGGER("debug.log");

    cli::command_map cmds("CoinDB by Eric Lombrozo v0.2.2");

    // Global operations
    cmds.add("create", &cmd_create);
    cmds.add("info", &cmd_info);

    // Keychain operations
    cmds.add("keychainexists", &cmd_keychainexists);
    cmds.add("newkeychain", &cmd_newkeychain);
    //cmds.add("erasekeychain", &cmd_erasekeychain);
    cmds.add("renamekeychain", &cmd_renamekeychain);
    cmds.add("keychaininfo", &cmd_keychaininfo);
    cmds.add("listkeychains", &cmd_listkeychains);
    cmds.add("exportkeychain", &cmd_exportkeychain);
    cmds.add("importkeychain", &cmd_importkeychain);

    // Account operations
    cmds.add("accountexists", &cmd_accountexists);
    cmds.add("newaccount", &cmd_newaccount);
    cmds.add("renameaccount", &cmd_renameaccount);
    cmds.add("accountinfo", &cmd_accountinfo);
    cmds.add("listaccounts", &cmd_listaccounts);
    cmds.add("exportaccount", &cmd_exportaccount);
    cmds.add("importaccount", &cmd_importaccount);
    cmds.add("newaccountbin", &cmd_newaccountbin);
    cmds.add("issuescript", &cmd_issuescript);
    cmds.add("listscripts", &cmd_listscripts);
    cmds.add("listtxouts", &cmd_listtxouts);
    cmds.add("refillaccountpool", &cmd_refillaccountpool);

    // Tx operations
    cmds.add("txinfo", &cmd_txinfo);
    cmds.add("insertrawtx", &cmd_insertrawtx);
    cmds.add("newrawtx", &cmd_newrawtx);
    cmds.add("deletetx", &cmd_deletetx);
    cmds.add("signingrequest", &cmd_signingrequest);
    cmds.add("signtx", &cmd_signtx);

    // Blockchain operations
    cmds.add("rawblockheader", &cmd_rawblockheader);
    cmds.add("rawmerkleblock", &cmd_rawmerkleblock);
    cmds.add("insertrawmerkleblock", &cmd_insertrawmerkleblock);

    // Miscellaneous
    cmds.add("randombytes", &cmd_randombytes);

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

