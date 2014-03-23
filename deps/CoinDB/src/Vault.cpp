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
    LOGGER(trace) << "Created Vault instance" << std::endl;
//    if (create) setVersion(version);
}

#if defined(DATABASE_SQLITE)
Vault::Vault(const std::string& filename, bool create, uint32_t version)
    : db_(openDatabase(filename, create))
{
    LOGGER(trace) << "Created Vault instance - filename: " << filename << " create: " << (create ? "true" : "false") << " version: " << version << std::endl;
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

std::shared_ptr<Keychain> Vault::newKeychain(const std::string& name, const secure_bytes_t& entropy, const secure_bytes_t& lockKey, const bytes_t& salt)
{
    LOGGER(trace) << "Vault::newKeychain(" << name << ", ...)" << std::endl;

    std::shared_ptr<Keychain> keychain(new Keychain(name, entropy, lockKey, salt));

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session session;
    odb::core::transaction t(db_->begin());
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
    if (keychain_r.empty()) throw std::runtime_error("Keychain not found.");

    if (old_name == new_name) return;

    odb::result<Keychain> new_keychain_r(db_->query<Keychain>(odb::query<Keychain>::name == new_name));
    if (!new_keychain_r.empty()) throw std::runtime_error("A keychain with that name already exists.");

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

std::shared_ptr<Keychain> Vault::getKeychain_unwrapped(const std::string& keychain_name) const
{
    odb::result<Keychain> r(db_->query<Keychain>(odb::query<Keychain>::name == keychain_name));
    if (r.empty()) {
        throw std::runtime_error("Vault::getKeychain - keychain not found.");
    }
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
    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    odb::result<Account> r(db_->query<Account>(odb::query<Account>::name == account_name));
    if (!r.empty())
        throw std::runtime_error("Vault::newAccount - account with that name already exists.");

    KeychainSet keychains;
    for (auto& keychain_name: keychain_names)
    {
        odb::result<Keychain> r(db_->query<Keychain>(odb::query<Keychain>::name == keychain_name));
        if (r.empty()) {
            std::stringstream ss;
            ss << "Vault::newAccount - keychain " << keychain_name << " not found.";
            throw std::runtime_error(ss.str());
        }
        keychains.insert(r.begin().load());
    }

    for (auto& keychain: keychains)
    {
        // TODO: pass lock key
        keychain->unlockChainCode(secure_bytes_t());
    }

    std::shared_ptr<Account> account(new Account(account_name, minsigs, keychains, unused_pool_size, time_created));
    db_->persist(account);

    std::shared_ptr<AccountBin> changeAccountBin= account->addBin("@change");
    db_->persist(changeAccountBin);

    std::shared_ptr<AccountBin> defaultAccountBin = account->addBin("@default");
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

std::shared_ptr<AccountBin> Vault::addAccountBin(const std::string& account_name, const std::string& bin_name)
{
    LOGGER(trace) << "Vault::addAccountBin(" << account_name << ", " << bin_name << ")" << std::endl;

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

    // TODO: pass unlock key
    for (auto& keychain: account->keychains())
    {
        keychain->unlockChainCode(secure_bytes_t());
    }

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

std::shared_ptr<TxOut> Vault::newTxOut_unwrapped(const std::string& account_name, const std::string& label, uint64_t value, const std::string& bin_name)
{
    std::shared_ptr<Account> account = getAccount_unwrapped(account_name);
    std::shared_ptr<AccountBin> bin = getAccountBin_unwrapped(account_name, bin_name);

    // TODO: pass unlock key
    for (auto& keychain: account->keychains()) { keychain->unlockChainCode(secure_bytes_t()); }

    // Replenish unused script pool for bin
    typedef odb::query<ScriptCountView> count_query;
    odb::result<ScriptCountView> count_result(db_->query<ScriptCountView>(count_query::AccountBin::id == bin->id() && count_query::SigningScript::status == SigningScript::UNUSED));
    uint32_t count = 0;
    if (count_result.empty()) count = count_result.begin().load()->count;

    for (uint32_t i = count; i <= account->unused_pool_size(); i++)
    {
        std::shared_ptr<SigningScript> script = bin->newSigningScript();
        for (auto& key: script->keys()) { db_->persist(key); }
        db_->persist(script); 
    } 
    db_->update(bin);
    db_->update(account);

    // Get the next available unused signing script
    typedef odb::query<SigningScriptView> view_query;
    odb::result<SigningScriptView> view_result(db_->query<SigningScriptView>((view_query::AccountBin::id == bin->id() && view_query::SigningScript::status == SigningScript::UNUSED) +
        "ORDER BY" + view_query::SigningScript::id + "LIMIT 1"));
    if (view_result.empty()) throw std::runtime_error("No scripts available."); // This should never happen

    std::shared_ptr<SigningScriptView> view(view_result.begin().load());

    typedef odb::query<SigningScript> script_query;
    odb::result<SigningScript> script_result(db_->query<SigningScript>(script_query::id == view->id));
    std::shared_ptr<SigningScript> script(script_result.begin().load());
    script->label(label);
    script->status(SigningScript::REQUESTED);
    db_->update(script);

    std::shared_ptr<TxOut> txout(new TxOut(value, script->txoutscript()));
    return txout; 
}

std::shared_ptr<TxOut> Vault::newTxOut(const std::string& account_name, const std::string& label, uint64_t value, const std::string& bin_name)
{
    LOGGER(trace) << "Vault::getAccountBin(" << account_name << ", " << bin_name << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    std::shared_ptr<TxOut> txout = newTxOut_unwrapped(account_name, label, value, bin_name);
    t.commit();
    return txout;
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
