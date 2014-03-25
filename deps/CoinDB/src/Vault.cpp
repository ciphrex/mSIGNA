///////////////////////////////////////////////////////////////////////////////
//
// Vault.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.
//

#include "Vault.h"

#include <CoinQ_script.h>
#include <CoinQ_blocks.h>

#include "Database.hxx"

#include "../odb/Schema-odb.hxx"

#include <odb/transaction.hxx>
#include <odb/session.hxx>

#include <CoinKey.h>
#include <MerkleTree.h>

#include <sstream>
#include <fstream>
#include <algorithm>

#include <logger.h>

const std::size_t MAXSQLCLAUSES = 500;

const unsigned long LOOKAHEAD = 100;

using namespace CoinDB;

/*
 * class Vault implementation
*/
Vault::Vault(int argc, char** argv, bool create, uint32_t version)
    : db_(open_database(argc, argv, create))
{
    LOGGER(trace) << "Opened Vault" << std::endl;
//    if (create) setVersion(version);
}

#if defined(DATABASE_SQLITE)
Vault::Vault(const std::string& filename, bool create, uint32_t version)
    : db_(openDatabase(filename, create))
{
    LOGGER(trace) << "Opened Vault - filename: " << filename << " create: " << (create ? "true" : "false") << " version: " << version << std::endl;
//    if (create) setVersion(version);
}
#endif

bool Vault::keychainExists(const std::string& keychain_name) const
{
    LOGGER(trace) << "Vault::keychainExists(" << keychain_name << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::transaction t(db_->begin());
    odb::result<Keychain> r(db_->query<Keychain>(odb::query<Keychain>::name == keychain_name));

    return !r.empty();
}

std::shared_ptr<Keychain> Vault::newKeychain(const std::string& keychain_name, const secure_bytes_t& entropy, const secure_bytes_t& lockKey, const bytes_t& salt)
{
    LOGGER(trace) << "Vault::newKeychain(" << keychain_name << ", ...)" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session session;
    odb::core::transaction t(db_->begin());
    odb::result<Keychain> r(db_->query<Keychain>(odb::query<Keychain>::name == keychain_name));
    if (!r.empty()) throw KeychainAlreadyExistsException(keychain_name);

    std::shared_ptr<Keychain> keychain(new Keychain(keychain_name, entropy, lockKey, salt));
    persistKeychain_unwrapped(keychain);
    t.commit();

    return keychain;
}

void Vault::renameKeychain(const std::string& old_name, const std::string& new_name)
{
    LOGGER(trace) << "Vault::renameKeychain(" << old_name << ", " << new_name << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session session;
    odb::core::transaction t(db_->begin());

    odb::result<Keychain> keychain_r(db_->query<Keychain>(odb::query<Keychain>::name == old_name));
    if (keychain_r.empty()) throw KeychainNotFoundException(old_name);

    if (old_name == new_name) return;

    odb::result<Keychain> new_keychain_r(db_->query<Keychain>(odb::query<Keychain>::name == new_name));
    if (!new_keychain_r.empty()) throw KeychainAlreadyExistsException(new_name);

    std::shared_ptr<Keychain> keychain(keychain_r.begin().load());
    keychain->name(new_name);

    db_->update(keychain);
    t.commit();
}

void Vault::persistKeychain_unwrapped(std::shared_ptr<Keychain> keychain)
{
    if (keychain->parent())
        db_->update(keychain->parent());

    db_->persist(keychain);
}

void Vault::tryUnlockAccountChainCodes_unwrapped(std::shared_ptr<Account> account)
{
    std::set<std::string> locked_keychains;
    for (auto& keychain: account->keychains())
    {
        const auto& it = mapChainCodeUnlock.find(keychain->name());
        if (it == mapChainCodeUnlock.end())
        {
            locked_keychains.insert(keychain->name());
        }
        else
        {
            if (!keychain->unlockChainCode(it->second))
            {
                mapChainCodeUnlock.erase(keychain->name());
                locked_keychains.insert(keychain->name());
            }
        }
    }
    if (!locked_keychains.empty()) throw AccountChainCodeLockedException(account->name(), locked_keychains);
}

std::shared_ptr<Keychain> Vault::getKeychain_unwrapped(const std::string& keychain_name) const
{
    odb::result<Keychain> r(db_->query<Keychain>(odb::query<Keychain>::name == keychain_name));
    if (r.empty()) throw KeychainNotFoundException(keychain_name);

    std::shared_ptr<Keychain> keychain(r.begin().load());
    return keychain;
}

std::shared_ptr<Keychain> Vault::getKeychain(const std::string& keychain_name) const
{
    LOGGER(trace) << "Vault::getKeychain(" << keychain_name << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::transaction t(db_->begin());
    return getKeychain_unwrapped(keychain_name);
}

std::vector<std::shared_ptr<Keychain>> Vault::getAllKeychains(bool root_only) const
{
    LOGGER(trace) << "Vault::getAllKeychains()" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::transaction t(db_->begin());
    odb::result<Keychain> r;
    if (root_only)
        r = db_->query<Keychain>(odb::query<Keychain>::parent.is_null());
    else
        r = db_->query<Keychain>();

    std::vector<std::shared_ptr<Keychain>> keychains;
    for (auto& keychain: r) { keychains.push_back(keychain.get_shared_ptr()); }
    return keychains;
}

void Vault::lockAllKeychainChainCodes()
{
    LOGGER(trace) << "Vault::lockAllKeychainChainCodes()" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    mapChainCodeUnlock.clear();
}

void Vault::lockKeychainChainCode(const std::string& keychain_name)
{
    LOGGER(trace) << "Vault::lockKeychainChainCode(" << keychain_name << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    mapChainCodeUnlock.erase(keychain_name);
}

void Vault::unlockKeychainChainCode(const std::string& keychain_name, const secure_bytes_t& unlock_key)
{
    LOGGER(trace) << "Vault::unlockKeychainChainCode(" << keychain_name << ", ?)" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    std::shared_ptr<Keychain> keychain = getKeychain_unwrapped(keychain_name);
    if (keychain->unlockChainCode(unlock_key))
        mapChainCodeUnlock[keychain_name] = unlock_key;
}
 
void Vault::lockAllKeychainPrivateKeys()
{
    LOGGER(trace) << "Vault::lockAllKeychainPrivateKeys()" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    mapPrivateKeyUnlock.clear();
}

void Vault::lockKeychainPrivateKey(const std::string& keychain_name)
{
    LOGGER(trace) << "Vault::lockKeychainPrivateKey(" << keychain_name << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    mapPrivateKeyUnlock.erase(keychain_name);
}

void unlockKeychainPrivateKey(const std::string& keychain_name, const secure_bytes_t& unlock_key)
{
}


// Account operations
bool Vault::accountExists(const std::string& account_name) const
{
    LOGGER(trace) << "Vault::accountExists(" << account_name << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::transaction t(db_->begin());
    odb::result<Account> r(db_->query<Account>(odb::query<Account>::name == account_name));

    return !r.empty();
}

void Vault::newAccount(const std::string& account_name, unsigned int minsigs, const std::vector<std::string>& keychain_names, uint32_t unused_pool_size, uint32_t time_created)
{
    LOGGER(trace) << "Vault::newAccount(" << account_name << ", " << minsigs << " of [" << stdutils::delimited_list(keychain_names, ", ") << "], " << unused_pool_size << ", " << time_created << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    odb::result<Account> r(db_->query<Account>(odb::query<Account>::name == account_name));
    if (!r.empty()) throw AccountAlreadyExistsException(account_name);

    KeychainSet keychains;
    for (auto& keychain_name: keychain_names)
    {
        odb::result<Keychain> r(db_->query<Keychain>(odb::query<Keychain>::name == keychain_name));
        if (r.empty()) throw KeychainNotFoundException(keychain_name);
        keychains.insert(r.begin().load());
    }

    std::shared_ptr<Account> account(new Account(account_name, minsigs, keychains, unused_pool_size, time_created));
    tryUnlockAccountChainCodes_unwrapped(account);
    db_->persist(account);

    // The first bin we create must be the change bin.
    std::shared_ptr<AccountBin> changeAccountBin= account->addBin(CHANGE_BIN_NAME);
    db_->persist(changeAccountBin);

    // The second bin we create must be the default bin.
    std::shared_ptr<AccountBin> defaultAccountBin = account->addBin(DEFAULT_BIN_NAME);
    db_->persist(defaultAccountBin);

    for (uint32_t i = 0; i < unused_pool_size; i++)
    {
        std::shared_ptr<SigningScript> changeSigningScript = changeAccountBin->newSigningScript();
        for (auto& key: changeSigningScript->keys()) { db_->persist(key); } 
        db_->persist(changeSigningScript);

        std::shared_ptr<SigningScript> defaultSigningScript = defaultAccountBin->newSigningScript();
        for (auto& key: defaultSigningScript->keys()) { db_->persist(key); }
        db_->persist(defaultSigningScript);
    }
    db_->update(changeAccountBin);
    db_->update(defaultAccountBin);
    db_->update(account);
    t.commit();
}

void Vault::renameAccount(const std::string& old_name, const std::string& new_name)
{
    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session session;
    odb::core::transaction t(db_->begin());

    odb::result<Account> account_r(db_->query<Account>(odb::query<Account>::name == old_name));
    if (account_r.empty()) throw std::runtime_error("Account not found.");

    if (old_name == new_name) return;

    odb::result<Account> new_account_r(db_->query<Account>(odb::query<Account>::name == new_name));
    if (!new_account_r.empty()) throw std::runtime_error("An account with that name already exists.");

    std::shared_ptr<Account> account(account_r.begin().load());
    account->name(new_name);

    db_->update(account);
    t.commit();
}

std::shared_ptr<Account> Vault::getAccount_unwrapped(const std::string& account_name) const
{
    odb::result<Account> r(db_->query<Account>(odb::query<Account>::name == account_name));
    if (r.empty()) throw AccountNotFoundException(account_name);

    std::shared_ptr<Account> account(r.begin().load());
    return account;
}

std::shared_ptr<Account> Vault::getAccount(const std::string& account_name) const
{
    LOGGER(trace) << "Vault::getAccount(" << account_name << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::transaction t(db_->begin());
    return getAccount_unwrapped(account_name);
}

AccountInfo Vault::getAccountInfo(const std::string& account_name) const
{
    LOGGER(trace) << "Vault::getAccountInfo(" << account_name << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    std::shared_ptr<Account> account = getAccount_unwrapped(account_name);
    return account->accountInfo();
}

std::vector<AccountInfo> Vault::getAllAccountInfo() const
{
    LOGGER(trace) << "Vault::getAllAccountInfo()" << std::endl;
 
    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    odb::result<Account> r(db_->query<Account>());
    std::vector<AccountInfo> accountInfoVector;
    for (auto& account: r) { accountInfoVector.push_back(account.accountInfo()); }
    return accountInfoVector;
}

uint64_t Vault::getAccountBalance(const std::string& account_name, unsigned int min_confirmations, int tx_flags) const
{
    LOGGER(trace) << "Vault::getAccountBalance(" << account_name << ", " << min_confirmations << ")" << std::endl;

    std::vector<Tx::status_t> tx_statuses = Tx::getStatusFlags(tx_flags);

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::transaction t(db_->begin());
    typedef odb::query<BalanceView> query_t;
    query_t query(query_t::Account::name == account_name && query_t::TxOut::status == TxOut::UNSPENT && query_t::Tx::status.in_range(tx_statuses.begin(), tx_statuses.end()));
    if (min_confirmations > 0)
    {
        odb::result<BestHeightView> height_r(db_->query<BestHeightView>());
        uint32_t best_height = height_r.empty() ? 0 : height_r.begin().load()->best_height;
        if (min_confirmations > best_height) return 0;
        query = (query && query_t::BlockHeader::height <= best_height + 1 - min_confirmations);
    }
    odb::result<BalanceView> r(db_->query<BalanceView>(query));
    return r.empty() ? 0 : r.begin().load()->balance;
}

std::shared_ptr<AccountBin> Vault::addAccountBin(const std::string& account_name, const std::string& bin_name)
{
    LOGGER(trace) << "Vault::addAccountBin(" << account_name << ", " << bin_name << ")" << std::endl;

    if (bin_name.empty() || bin_name[0] == '@') throw std::runtime_error("Invalid account bin name.");

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session s;
    odb::core::transaction t(db_->begin());

    bool binExists = true;
    try
    {
        getAccountBin_unwrapped(account_name, bin_name);
    }
    catch (const AccountBinNotFoundException& e)
    {
        binExists = false;
    }

    if (binExists) throw AccountBinAlreadyExistsException(account_name, bin_name);

    std::shared_ptr<Account> account = getAccount_unwrapped(account_name);
    tryUnlockAccountChainCodes_unwrapped(account);

    std::shared_ptr<AccountBin> bin = account->addBin(bin_name);
    db_->persist(bin);

    for (uint32_t i = 0; i < account->unused_pool_size(); i++)
    {
        std::shared_ptr<SigningScript> script = bin->newSigningScript();
        for (auto& key: script->keys()) { db_->persist(key); }
        db_->persist(script);
    }
    db_->update(bin);
    db_->update(account);
    t.commit();

    return bin;
}

std::shared_ptr<SigningScript> Vault::newAccountBinSigningScript_unwrapped(std::shared_ptr<AccountBin> bin, const std::string& label)
{
    try
    {
        refillAccountBinPool_unwrapped(bin);
    }
    catch (const AccountChainCodeLockedException& e)
    {
        LOGGER(debug) << "Vault::newAccountBinSigningScript_unwrapped(" << bin->account()->name() << "::" << bin->name() << ", " << label << ") - Chain code is locked so pool cannot be replenished." << std::endl;
    }

    // Get the next available unused signing script
    typedef odb::query<SigningScriptView> view_query;
    odb::result<SigningScriptView> view_result(db_->query<SigningScriptView>(
        (view_query::AccountBin::id == bin->id() && view_query::SigningScript::status == SigningScript::UNUSED) +
        "ORDER BY" + view_query::SigningScript::index + "LIMIT 1"));
    if (view_result.empty()) throw AccountBinOutOfScriptsException(bin->account()->name(), bin->name());

    std::shared_ptr<SigningScriptView> view(view_result.begin().load());

    typedef odb::query<SigningScript> script_query;
    odb::result<SigningScript> script_result(db_->query<SigningScript>(script_query::id == view->id));
    std::shared_ptr<SigningScript> script(script_result.begin().load());
    script->label(label);
    script->status(SigningScript::PENDING);
    db_->update(script);
    return script;
}

std::shared_ptr<SigningScript> Vault::newSigningScript(const std::string& account_name, const std::string& bin_name, const std::string& label)
{
    LOGGER(trace) << "Vault::newSigningScript(" << account_name << ", " << bin_name << ", " << label << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    std::shared_ptr<AccountBin> bin = getAccountBin_unwrapped(account_name, bin_name);
    std::shared_ptr<SigningScript> script = newAccountBinSigningScript_unwrapped(bin, label);
    t.commit();
    return script;
}

void Vault::refillAccountPool_unwrapped(std::shared_ptr<Account> account)
{
    for (auto& bin: account->bins()) { refillAccountBinPool_unwrapped(bin); }
}

void Vault::refillAccountPool(const std::string& account_name)
{
    LOGGER(trace) << "Vault::refillAccountPool(" << account_name << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    std::shared_ptr<Account> account = getAccount_unwrapped(account_name);
    refillAccountPool_unwrapped(account);
    t.commit();
}

void Vault::refillAccountBinPool_unwrapped(std::shared_ptr<AccountBin> bin)
{
    tryUnlockAccountChainCodes_unwrapped(bin->account());

    typedef odb::query<ScriptCountView> count_query;
    odb::result<ScriptCountView> count_result(db_->query<ScriptCountView>(count_query::AccountBin::id == bin->id() && count_query::SigningScript::status == SigningScript::UNUSED));
    uint32_t count = count_result.empty() ? 0 : count_result.begin().load()->count;

    uint32_t unused_pool_size = bin->account()->unused_pool_size();
    for (uint32_t i = count; i < unused_pool_size; i++)
    {
        std::shared_ptr<SigningScript> script = bin->newSigningScript();
        for (auto& key: script->keys()) { db_->persist(key); }
        db_->persist(script); 
    } 
    db_->update(bin);
}

std::vector<SigningScriptView> Vault::getSigningScriptViews(const std::string& account_name, const std::string& bin_name, int flags) const
{
    LOGGER(trace) << "Vault::getSigningScriptViews(" << account_name << ", " << bin_name << ", " << SigningScript::getStatusString(flags) << ")" << std::endl;

    std::vector<SigningScript::status_t> statusRange = SigningScript::getStatusFlags(flags);

    typedef odb::query<SigningScriptView> query_t;
    query_t query(query_t::SigningScript::status.in_range(statusRange.begin(), statusRange.end()));
    if (account_name != "@all") query = (query && query_t::Account::name == account_name);
    if (bin_name != "@all")     query = (query && query_t::AccountBin::name == bin_name);
    query += "ORDER BY" + query_t::Account::name + "ASC," + query_t::AccountBin::name + "ASC," + query_t::SigningScript::status + "DESC," + query_t::SigningScript::index + "ASC";

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session s;
    odb::core::transaction t(db_->begin());

    std::vector<SigningScriptView> views;
    odb::result<SigningScriptView> r(db_->query<SigningScriptView>(query));
    for (auto& view: r) { views.push_back(view); }
    return views;
}

std::vector<TxOutView> Vault::getTxOutViews(const std::string& account_name, const std::string& bin_name, int txout_status_flags, int tx_status_flags) const
{
    LOGGER(trace) << "Vault::getTxOutViews(" << account_name << ", " << bin_name << ", " << TxOut::getStatusString(txout_status_flags) << ", " << ", " << Tx::getStatusString(tx_status_flags) << ")" << std::endl;

    typedef odb::query<TxOutView> query_t;
    query_t query(query_t::receiving_account::id != 0 || query_t::sending_account::id != 0);
    if (account_name != "@all")                 query = (query && (query_t::sending_account::name == account_name || query_t::receiving_account::name == account_name));
    if (bin_name != "@all")                     query = (query && query_t::AccountBin::name == bin_name);

    std::vector<TxOut::status_t> txout_statuses = TxOut::getStatusFlags(txout_status_flags);
    query = (query && query_t::TxOut::status.in_range(txout_statuses.begin(), txout_statuses.end()));

    std::vector<Tx::status_t> tx_statuses = Tx::getStatusFlags(tx_status_flags);
    query = (query && query_t::Tx::status.in_range(tx_statuses.begin(), tx_statuses.end()));

    query += "ORDER BY" + query_t::BlockHeader::height + "DESC," + query_t::Tx::timestamp + "DESC," + query_t::Tx::id + "DESC";

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::transaction t(db_->begin());
    std::vector<TxOutView> views;
    odb::result<TxOutView> r(db_->query<TxOutView>(query));
    for (auto& view: r) { views.push_back(view); }
    return views;
}


// AccountBin operations
std::shared_ptr<AccountBin> Vault::getAccountBin_unwrapped(const std::string& account_name, const std::string& bin_name) const
{
    typedef odb::query<AccountBinView> query;
    odb::result<AccountBinView> r(db_->query<AccountBinView>(query::Account::name == account_name && query::AccountBin::name == bin_name));
    if (r.empty()) throw AccountBinNotFoundException(account_name, bin_name);

    unsigned long bin_id = r.begin().load()->bin_id;
    std::shared_ptr<AccountBin> bin(db_->load<AccountBin>(bin_id));
    return bin;
}

std::shared_ptr<AccountBin> Vault::getAccountBin(const std::string& account_name, const std::string& bin_name) const
{
    LOGGER(trace) << "Vault::getAccountBin(" << account_name << ", " << bin_name << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    std::shared_ptr<AccountBin> bin = getAccountBin_unwrapped(account_name, bin_name);
    return bin; 
}


// Tx operations
std::shared_ptr<Tx> Vault::insertTx_unwrapped(std::shared_ptr<Tx> tx)
{
    // TODO: Validate signatures

    tx->updateStatus();
    tx->updateHash();

    odb::result<Tx> tx_r(db_->query<Tx>(odb::query<Tx>::unsigned_hash == tx->unsigned_hash()));

    // First handle situations where we have a duplicate
    if (!tx_r.empty())
    {
        LOGGER(debug) << "Vault::insertTx_unwrapped - We have a transaction with the same unsigned hash: " << uchar_vector(tx->unsigned_hash()).getHex() << std::endl;
        std::shared_ptr<Tx> stored_tx(tx_r.begin().load());

        // First handle situations where the transaction we currently have is not fully signed.
        if (stored_tx->status() == Tx::UNSIGNED)
        {
            if (tx->status() != Tx::UNSIGNED)
            {
                // The transaction we received is a signed version of the one we had unsigned, so replace
                LOGGER(debug) << "Vault::insertTx_unwrapped - REPLACING OLD UNSIGNED TRANSACTION WITH NEW SIGNED TRANSACTION. hash: " << uchar_vector(tx->hash()).getHex() << std::endl; 
                std::size_t i = 0;
                txins_t txins = tx->txins();
                for (auto& txin: stored_tx->txins())
                {
                    txin->script(txins[i++]->script());
                    db_->update(txin);
                }
                stored_tx->status(tx->status());
                stored_tx->updateHash();
                db_->update(stored_tx);
                return stored_tx;
            }
            else
            {
                // The transaction we received is unsigned but might have more signatures. Only add new signatures.
                bool updated = false;
                std::size_t i = 0;
                txins_t txins = tx->txins();
                for (auto& txin: stored_tx->txins())
                {
                    using namespace CoinQ::Script;
                    Script stored_script(txin->script());
                    Script new_script(txins[i]->script());
                    unsigned int sigsadded = stored_script.mergesigs(new_script);
                    if (sigsadded > 0)
                    {
                        LOGGER(debug) << "Vault::insertTx_unwrapped - ADDED " << sigsadded << " NEW SIGNATURE(S) TO INPUT " << i << std::endl;
                        txin->script(stored_script.txinscript(Script::EDIT));
                        db_->update(txin);
                        updated = true;
                    }
                    i++;
                }
                return updated ? stored_tx : nullptr;
            }
        }
        else
        {
            // The transaction we currently have is already fully signed, so only update status if necessary
            if (tx->status() != Tx::UNSIGNED)
            {
                if (tx->status() > stored_tx->status())
                {
                    LOGGER(debug) << "Vault::insertTx_unwrapped - UPDATING TRANSACTION STATUS FROM " << stored_tx->status() << " TO " << tx->status() << ". hash: " << uchar_vector(stored_tx->hash()).getHex() << std::endl;
                    stored_tx->status(tx->status());
                    db_->update(stored_tx);
                    return stored_tx;
                }
                else
                {
                    LOGGER(debug) << "Vault::insertTx_unwrapped - Transaction not updated. hash: " << uchar_vector(stored_tx->hash()).getHex() << std::endl;
                    return nullptr;
                }
            }
            else
            {
                LOGGER(debug) << "Vault::insertTx_unwrapped - Stored transaction is already signed, received transaction is missing signatures. Ignore. hash: " << uchar_vector(stored_tx->hash()).getHex() << std::endl;
                return nullptr;
            }
        }
    }

    // If we get here it means we've either never seen this transaction before or it doesn't affect our accounts.

    std::set<std::shared_ptr<Tx>> conflicting_txs;
    std::set<std::shared_ptr<TxOut>> updated_txouts;

    // Check inputs
    bool sent_from_vault = false; // whether any of the inputs belong to vault
    bool have_all_outpoints = true; // whether we have all outpoints (for fee calculation)
    uint64_t input_total = 0;
    std::shared_ptr<Account> sending_account;

    for (auto& txin: tx->txins())
    {
        // Check if inputs connect
        tx_r = db_->query<Tx>(odb::query<Tx>::hash == txin->outhash());
        if (tx_r.empty())
        {
            have_all_outpoints = false;
        }
        else
        {
            std::shared_ptr<Tx> spent_tx(tx_r.begin().load());
            txouts_t outpoints = spent_tx->txouts();
            uint32_t outindex = txin->outindex();
            if (outpoints.size() <= outindex) throw std::runtime_error("Vault::insertTx_unwrapped - outpoint out of range.");
            std::shared_ptr<TxOut>& outpoint = outpoints[outindex];

            // Check for double spend, track conflicted transaction so we can update status if necessary later.
            std::shared_ptr<TxIn> conflict_txin = outpoint->spent();
            if (conflict_txin)
            {
                LOGGER(debug) << "Vault::insertTx_unwrapped - Discovered conflicting transaction. Double spend. hash: " << uchar_vector(conflict_txin->tx()->hash()).getHex() << std::endl;
                conflicting_txs.insert(conflict_txin->tx());
            } 

            input_total += outpoint->value();

            // Was this transaction signed using one of our accounts?
            odb::result<SigningScript> script_r(db_->query<SigningScript>(odb::query<SigningScript>::txoutscript == outpoint->script()));
            if (!script_r.empty())
            {
                sent_from_vault = true;
                outpoint->spent(txin);
                updated_txouts.insert(outpoint);
                if (!sending_account)
                {
                    // Assuming all inputs belong to the same account
                    // TODO: Allow coin mixing
                    std::shared_ptr<SigningScript> script(script_r.begin().load());
                    sending_account = script->account();
                }
            }
        }
    }

    std::set<std::shared_ptr<SigningScript>> scripts;

    // Check outputs
    bool sent_to_vault = false; // whether any of the outputs are spendable by accounts in vault
    uint64_t output_total = 0;

    for (auto& txout: tx->txouts())
    {
        output_total += txout->value();
        odb::result<SigningScript> script_r(db_->query<SigningScript>(odb::query<SigningScript>::txoutscript == txout->script()));
        if (!script_r.empty())
        {
            // This output is spendable from an account in the vault
            sent_to_vault = true;
            std::shared_ptr<SigningScript> script(script_r.begin().load());
            txout->signingscript(script);

            // Update the signing script and txout status
            switch (script->status())
            {
            case SigningScript::UNUSED:
                if (sent_from_vault && script->account_bin()->isChange())
                {
                    script->status(SigningScript::CHANGE);
                }
                else
                {
                    script->status(SigningScript::RECEIVED);
                }
                scripts.insert(script);
                try
                {
                    refillAccountBinPool_unwrapped(script->account_bin());
                }
                catch (const AccountChainCodeLockedException& e)
                {
                    LOGGER(debug) << "Vault::insertTx_unwrapped - Chain code is locked so change pool cannot be replenished." << std::endl;
                }
                break;

            case SigningScript::PENDING:
                script->status(SigningScript::RECEIVED);
                scripts.insert(script);
                break;

            default:
                break;
            }

            // Check if the output has already been spent (transactions inserted out of order)
            odb::result<TxIn> txin_r(db_->query<TxIn>(odb::query<TxIn>::outhash == tx->hash() && odb::query<TxIn>::outindex == txout->txindex()));
            if (!txin_r.empty())
            {
                std::shared_ptr<TxIn> txin(txin_r.begin().load());
                txout->spent(txin);
            }
        }
        else if (sending_account)
        {
            // Again, assume all inputs sent from same account.
            // TODO: Allow coin mixing.
            txout->sending_account(sending_account);
        }
    }

    if (!conflicting_txs.empty())
    {
        tx->status(Tx::CONFLICTING);
        for (auto& conflicting_tx: conflicting_txs)
        {
            if (conflicting_tx->status() != Tx::CONFIRMED)
            {
                conflicting_tx->status(Tx::CONFLICTING);
                db_->update(conflicting_tx);
            }
        }
    }

    if (sent_from_vault || sent_to_vault)
    {
        LOGGER(debug) << "Vault::insertTx_unwrapped - INSERTING NEW TRANSACTION. hash: " << uchar_vector(tx->hash()).getHex() << ", unsigned hash: " << uchar_vector(tx->unsigned_hash()).getHex() << std::endl;
        if (have_all_outpoints) { tx->fee(input_total - output_total); }
        db_->persist(*tx);

        for (auto& txin:    tx->txins())    { db_->persist(txin); }
        for (auto& txout:   tx->txouts())   { db_->persist(txout); }
        for (auto& script:  scripts)        { db_->update(script); }
        for (auto& txout:   updated_txouts) { db_->update(txout); }

        // TODO: updateConfirmations_unwrapped(tx);
        return tx;
    }

    LOGGER(debug) << "Vault::insertTx_unwrapped - transaction not inserted." << std::endl;
    return nullptr; 
}

std::shared_ptr<Tx> Vault::insertTx(std::shared_ptr<Tx> tx)
{
    LOGGER(trace) << "Vault::insertTx(...) - hash: " << uchar_vector(tx->hash()).getHex() << ", unsigned hash: " << uchar_vector(tx->unsigned_hash()).getHex() << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    tx = insertTx_unwrapped(tx);
    if (tx) t.commit();
    return tx;
}

std::shared_ptr<Tx> Vault::createTx_unwrapped(const std::string& account_name, uint32_t tx_version, uint32_t tx_locktime, txouts_t txouts, uint64_t fee, unsigned int /*maxchangeouts*/)
{
    // TODO: Better rng seeding
    std::srand(std::time(0));

    // TODO: Better fee calculation heuristics
    uint64_t desired_total = fee;
    for (auto& txout: txouts) { desired_total += txout->value(); }

    std::shared_ptr<Account> account = getAccount_unwrapped(account_name);

    // TODO: Better coin selection
    typedef odb::query<TxOutView> query_t;
    odb::result<TxOutView> utxoview_r(db_->query<TxOutView>(query_t::TxOut::status == TxOut::UNSPENT && query_t::receiving_account::id == account->id()));
    std::vector<TxOutView> utxoviews;
    for (auto& utxoview: utxoview_r) { utxoviews.push_back(utxoview); }
   
    std::random_shuffle(utxoviews.begin(), utxoviews.end(), [](int i) { return std::rand() % i; });

    txins_t txins;
    int i = 0;
    uint64_t total = 0;
    for (auto& utxoview: utxoviews)
    {
        total += utxoview.value;
        std::shared_ptr<TxIn> txin(new TxIn(utxoview.tx_hash, utxoview.tx_index, utxoview.signingscript_txinscript, 0xffffffff));
        txins.push_back(txin);
        i++;
        if (total >= desired_total) break;
    }
    if (total < desired_total) throw AccountInsufficientFundsException(account_name); 

    utxoviews.resize(i);
    uint64_t change = total - desired_total;

    if (change > 0)
    {
        std::shared_ptr<AccountBin> bin = getAccountBin_unwrapped(account_name, CHANGE_BIN_NAME);
        std::shared_ptr<SigningScript> changescript = newAccountBinSigningScript_unwrapped(bin);

        // TODO: Allow adding multiple change outputs
        std::shared_ptr<TxOut> txout(new TxOut(change, changescript));
        txouts.push_back(txout);
    }
    std::random_shuffle(txouts.begin(), txouts.end(), [](int i) { return std::rand() % i; });

    std::shared_ptr<Tx> tx(new Tx());
    tx->set(tx_version, txins, txouts, tx_locktime, time(NULL), Tx::UNSIGNED);
    return tx;
}

std::shared_ptr<Tx> Vault::createTx(const std::string& account_name, uint32_t tx_version, uint32_t tx_locktime, txouts_t txouts, uint64_t fee, unsigned int maxchangeouts, bool insert)
{
    LOGGER(trace) << "Vault::createTx(" << account_name << ", " << tx_version << ", " << tx_locktime << ", " << txouts.size() << " txout(s), " << fee << ", " << maxchangeouts << ", " << (insert ? "insert" : "no insert") << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    std::shared_ptr<Tx> tx = createTx_unwrapped(account_name, tx_version, tx_locktime, txouts, fee, maxchangeouts);
    if (insert)
    {
        tx = insertTx_unwrapped(tx);
        if (tx) t.commit();
    } 
    return tx;
}

// Block operations
uint32_t Vault::getBestHeight() const
{
    LOGGER(trace) << "Vault::getBestHeight()" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::transaction t(db_->begin());
    odb::result<BestHeightView> r(db_->query<BestHeightView>());
    uint32_t best_height = r.empty() ? 0 : r.begin().load()->best_height;
    return best_height;
}
