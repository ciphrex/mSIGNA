///////////////////////////////////////////////////////////////////////////////
//
// main.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.
//
// vaultd - headless daemon with WebSockets API
//

#include <WebSocketServer.h>
#include <cli.hpp>

#include <formatting.h>

#include <Vault.h>
#include <Schema-odb.hxx>

#include <random.h>

#include <logger.h>

#include <Base58Check.h>

#include <thread>
#include <chrono>

#include <iostream>
#include <sstream>
#include <ctime>
#include <functional>

#include <signal.h>

using namespace std;
using namespace odb::core;
using namespace CoinDB;

const string WS_PORT = "12345";

bool g_bShutdown = false;

void finish(int sig)
{
    LOGGER(debug) << "Stopping..." << endl;
    g_bShutdown = true;
}

// Global operations
cli::result_t cmd_create(const cli::params_t& params)
{
    Vault vault(params[0], true);

    stringstream ss;
    ss << "Vault " << params[0] << " created.";
    return ss.str();
}

cli::result_t cmd_info(const cli::params_t& params)
{
    Vault vault(params[0], false);
    uint32_t schema_version = vault.getSchemaVersion();
    uint32_t horizon_timestamp = vault.getHorizonTimestamp();

    stringstream ss;
    ss << "filename:            " << params[0] << endl
       << "schema version:      " << schema_version << endl
       << "horizon timestamp:   " << horizon_timestamp; 
    return ss.str();
}

// Keychain operations
cli::result_t cmd_keychainexists(const cli::params_t& params)
{
    Vault vault(params[0], false);
    bool bExists = vault.keychainExists(params[1]);

    stringstream ss;
    ss << (bExists ? "true" : "false");
    return ss.str();
}

cli::result_t cmd_newkeychain(const cli::params_t& params)
{
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
cli::result_t cmd_renamekeychain(const cli::params_t& params)
{
    Vault vault(params[0], false);
    vault.renameKeychain(params[1], params[2]);

    stringstream ss;
    ss << "Keychain " << params[1] << " renamed to " << params[2] << ".";
    return ss.str();
}

cli::result_t cmd_keychaininfo(const cli::params_t& params)
{
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

cli::result_t cmd_keychains(const cli::params_t& params)
{
    std::string account_name;
    if (params.size() > 1 && params[1] != "@all") account_name = params[1];

    bool show_hidden = params.size() > 2 && params[2] == "true";

    Vault vault(params[0], false);
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

    Vault vault(params[0], false);
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

    Vault vault(params[0], false);
    vault.exportKeychain(params[1], output_file, export_privkey);

    stringstream ss;
    ss << (export_privkey ? "Private" : "Public") << " keychain " << params[1] << " exported to " << output_file << ".";
    return ss.str();
}

cli::result_t cmd_importkeychain(const cli::params_t& params)
{
    bool import_privkey = params.size() > 2 ? (params[2] == "true") : true;

    Vault vault(params[0], false);
    std::shared_ptr<Keychain> keychain = vault.importKeychain(params[1], import_privkey);

    stringstream ss;
    ss << (import_privkey ? "Private" : "Public") << " keychain " << keychain->name() << " imported from " << params[1] << ".";
    return ss.str();
}

cli::result_t cmd_exportbip32(const cli::params_t& params)
{
    bool export_privkey = params.size() > 2;

    Vault vault(params[0], false);
    vault.unlockChainCodes(uchar_vector("1234"));
    if (export_privkey)
    {
        secure_bytes_t unlock_key = sha256_2(params[2]);
        vault.unlockKeychain(params[1], unlock_key);
    }
    secure_bytes_t extkey = vault.getKeychainExtendedKey(params[1], export_privkey);

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

    Vault vault(params[0], false);
    std::shared_ptr<Keychain> keychain = vault.importKeychainExtendedKey(params[1], extkey, import_privkey, lock_key);

    stringstream ss;
    ss << (keychain->isPrivate() ? "Private" : "Public") << " keychain " << keychain->name() << " imported from BIP32.";
    return ss.str();
}

// Account operations
cli::result_t cmd_accountexists(const cli::params_t& params)
{
    Vault vault(params[0], false);
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

    Vault vault(params[0], false);
    vault.unlockChainCodes(secure_bytes_t());
    vault.newAccount(params[1], minsigs, keychain_names);

    stringstream ss;
    ss << "Added account " << params[1] << " to vault " << params[0] << ".";
    return ss.str();
}

cli::result_t cmd_renameaccount(const cli::params_t& params)
{
    Vault vault(params[0], false);
    vault.renameAccount(params[1], params[2]);

    stringstream ss;
    ss << "Renamed account " << params[1] << " to " << params[2] << ".";
    return ss.str();
}

cli::result_t cmd_accountinfo(const cli::params_t& params)
{
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

cli::result_t cmd_listaccounts(const cli::params_t& params)
{
    Vault vault(params[0], false);
    vector<AccountInfo> accounts = vault.getAllAccountInfo();

    stringstream ss;
    ss << formattedAccountHeader();
    for (auto& account: accounts)
        ss << endl << formattedAccount(account);
    return ss.str();
}

cli::result_t cmd_exportaccount(const cli::params_t& params)
{
    Vault vault(params[0], false);

    secure_bytes_t exportChainCodeUnlockKey;
    if (params.size() > 2 && !params[2].empty())
        exportChainCodeUnlockKey = sha256_2(params[2]);

    if (params.size() > 3 && !params[3].empty())
        vault.unlockChainCodes(sha256_2(params[3]));

    std::string output_file = params.size() > 4 ? params[4] : (params[1] + ".account");
    vault.exportAccount(params[1], output_file, true, exportChainCodeUnlockKey);

    stringstream ss;
    ss << "Account " << params[1] << " exported to " << output_file << ".";
    return ss.str();
}

cli::result_t cmd_importaccount(const cli::params_t& params)
{
    Vault vault(params[0], false);

    unsigned int privkeycount = 1;

    secure_bytes_t chainCodeUnlockKey;
    if (params.size() > 2 && !params[2].empty())
        chainCodeUnlockKey = sha256_2(params[2]);

    if (params.size() > 3 && !params[3].empty())
        vault.unlockChainCodes(sha256_2(params[3]));

    std::shared_ptr<Account> account = vault.importAccount(params[1], privkeycount, chainCodeUnlockKey);

    stringstream ss;
    ss << "Account " << account->name() << " imported from " << params[1] << ".";
    return ss.str();
}

cli::result_t cmd_newaccountbin(const cli::params_t& params)
{
    Vault vault(params[0], false);
    AccountInfo accountInfo = vault.getAccountInfo(params[1]);
    vault.unlockChainCodes(secure_bytes_t());
    vault.addAccountBin(params[1], params[2]);

    stringstream ss;
    ss << "Account bin " << params[2] << " added to account " << params[1] << ".";
    return ss.str(); 
}

cli::result_t cmd_listbins(const cli::params_t& params)
{
    Vault vault(params[0], false);
    vector<AccountBinView> bins = vault.getAllAccountBinViews();

    stringstream ss;
    ss << formattedAccountBinViewHeader();
    for (auto& bin: bins)
        ss << endl << formattedAccountBinView(bin);
    return ss.str();
}

cli::result_t cmd_issuescript(const cli::params_t& params)
{
    Vault vault(params[0], false);
    std::string account_name;
    if (params[1] != "@null") account_name = params[1];
    std::string bin_name = params.size() > 2 ? params[2] : std::string(DEFAULT_BIN_NAME);
    std::string label = params.size() > 3 ? params[3] : std::string("");
    std::shared_ptr<SigningScript> script = vault.issueSigningScript(account_name, bin_name, label);

    std::string address = getAddressFromScript(script->txoutscript());

    stringstream ss;
    ss << "account:     " << params[1] << endl
       << "account bin: " << bin_name << endl
       << "label:       " << label << endl
       << "script:      " << uchar_vector(script->txoutscript()).getHex() << endl
       << "address:     " << address;
    return ss.str(); 
}

cli::result_t cmd_listscripts(const cli::params_t& params)
{
    std::string account_name = params.size() > 1 ? params[1] : std::string("@all");
    if (account_name == "@all") account_name = "";

    std::string bin_name = params.size() > 2 ? params[2] : std::string("@all");
    if (bin_name == "@all") bin_name = "";

    int flags = params.size() > 3 ? (int)strtoul(params[3].c_str(), NULL, 0) : ((int)SigningScript::ISSUED | (int)SigningScript::USED);
    
    Vault vault(params[0], false);
    vector<SigningScriptView> scriptViews = vault.getSigningScriptViews(account_name, bin_name, flags);

    stringstream ss;
    ss << formattedScriptHeader();
    for (auto& scriptView: scriptViews)
        ss << endl << formattedScript(scriptView);
    return ss.str();
}

cli::result_t cmd_history(const cli::params_t& params)
{
    std::string account_name = params.size() > 1 ? params[1] : std::string("@all");
    if (account_name == "@all") account_name = "";

    std::string bin_name = params.size() > 2 ? params[2] : std::string("@all");
    if (bin_name == "@all") bin_name = "";

    bool hide_change = params.size() > 3 ? params[3] == "true" : true;
    
    Vault vault(params[0], false);
    uint32_t best_height = vault.getBestHeight();
    vector<TxOutView> txOutViews = vault.getTxOutViews(account_name, bin_name, TxOut::ROLE_BOTH, TxOut::BOTH, Tx::ALL, hide_change);
    stringstream ss;
    ss << formattedTxOutViewHeader();
    for (auto& txOutView: txOutViews)
        ss << endl << formattedTxOutView(txOutView, best_height);
    return ss.str();
}

cli::result_t cmd_refillaccountpool(const cli::params_t& params)
{
    Vault vault(params[0], false);
    AccountInfo accountInfo = vault.getAccountInfo(params[1]);
    vault.unlockChainCodes(secure_bytes_t());
    vault.refillAccountPool(params[1]);

    stringstream ss;
    ss << "Refilled account pool for account " << params[1] << ".";
    return ss.str();
}

// Account bin operations
cli::result_t cmd_exportbin(const cli::params_t& params)
{
    Vault vault(params[0], false);

    string export_name = params.size() > 3 ? params[3] : (params[1].empty() ? params[2] : params[1] + "-" + params[2]);
    secure_bytes_t exportChainCodeUnlockKey;
    if (params.size() > 4 && !params[4].empty())
        exportChainCodeUnlockKey = sha256_2(params[4]);

    vault.unlockChainCodes(secure_bytes_t());

    string output_file = params.size() > 5 ? params[5] : (export_name + ".bin");
    vault.exportAccountBin(params[1], params[2], export_name, output_file, exportChainCodeUnlockKey);

    stringstream ss;
    ss << "Account bin " << export_name << " exported to " << output_file << ".";
    return ss.str();
}

cli::result_t cmd_importbin(const cli::params_t& params)
{
    Vault vault(params[0], false);

    secure_bytes_t importChainCodeUnlockKey;
    if (params.size() > 2 && !params[2].empty())
        importChainCodeUnlockKey = sha256_2(params[2]);

    vault.unlockChainCodes(uchar_vector("1234"));

    std::shared_ptr<AccountBin> bin = vault.importAccountBin(params[1], importChainCodeUnlockKey);

    stringstream ss;
    ss << "Account bin " << bin->name() << " imported from " << params[1] << ".";
    return ss.str();
}

// Tx operations
cli::result_t cmd_txinfo(const cli::params_t& params)
{
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

cli::result_t cmd_insertrawtx(const cli::params_t& params)
{
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

cli::result_t cmd_newrawtx(const cli::params_t& params)
{
    using namespace CoinQ::Script;
    const size_t MAX_VERSION_LEN = 2;

    Vault vault(params[0], false);

    // Get outputs
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

    std::shared_ptr<Tx> tx = vault.createTx(params[1], version, locktime, txouts, fee, 1, true);
    return uchar_vector(tx->raw()).getHex();
}

cli::result_t cmd_deletetx(const cli::params_t& params)
{
    Vault vault(params[0], false);
    uchar_vector hash(params[1]);
    vault.deleteTx(hash);

    stringstream ss;
    ss << "Tx deleted. hash: " << hash.getHex();
    return ss.str();
}

cli::result_t cmd_signingrequest(const cli::params_t& params)
{
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
cli::result_t cmd_signtx(const cli::params_t& params)
{
    Vault vault(params[0], false);
    vault.unlockChainCodes(uchar_vector("1234"));
    vault.unlockKeychain(params[2], secure_bytes_t());

    stringstream ss;
    std::vector<std::string> keychain_names;
    keychain_names.push_back(params[2]);
    if (vault.signTx(uchar_vector(params[1]), keychain_names, true))
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
cli::result_t cmd_bestheight(const cli::params_t& params)
{
    Vault vault(params[0], false);
    uint32_t best_height = vault.getBestHeight();

    stringstream ss;
    ss << best_height;
    return ss.str();
}

cli::result_t cmd_horizonheight(const cli::params_t& params)
{
    Vault vault(params[0], false);
    uint32_t horizon_height = vault.getHorizonHeight();

    stringstream ss;
    ss << horizon_height;
    return ss.str();
}

cli::result_t cmd_horizontimestamp(const cli::params_t& params)
{
    bool use_gmt = params.size() > 1 && params[1] == "true";

    Vault vault(params[0], false);
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

    Vault vault(params[0], false);
    std::shared_ptr<BlockHeader> blockheader = vault.getBlockHeader(height);

    return blockheader->toCoinClasses().toIndentedString();
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
    merkleblock->fromCoinClasses(rawmerkleblock, height);

    Vault vault(params[0], false);
    bool rval = (bool)vault.insertMerkleBlock(merkleblock);

    stringstream ss;
    ss << "Merkle block " << uchar_vector(merkleblock->blockheader()->hash()).getHex() << (rval ? " " : " not ") << "inserted.";
    return ss.str();
}

cli::result_t cmd_deleteblock(const cli::params_t& params)
{
    uint32_t height = strtoull(params[1].c_str(), NULL, 0);
    Vault vault(params[0], false);
    unsigned int count = vault.deleteMerkleBlock(height);

    stringstream ss;
    ss << count << " merkle blocks deleted.";
    return ss.str();
}

cli::result_t cmd_randombytes(const cli::params_t& params)
{
    uchar_vector bytes = random_bytes(strtoul(params[0].c_str(), NULL, 0));
    return bytes.getHex();
}

// WebSocket callbacks
void openCallback(WebSocket::Server& server, websocketpp::connection_hdl hdl)
{
    LOGGER(debug) << "Client " << hdl.lock().get() << " connected." << endl;
}

void closeCallback(WebSocket::Server& server, websocketpp::connection_hdl hdl)
{
    LOGGER(debug) << "Client " << hdl.lock().get() << " disconnected." << endl;
}

using namespace cli;
Shell shell("vaultd by Eric Lombrozo v0.0.1");

void requestCallback(WebSocket::Server& server, const WebSocket::Server::client_request_t& req)
{
    JsonRpc::Response response;

    const string& cmdname = req.second.getMethod();
    params_t params;
    for (auto& param: req.second.getParams()) { params.push_back(param.get_str()); }
    try
    {
        result_t result = shell.exec(cmdname, params);
        response.setResult(result, req.second.getId());
    }
    catch (const std::exception& e)
    {
        response.setError(e.what(), req.second.getId());        
    }

    server.send(req.first, response);
}

int main(int argc, char* argv[])
{
    INIT_LOGGER("vaultd.log");

    signal(SIGINT, &finish);

    // Global operations
    shell.add(command(&cmd_create, "create", "create a new vault", command::params(1, "db file")));
    shell.add(command(&cmd_info, "info", "display general information about file", command::params(1, "db file")));

    // Keychain operations
    shell.add(command(&cmd_keychainexists, "keychainexists", "check if a keychain exists", command::params(1, "db file")));
    shell.add(command(&cmd_newkeychain, "newkeychain", "create a new keychain", command::params(2, "db file", "keychain name")));
    shell.add(command(&cmd_renamekeychain, "renamekeychain", "rename a keychain", command::params(3, "db file", "old name", "new name")));
    shell.add(command(&cmd_keychaininfo, "keychaininfo", "display keychain information about a specific keychain", command::params(2, "db file", "keychain name")));
    shell.add(command(&cmd_keychains, "keychains", "display keychains", command::params(1, "db file"), command::params(2, "account = @all", "show hidden = false")));
    shell.add(command(&cmd_exportkeychain, "exportkeychain", "export a keychain to file", command::params(2, "db file", "keychain name"), command::params(1, "export private key = false")));
    shell.add(command(&cmd_importkeychain, "importkeychain", "import a keychain from file", command::params(2, "db file", "keychain file"), command::params(1, "import private key = true")));
    shell.add(command(&cmd_exportbip32, "exportbip32", "export a keychain in BIP32 extended key format", command::params(2, "db file", "keychain name"), command::params(1, "passphrase")));
    shell.add(command(&cmd_importbip32, "importbip32", "import a keychain in BIP32 extended key format", command::params(3, "db file", "keychain name", "BIP32"), command::params(1, "passphrase")));

    // Account operations
    shell.add(command(&cmd_accountexists, "accountexists", "check if an account exists", command::params(2, "db file", "account name")));
    shell.add(command(&cmd_newaccount, "newaccount", "create a new account using specified keychains", command::params(4, "db file", "account name", "minsigs", "keychain 1"), command::params(3, "keychain 2", "keychain 3", "...")));
    shell.add(command(&cmd_renameaccount, "renameaccount", "rename an account", command::params(3, "db file", "old name", "new name")));
    shell.add(command(&cmd_accountinfo, "accountinfo", "display account information", command::params(2, "db file", "account name")));
    shell.add(command(&cmd_listaccounts, "listaccounts", "display list of accounts", command::params(1, "db file")));
    shell.add(command(&cmd_exportaccount, "exportaccount", "export account to file", command::params(2, "db file", "account name"), command::params(3, "export chain code passphrase", "native chain code passphrase", "output file = *.account")));
    shell.add(command(&cmd_importaccount, "importaccount", "import account from file", command::params(2, "db file", "account file"), command::params(2, "import chain code passphrase", "native chain code passphrase"))); 
    shell.add(command(&cmd_newaccountbin, "newaccountbin", "add a new account bin", command::params(3, "db file", "account name", "bin name")));
    shell.add(command(&cmd_issuescript, "issuescript", "issue a new signing script", command::params(2, "db file", "account name"), command::params(1, (std::string("bin name = ") + DEFAULT_BIN_NAME).c_str())));
    shell.add(command(&cmd_listscripts, "listscripts", "display list of signing scripts (flags: UNUSED=1, CHANGE=2, PENDING=4, RECEIVED=8, CANCELED=16)", command::params(1, "db file"),
        command::params(3, "account name = @all", "bin name = @all", "flags = PENDING | RECEIVED")));
    shell.add(command(&cmd_history, "history", "display transaction history", command::params(1, "db file"), command::params(3, "account name = @all", "bin name = @all", "hide change = true")));
    shell.add(command(&cmd_refillaccountpool, "refillaccountpool", "refill signing script pool for account", command::params(2, "db file", "account name")));

    // Account bin operations
    shell.add(command(&cmd_listbins, "listbins", "display list of bins", command::params(1, "db file")));
    shell.add(command(&cmd_exportbin, "exportbin", "export account bin to file", command::params(3, "db file", "account name", "bin name"), command::params(3, "export name = account_name-bin_name", "export chain code passphrase", "output file = *.bin")));
    shell.add(command(&cmd_importbin, "importbin", "import account bin from file", command::params(2, "db file", "bin file"), command::params(1, "import chain code passphrase")));

    // Tx operations
    shell.add(command(&cmd_txinfo, "txinfo", "display transaction information", command::params(2, "db file", "tx hash"), command::params(1, "raw hex = false")));
    shell.add(command(&cmd_insertrawtx, "insertrawtx", "insert a raw hex transaction into database", command::params(2, "db file", "tx raw hex")));
    shell.add(command(&cmd_newrawtx, "newrawtx", "create a new raw transaction", command::params(4, "db file", "account name", "address 1", "value 1"), command::params(6, "address 2", "value 2", "...", "fee = 0", "version = 1", "locktime = 0")));
    shell.add(command(&cmd_deletetx, "deletetx", "delete a transaction", command::params(2, "db file", "tx hash")));
    shell.add(command(&cmd_signingrequest, "signingrequest", "gets signing request for transaction with missing signatures", command::params(2, "db file", "tx hash")));
    shell.add(command(&cmd_signtx, "signtx", "add signatures to transaction for specified keychain", command::params(4, "db file", "tx hash", "keychain name", "passphrase")));

    // Blockchain operations
    shell.add(command(&cmd_bestheight, "bestheight", "display the best block height", command::params(1, "db file")));
    shell.add(command(&cmd_horizonheight, "horizonheight", "display height of first stored block", command::params(1, "db file")));
    shell.add(command(&cmd_horizontimestamp, "horizontimestamp", "display timestamp minimum for first stored block", command::params(1, "db file"), command::params(1, "use gmt = false")));
    shell.add(command(&cmd_blockinfo, "blockinfo", "display block information", command::params(2, "db file", "height")));    
    shell.add(command(&cmd_rawblockheader, "rawblockheader", "construct a raw block header", command::params(6, "version", "previous block hash", "merkle root", "timestamp", "bits", "nonce")));
    shell.add(command(&cmd_rawmerkleblock, "rawmerkleblock", "construct a raw merkle block", command::params(4, "raw block header", "flags", "nTxs", "nHashes"), command::params(3, "hash 1", "hash 2", "...")));
    shell.add(command(&cmd_insertrawmerkleblock, "insertrawmerkleblock", "insert raw merkle block into database", command::params(2, "db file", "raw merkle block"), command::params(1, "height = 0")));
    shell.add(command(&cmd_deleteblock, "deleteblock", "delete merkle block including all descendants", command::params(1, "db file"), command::params(1, "height = 0")));

    // Miscellaneous
    shell.add(command(&cmd_randombytes, "randombytes", "output random bytes in hex", command::params(1, "length")));

    WebSocket::Server wsServer(WS_PORT);
    wsServer.setOpenCallback(&openCallback);
    wsServer.setCloseCallback(&closeCallback);
    wsServer.setRequestCallback(&requestCallback);

    try 
    {
        LOGGER(debug) << "Starting websocket server on port " << WS_PORT << "..." << endl;
        wsServer.start();
        LOGGER(debug) << "Websocket server started." << endl;
    }
    catch (const std::exception& e)
    {
        LOGGER(error) << "Error starting websocket server: " << e.what() << endl;
        return 1;
    }

    while (!g_bShutdown) { std::this_thread::sleep_for(std::chrono::microseconds(200)); }

    try
    {
        LOGGER(debug) << "Stopping websocket server..." << endl;
        wsServer.stop();
        LOGGER(debug) << "Websocket server stopped." << endl;
    }
    catch (const std::exception& e)
    {
        LOGGER(error) << "Error stopping websocket server: " << e.what() << endl;
        return 2;
    }

    return 0;
}

