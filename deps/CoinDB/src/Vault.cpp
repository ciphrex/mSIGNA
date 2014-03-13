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

/*
void Vault::setVersion(uint32_t version)
{
    LOGGER(trace) << "Vault::setVersion(" << version << ")" << std::endl;
    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::transaction t(db_->begin());

    odb::result<Version> r(db_->query<Version>((odb::query<Version>::id <= 1) + "LIMIT 1"));
    std::shared_ptr<Version> version_object;
    if (r.empty()) {
        version_object = std::shared_ptr<Version>(new Version(version));
        db_->persist(version_object);
    }
    else {
        version_object = std::shared_ptr<Version>(r.begin().load());
        version_object->version(version);
        db_->update(version_object);
    }

    t.commit();
}

uint32_t Vault::getVersion() const
{
    LOGGER(trace) << "Vault::getVersion()" << std::endl;
    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::transaction t(db_->begin());

    odb::result<Version> r(db_->query<Version>((odb::query<Version>::id <= 1) + "LIMIT 1"));
    if (r.empty()) return 0;

    std::shared_ptr<Version> version_object(r.begin().load());
    return version_object->version();
}
*/

bool Vault::keychainExists(const std::string& keychain_name) const
{
    LOGGER(trace) << "Vault::keychainExists(" << keychain_name << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::transaction t(db_->begin());
    odb::result<Keychain> r(db_->query<Keychain>(odb::query<Keychain>::name == keychain_name));
 
    return !r.empty();
}

void Vault::newKeychain(const std::string& name, const secure_bytes_t& entropy, const secure_bytes_t& lockKey, const bytes_t& salt)
{
    Keychain keychain(name, entropy, lockKey, salt);

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session session;
    odb::core::transaction t(db_->begin());

    persistKeychain_unwrapped(keychain);

    t.commit();
}

void Vault::newKeychain(const std::string& name, std::shared_ptr<Keychain> parent)
{
    Keychain keychain(name, parent);

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session session;
    odb::core::transaction t(db_->begin());

    persistKeychain_unwrapped(keychain);

    t.commit();
}

void Vault::eraseKeychain(const std::string& name) const
{
    throw std::runtime_error("Vault::eraseKeychain - method not yet implemented.");
    // TODO: erase individual keys - for now, this method is commented out
/*
    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session session;
    odb::core::transaction t(db_->begin());

    odb::result<Keychain> r(db_->query<Keychain>(odb::query<Keychain>::name == name));

    if (r.empty()) {
        throw std::runtime_error("Keychain not found.");
    }

    std::shared_ptr<Keychain> keychain(r.begin().load());

    odb::result<Key> signing_script_r(db_->query<SigningScript>(odb::query<SigningScript>::account->id == account->id()));
    for (auto& signing_script: signing_script_r) { db_->erase(signing_script); }

//    db_->erase_query<SigningScript>(odb::query<SigningScript>::account == account->id());
    db_->erase(account);

    t.commit();
*/
}

void Vault::renameKeychain(const std::string& old_name, const std::string& new_name)
{
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

void Vault::persistKeychain_unwrapped(Keychain& keychain)
{
    if (keychain.parent())
        db_->update(keychain.parent());

    db_->persist(keychain);
}

std::shared_ptr<Keychain> Vault::getKeychain(const std::string& keychain_name) const
{
    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::transaction t(db_->begin());
    odb::result<Keychain> r(db_->query<Keychain>(odb::query<Keychain>::name == keychain_name));
    if (r.empty()) {
        throw std::runtime_error("Vault::getKeychain - keychain not found.");
    }

    std::shared_ptr<Keychain> keychain(r.begin().load());
    return keychain;
}

inline void write_uint32(std::ofstream& f, uint32_t data)
{
    f.put((data & 0xff000000) >> 24);
    f.put((data & 0xff0000) >> 16);
    f.put((data & 0xff00) >> 8);
    f.put(data & 0xff);
}

bytes_t Vault::exportKeychain(std::shared_ptr<Keychain> keychain, const std::string& filepath, bool exportprivkeys) const
{
/*
    boost::lock_guard<boost::mutex> lock(mutex);


    if (exportprivkeys && !keychain->isPrivate()) {
        throw std::runtime_error("Vault::exportKeychain - keychain does not contain private keys.");
    }

    uint32_t size;
    std::ofstream f(filepath, std::ofstream::trunc | std::ofstream::binary);

    write_uint32(f, keychain->depth());
    write_uint32(f, keychain->parent_fp());
    write_uint32(f, keychain->child_num());

    size = keychain->pubkey().size();
    write_uint32(f, size);
    f.write((char*)&keychain->pubkey()[0], size);

    // If chain code is unlocked, export plaintext. Otherwise, export encrypted chain code
    bool chain_code_locked = keychain->isChainCodeLocked();
    f.put(chain_code_locked);
    if (chain_code_locked) {
        size = keychain->chain_code_ciphertext().size();
        write_uint32(f. size);
        f.write((char*)&keychain->chain_code_ciphertext()[0], size);
    }
    else {
        secure_bytes_t chain_code = keychain->chain_code();
        size = chain_code.size();
        write_uint32(f, size);
        f.write((char*)&chain_code[0], size);
    }


    // write extended key
    // TODO: save encrypted bytes
    bytes_t extkey = keychain->extendedkey()->bytes(exportprivkeys);
    bytes_t extpubkey;
    if (keychain->is_private()) {
        extpubkey = Coin::HDKeychain(extkey).getPublic().extkey();
        if (!exportprivkeys) {
            extkey = extpubkey;
        }
    }
    else {
        extpubkey = extkey;
    }

    size = extkey.size();
    write_uint32(f, size);
    f.write((char*)&extkey[0], size);

    // write number of saved keys using 4 bytes msb first
    size = keychain->numsavedkeys();
    write_uint32(f, size);

    // write hash
    bytes_t hash = sha256_2(extpubkey);
    size = hash.size();
    write_uint32(f, size);
    f.write((char*)&hash[0], size);

    return hash;
*/
    return bytes_t();
}

inline uint32_t sizebuf_to_uint(char sizebuf[])
{
    uint32_t data = ((uint32_t)sizebuf[0] & 0xff) << 24;
    data |= ((uint32_t)sizebuf[1] & 0xff) << 16;
    data |= ((uint32_t)sizebuf[2] & 0xff) << 8;
    data |= ((uint32_t)sizebuf[3] & 0xff);
    return data;
}

bytes_t Vault::importKeychain(const std::string& keychain_name, const std::string& filepath, bool& importprivkeys)
{
/*
    if (keychainExists(keychain_name)) {
        throw std::runtime_error("Vault::importKeychain - keychain with same name already exists.");
    }

    std::ifstream f(filepath, std::ifstream::binary);
    if (!f) {
        throw std::runtime_error("Vault::importKeychain - file could not be opened.");
    }

    const unsigned int MAXBUFSIZE = 1024;

    uint32_t size;
    char sizebuf[4];
    char buf[MAXBUFSIZE];

    // check whether it is deterministic
    char deterministic;
    f.get(deterministic);
    if (!f) {
        throw std::runtime_error("Vault::importKeychain - error reading deterministic flag.");
    }

    if (deterministic != 0) {
        // get extkey
        f.read(sizebuf, 4);
        if (!f) {
            throw std::runtime_error("Vault::importKeychain - error reading extkey.");
        }
        size = sizebuf_to_uint(sizebuf);
        if (size > MAXBUFSIZE) {
            throw std::runtime_error("Vault::importKeychain - error reading extkey.");
        }

        f.read(buf, size);
        if (!f) {
            throw std::runtime_error("Vault::importKeychain - error reading extkey.");
        }

        bytes_t extkey((unsigned char*)buf, (unsigned char*)buf + size);
        bytes_t extpubkey;
        Coin::HDKeychain hdkeychain(extkey);
        if (hdkeychain.isPrivate()) {
            extpubkey = hdkeychain.getPublic().extkey();
            if (!importprivkeys) {
                extkey = extpubkey;
            }
        }
        else { 
            importprivkeys = false;
            extpubkey = extkey;
        }

        // read number of saved keys (lookahead distance)
        f.read(sizebuf, 4);
        if (!f) {
            throw std::runtime_error("Vault::importKeychain - error reading number of saved keys.");
        }

        uint32_t numkeys = sizebuf_to_uint(sizebuf);
        // TODO: Add checksum for numkeys

        // read hash
        f.read(sizebuf, 4);
        if (!f) {
            throw std::runtime_error("Vault::importKeychain - error reading hash.");
        }
        size = sizebuf_to_uint(sizebuf);
        if (size > MAXBUFSIZE) {
            throw std::runtime_error("Vault::importKeychain - error reading hash.");
        }

        f.read(buf, size);
        if (!f) {
            throw std::runtime_error("Vault::importKeychain - error reading hash.");
        }

        bytes_t hash((unsigned char*)buf, (unsigned char*)buf + size);
        if (hash != sha256_2(extpubkey)) {
            throw std::runtime_error("Vault::importKeychain - hash mismatch, file is corrupted.");
        }
        
        newHDKeychain(keychain_name, extkey, numkeys);
        return sha256_2(extkey);
    }
    else {
        // get privkeys flag
        char hasprivkeys;
        f.get(hasprivkeys);
        if (!f) {
            throw std::runtime_error("Vault::importKeychain - error reading exportprivkey flag.");
        }
        if (hasprivkeys == 0) {
            importprivkeys = false;
        }

        // read number of keys using 4 bytes msb first
        f.read(sizebuf, 4);
        if (!f) {
            throw std::runtime_error("Vault::importKeychain - error reading numkeys.");
        }
        uint32_t numkeys = sizebuf_to_uint(sizebuf);

        Keychain::type_t keychain_type =
            importprivkeys ? Keychain::PRIVATE : Keychain::PUBLIC;

        Keychain keychain(keychain_name, keychain_type);

        boost::lock_guard<boost::mutex> lock(mutex);

        odb::core::transaction t(db_->begin());

        // read keys, write to db
        std::stringstream err;
        uchar_vector hash;
        CoinKey coinkey;
        uint32_t i;
        for (i = 0; i < numkeys; i++) {
            f.read(sizebuf, 4);
            if (!f) break;
            size = sizebuf_to_uint(sizebuf);
            if (size > MAXBUFSIZE) break;

            f.read(buf, size);
            if (!f) break;
            bytes_t keydata((unsigned char*)buf, (unsigned char*)buf + size);

            if (hasprivkeys != 0) {
                if (!coinkey.setPrivateKey(keydata)) {
                    err << "Vault::importKeychain - invalid private key at position " << i << ".";
                    throw std::runtime_error(err.str());
                }
                bytes_t pubkey = coinkey.getPublicKey();
                hash = sha256(hash + pubkey);
                Key* pKey;
                if (importprivkeys) {
                    pKey = new Key(pubkey, keydata);
                }
                else {
                    pKey = new Key(pubkey);
                }
                std::shared_ptr<Key> key(pKey);
                keychain.add(key);
                db_->persist(*key);                         
            }
            else {
                if (!coinkey.setPublicKey(keydata)) {
                    err << "Vault::importKeychain - invalid public key at position " << i << ".";
                    throw std::runtime_error(err.str());
                }
                hash = sha256(hash + keydata);
                std::shared_ptr<Key> key(new Key(keydata));
                keychain.add(key);
                db_->persist(*key);
            }
        }
        if (i < numkeys) {
            err << "Vault::importKeychain - error reading key at position " << i << ".";
            throw std::runtime_error(err.str());
        }

        // read hash, compare
        f.read(sizebuf, 4);
        size = sizebuf_to_uint(sizebuf);
        if (!f || size > MAXBUFSIZE) {
            throw std::runtime_error("Vault::importKeychain - error reading hash.");
        }

        f.read(buf, size);
        if (!f) {
            throw std::runtime_error("Vault::importKeychain - error reading hash.");
        }
        
        bytes_t hashread((unsigned char*)buf, (unsigned char*)buf + size);
        if (hashread != hash) {
            throw std::runtime_error("Vault::importKeychain - hash mismatch, file is corrupted.");
        }

        // make sure we don't already have a keychain with this hash
        odb::result<Keychain> r(db_->query<Keychain>(odb::query<Keychain>::hash == hash));
        if (!r.empty()) {
            throw std::runtime_error("Vault::importKeychain - vault already contains a keychain with the same hash.");
        }

        db_->persist(keychain);

        t.commit();

        return hash;
    }
*/
    return bytes_t();
}

bool Vault::isKeychainFilePrivate(const std::string& filepath) const
{
    std::ifstream f(filepath, std::ifstream::binary);
    if (!f) {
        throw std::runtime_error("Vault::isKeychainFilePrivate - file could not be opened.");
    }

    // get exportprivkeys flag
    char exportprivkeys;
    f.get(exportprivkeys);
    if (!f) {
        throw std::runtime_error("Vault::isKeychainFilePrivate - error reading exportprivkey flag.");
    }

    return exportprivkeys;
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
    odb::core::transaction t(db_->begin());
    odb::result<Account> r(db_->query<Account>(odb::query<Account>::name == account_name));
    if (!r.empty())
        throw std::runtime_error("Vault::newAccount - account with that name already exists.");

    Account::keychains_t keychains;
    for (auto& keychain_name: keychain_names)
    {
        odb::result<Keychain> r(db_->query<Keychain>(odb::query<Keychain>::name == keychain_name));
        if (r.empty()) {
            throw std::runtime_error("Vault::newAccount - keychain not found.");
        }
        keychains.insert(r.begin().load());
    }

    std::shared_ptr<Account> account(new Account(account_name, minsigs, keychains, unused_pool_size, time_created));

    typedef odb::query<ScriptCountView> query;
    odb::result<ScriptCountView> scriptcount_r(db_->query<ScriptCountView>(query::Account::name == account_name && query::SigningScript::status == SigningScript::UNUSED));
    uint32_t count = 0;
    if (!scriptcount_r.empty())
        count = scriptcount_r.begin().load()->count;

    db_->persist(account);

    for (uint32_t i = count; i < account->unused_pool_size(); i++)
    {
        std::shared_ptr<SigningScript> signingscript = account->newSigningScript();
        db_->persist(signingscript);
    }
    db_->update(account);

    t.commit();
}

void Vault::eraseAccount(const std::string& account_name) const
{
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

std::shared_ptr<Account> Vault::getAccount(const std::string& account_name) const
{
    LOGGER(trace) << "Vault::getAccount(" << account_name << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::transaction t(db_->begin());
    odb::result<Account> r(db_->query<Account>(odb::query<Account>::name == account_name));
    if (r.empty()) {
        throw std::runtime_error("Vault::getAccount - account not found.");
    }

    std::shared_ptr<Account> account(r.begin().load());
    return account;    
}

uint32_t Vault::getCurrentAccountPoolSize(const std::string& account_name) const
{
    LOGGER(trace) << "Vault::getCurrentAccountPoolSize(" << account_name << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::transaction t(db_->begin());
    odb::result<Account> account_r(db_->query<Account>(odb::query<Account>::name == account_name));
    if (account_r.empty()) {
        throw std::runtime_error("Vault::getAccount - account not found.");
    }

    typedef odb::query<ScriptCountView> query;
    odb::result<ScriptCountView> scriptcount_r(db_->query<ScriptCountView>(query::Account::name == account_name && query::SigningScript::status == SigningScript::UNUSED));
    uint32_t count = 0;
    if (!scriptcount_r.empty())
        count = scriptcount_r.begin().load()->count;

    return count;
}

uint32_t Vault::refillAccountPool(const std::string& account_name)
{
    LOGGER(trace) << "Vault::getCurrentAccountPoolSize(" << account_name << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::transaction t(db_->begin());
    odb::result<Account> account_r(db_->query<Account>(odb::query<Account>::name == account_name));
    if (account_r.empty()) {
        throw std::runtime_error("Vault::getAccount - account not found.");
    }

    std::shared_ptr<Account> account(account_r.begin().load());

    typedef odb::query<ScriptCountView> query;
    odb::result<ScriptCountView> scriptcount_r(db_->query<ScriptCountView>(query::Account::name == account_name && query::SigningScript::status == SigningScript::UNUSED));
    uint32_t count = 0;
    if (!scriptcount_r.empty())
        count = scriptcount_r.begin().load()->count;

    for (uint32_t i = count; i < account->unused_pool_size(); i++)
    {
        std::shared_ptr<SigningScript> signingscript = account->newSigningScript();
        db_->persist(signingscript);
    }
    db_->update(account);

    t.commit();
    return account->unused_pool_size() - count;
}

/*

void Vault::newAccount(const std::string& name, unsigned int minsigs, const std::vector<std::string>& keychain_names, bool is_ours)
{
    if (keychain_names.empty()) {
        throw std::runtime_error("Vault::newAccount() - keychain_names cannot be empty.");
    }

    if (keychain_names.size() > 16) {
        throw std::runtime_error("Vault::newAccount() - keychain_names cannot contain more than 16 names.");
    }

    std::vector<std::string> keychain_names__(keychain_names);
    std::sort(keychain_names__.begin(), keychain_names__.end());
    auto it = std::unique_copy(keychain_names.begin(), keychain_names.end(), keychain_names__.begin());
    if ((size_t)std::distance(keychain_names__.begin(), it) != (size_t)(keychain_names.size())) {
        throw std::runtime_error("Vault::newAccount() - keychain_names cannot contain duplicates.");
    }

    if (minsigs < 1 || minsigs > keychain_names__.size()) {
        throw std::runtime_error("Vault::newAccount() - invalid minsigs.");
    }

    typedef odb::query<Keychain> query;
    typedef odb::result<Keychain> result;

    {
        boost::lock_guard<boost::mutex> lock(mutex);

        odb::core::session session;

        odb::core::transaction t(db_->begin());

        Account::keychains_t keychains;
        for (auto& keychain_name: keychain_names__) {
            result r(db_->query<Keychain>(query::name == keychain_name));
            if (r.empty()) {
                std::stringstream err;
                err << "Keychain " << keychain_name << " not found.";
                throw std::runtime_error(err.str());
            }

            std::shared_ptr<Keychain> keychain(r.begin().load());
            keychains.insert(keychain);
        }

        std::shared_ptr<Account> account(new Account(is_ours));
        account->set(name, minsigs, keychains);
        for (auto& script: account->scripts()) { db_->persist(script); }
        db_->persist(account);

        t.commit();
    }
}

std::vector<AccountInfo> Vault::getAccounts(account_ownership_t ownership) const
{
    std::vector<AccountInfo> accounts;

    boost::lock_guard<boost::mutex> lock(mutex);

    odb::core::transaction t(db_->begin());
//    t.tracer(odb::stderr_tracer);

    odb::result<AccountView> accountview_result;
    if (ownership == ACCOUNT_OWNER_US) {
        accountview_result = db_->query<AccountView>(odb::query<AccountView>::is_ours);
    }
    else if (ownership == ACCOUNT_OWNER_NOT_US) {
        accountview_result = db_->query<AccountView>(!odb::query<AccountView>::is_ours);
    }
    else {
        accountview_result = db_->query<AccountView>();
    }
    for (auto& accountview: accountview_result) {
        std::vector<std::string> keychain_names;
        odb::result<KeychainView> keychainview_result(db_->query<KeychainView>(odb::query<KeychainView>::Account::name == accountview.name));
        for (auto& keychainview: keychainview_result) {
            keychain_names.push_back(keychainview.keychain_name);
        }

        typedef odb::query<BalanceView> balance_query;
        odb::result<BalanceView> balance_result(db_->query<BalanceView>(balance_query::Account::id == accountview.id && balance_query::TxOut::spent.is_null()));
        uint64_t balance = 0;
        for (auto& view: balance_result) { balance += view.balance; }

        typedef odb::query<ScriptCountView> scriptcount_query;
        odb::result<ScriptCountView> scriptcount_result(db_->query<ScriptCountView>(scriptcount_query::Account::id == accountview.id && scriptcount_query::SigningScript::status == SigningScript::UNUSED));
        unsigned long scriptcount = 0;
        if (!scriptcount_result.empty()) {
            scriptcount = scriptcount_result.begin().load()->count;
        }

        accounts.push_back(AccountInfo(accountview.id, accountview.name, accountview.minsigs, keychain_names, balance, 0, scriptcount));
    }
    t.commit();

    return accounts;
}

std::shared_ptr<Account> Vault::getAccount(const std::string& name) const
{
    boost::lock_guard<boost::mutex> lock(mutex);

    odb::core::session session;

    if (!accountExists(name)) {
        throw std::runtime_error("Account not found.");
    }

    odb::core::transaction t(db_->begin());

    odb::result<Account> r(db_->query<Account>(odb::query<Account>::name == name));
    if (r.empty()) {
        std::stringstream err;
        err << "Account " << name << " not found.";
        throw std::runtime_error(err.str());
    }

    return std::shared_ptr<Account>(r.begin().load());
}

bool Vault::accountExists(const std::string& account_name) const
{
    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::transaction t(db_->begin());
    odb::result<AccountView> r(db_->query<AccountView>(odb::query<AccountView>::name == account_name));
    return !r.empty();
}

void Vault::eraseAccount(const std::string& name) const
{
    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session session;
    odb::core::transaction t(db_->begin());

    odb::result<Account> r(db_->query<Account>(odb::query<Account>::name == name));

    if (r.empty()) {
        throw std::runtime_error("Account not found.");
    }

    std::shared_ptr<Account> account(r.begin().load());

    odb::result<SigningScript> signing_script_r(db_->query<SigningScript>(odb::query<SigningScript>::account->id == account->id()));
    for (auto& signing_script: signing_script_r) { db_->erase(signing_script); }

//    db_->erase_query<SigningScript>(odb::query<SigningScript>::account == account->id());
    db_->erase(account);

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

bytes_t Vault::exportAccount(const std::string& account_name, const std::string& filepath) const
{
    boost::lock_guard<boost::mutex> lock(mutex);

    odb::core::session session;

    odb::core::transaction t(db_->begin());

    odb::result<Account> r(db_->query<Account>(odb::query<Account>::name == account_name));
    if (r.empty()) {
        throw std::runtime_error("Vault::exportAccount - account not found.");
    }

    std::shared_ptr<Account> account(r.begin().load());

    std::ofstream f(filepath, std::ofstream::trunc | std::ofstream::binary);

    // Write keychain hashes
    uint32_t size = account->keychain_hashes().size();
    write_uint32(f, size);
    for (auto& hash: account->keychain_hashes()) {
        size = hash.size();
        write_uint32(f, size);
        f.write((char*)&hash[0], size);
    }

    // Write minsigs
    write_uint32(f, account->minsigs());

    // Write time created
    write_uint32(f, account->time_created());

    // write number of signing scripts using 4 bytes msb first
    size = account->scripts().size();
    write_uint32(f, size);

    // write signing scripts and compute hash
    uchar_vector hash;
    bytes_t buffer;
    for (auto& script: account->scripts()) {
        hash = sha256(hash + script->txinscript() + script->txoutscript());

        // Write label
        size = script->label().size();
        write_uint32(f, size);
        f.write(script->label().c_str(), size);

        // Write status
        f.put(script->status());

        // Write txinscript
        buffer = script->txinscript();
        size = buffer.size();
        write_uint32(f, size);
        f.write((char*)&buffer[0], size);

        // Write txoutscript
        buffer = script->txoutscript();
        size = buffer.size();
        write_uint32(f, size);
        f.write((char*)&buffer[0], size);
    }

    // Write hash
    size = hash.size();
    write_uint32(f, size);
    f.write((char*)&hash[0], size);

    return hash;
}

bytes_t Vault::importAccount(const std::string& account_name, const std::string& filepath, bool is_ours)
{
    if (accountExists(account_name)) {
        throw std::runtime_error("Vault::importAccount - account with same name already exists.");
    }

    std::ifstream f(filepath, std::ifstream::binary);
    if (!f) {
        throw std::runtime_error("Vault::importAccount - file could not be opened.");
    }

    const unsigned int MAXBUFSIZE = 256;
    char sizebuf[4];
    uint32_t size;
    char buf[MAXBUFSIZE];

    // Get keychain hashes
    f.read(sizebuf, 4);
    if (!f) {
        throw std::runtime_error("Vault::importAccount - error reading keychain hash count.");
    }
    uint32_t num_keychains = sizebuf_to_uint(sizebuf);

    std::set<bytes_t> keychain_hashes;
    uint32_t i;
    for (i = 0; i < num_keychains; i++) {
        f.read(sizebuf, 4);
        if (!f) break;

        size = sizebuf_to_uint(sizebuf);
        if (size > MAXBUFSIZE) break;

        f.read(buf, size);
        if (!f) break;

        bytes_t keychain_hash((unsigned char*)buf, (unsigned char*)buf + size);
        keychain_hashes.insert(keychain_hash);
    }
    if (i < num_keychains) {
        std::stringstream err;
        err << "Vault::importAccount - error reading keychain hash at position " << i << ".";
        throw std::runtime_error(err.str());
    }

    // Read minsigs
    f.read(sizebuf, 4);
    if (!f) {
        throw std::runtime_error("Vault::importAccount - error reading minsigs.");
    }
    uint32_t minsigs = sizebuf_to_uint(sizebuf);

    // Read time created
    f.read(sizebuf, 4);
    if (!f) {
        throw std::runtime_error("Vault::importAccount - error reading time created.");
    }
    uint32_t time_created = sizebuf_to_uint(sizebuf);

    // Read signing scripts and compute hash
    f.read(sizebuf, 4);
    if (!f) {
        throw std::runtime_error("Vault::importAccount - error reading signing script count.");
    }
    uint32_t num_scripts = sizebuf_to_uint(sizebuf);

    uchar_vector hash;
    Account::signingscripts_t scripts;
    for (i = 0; i < num_scripts; i++) {
        // Read label
        f.read(sizebuf, 4);
        if (!f) break;

        size = sizebuf_to_uint(sizebuf);
        if (size > MAXBUFSIZE) break;

        f.read(buf, size);
        std::string label(buf, buf + size);

        // Read status
        char status_char;
        f.get(status_char);
        if (!f) break;

        SigningScript::status_t status;
        switch ((int)status_char) {
        case 1:
            status = SigningScript::UNUSED;
            break;
        case 2:
            status = SigningScript::CHANGE;
            break;
        case 4:
            status = SigningScript::REQUEST;
            break;
        case 8:
            status = SigningScript::RECEIPT;
            break;
        default:
            status = SigningScript::ALL;
        }
        if (status == SigningScript::ALL) break;

        // Read txinscript
        f.read(sizebuf, 4);
        if (!f) break;

        size = sizebuf_to_uint(sizebuf);
        uchar_vector txinscript;
        while (size > MAXBUFSIZE) {
            f.read(buf, MAXBUFSIZE);
            if (!f) break;

            txinscript += bytes_t((unsigned char*)buf, (unsigned char*)buf + MAXBUFSIZE);
            size -= MAXBUFSIZE;            
        }
        if (size > MAXBUFSIZE) break;

        f.read(buf, size);
        if (!f) break;

        txinscript += bytes_t((unsigned char*)buf, (unsigned char*)buf + size);

        // Read txoutscript
        f.read(sizebuf, 4);
        if (!f) break;

        size = sizebuf_to_uint(sizebuf);
        uchar_vector txoutscript;
        while (size > MAXBUFSIZE) {
            f.read(buf, MAXBUFSIZE);
            if (!f) break;

            txoutscript += bytes_t((unsigned char*)buf, (unsigned char*)buf + MAXBUFSIZE);
            size -= MAXBUFSIZE;            
        }
        if (size > MAXBUFSIZE) break;

        f.read(buf, size);
        if (!f) break;

        txoutscript += bytes_t((unsigned char*)buf, (unsigned char*)buf + size);
 
        std::shared_ptr<SigningScript> script(new SigningScript(
            txinscript, txoutscript, label, status));

        scripts.push_back(script);
        hash = sha256(hash + txinscript + txoutscript);
    }
    if (i < num_scripts) {
        std::stringstream err;
        err << "Vault::importAccount - error reading script at position " << i << ".";
        throw std::runtime_error(err.str());
    }

    // Read hash
    f.read(sizebuf, 4);
    if (!f) {
        throw std::runtime_error("Vault::importAccount - error reading hash.");
    }

    size = sizebuf_to_uint(sizebuf);
    if (size > MAXBUFSIZE) {
        throw std::runtime_error("Vault::importAccount - error reading hash.");
    }

    f.read(buf, size);
    if (!f) {
        throw std::runtime_error("Vault::importAccount - error reading hash.");
    }

    bytes_t read_hash((unsigned char*)buf, (unsigned char*)buf + size);
    if (hash != read_hash) {
        throw std::runtime_error("Vault::importAccount - hash mismatch.");
    }

    Account account(is_ours);
    account.set(account_name, minsigs, keychain_hashes, scripts, time_created);

    boost::lock_guard<boost::mutex> lock(mutex);

    odb::core::transaction t(db_->begin());

    for (auto& script: scripts) {
        if (!script->label().empty()) {
            // Make sure a tag for the script doesn't already exist.
            // TODO: determine whether behavior should be as it is here, where duplicate scripts are ignored - or whether to raise exception
            odb::result<ScriptTag> tag_r(db_->query<ScriptTag>(odb::query<ScriptTag>::txoutscript == script->txoutscript()));
            if (tag_r.empty()) {
                std::shared_ptr<ScriptTag> tag(new ScriptTag(script->txoutscript(), script->label()));
                db_->persist(tag); 
            }
            else {
                LOGGER(error) << "Vault::importAccount - ScriptTag already exists for txoutscript: " << uchar_vector(script->txoutscript()).getHex() << std::endl;
            }
        }
        db_->persist(script);
    }
    db_->persist(account);

    t.commit();

    return hash;
}

std::vector<std::shared_ptr<SigningScriptView>> Vault::getSigningScriptViews(const std::string& account_name, int flags) const
{
    if (!accountExists(account_name)) {
        throw std::runtime_error("Account not found.");
    }

    // Map flags to status_t vector
    std::vector<SigningScript::status_t> stati;
    if (flags & SigningScript::UNUSED)   stati.push_back(SigningScript::UNUSED);
    if (flags & SigningScript::CHANGE)   stati.push_back(SigningScript::CHANGE);
    if (flags & SigningScript::REQUEST)  stati.push_back(SigningScript::REQUEST);
    if (flags & SigningScript::RECEIPT)  stati.push_back(SigningScript::RECEIPT);

    std::vector<std::shared_ptr<SigningScriptView>> views;

    boost::lock_guard<boost::mutex> lock(mutex);

    odb::core::transaction t(db_->begin());

    typedef odb::query<SigningScriptView> query;
    odb::result<SigningScriptView> r(db_->query<SigningScriptView>(
        query::Account::name == account_name && query::SigningScript::status.in_range(stati.begin(), stati.end()))
    );
    for (auto i =  r.begin(); i != r.end(); ++i) {
        std::shared_ptr<SigningScriptView> view(i.load());
        views.push_back(view);
    }

    return views;
}

unsigned long Vault::getScriptCount(const std::string& account_name, SigningScript::status_t status) const
{
    if (!accountExists(account_name)) {
        throw std::runtime_error("Account not found.");
    }

    boost::lock_guard<boost::mutex> lock(mutex);

    odb::core::transaction t(db_->begin());

    typedef odb::query<ScriptCountView> query;
    odb::result<ScriptCountView> r(db_->query<ScriptCountView>(query::Account::name == account_name && query::SigningScript::status == status));

    return r.begin()->count;
}

unsigned long Vault::generateLookaheadScripts(const std::string& account_name, unsigned long lookahead)
{
    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session session;
    odb::core::transaction t(db_->begin());

    lookahead = generateLookaheadScripts_unwrapped(account_name, lookahead);

    t.commit();

    return lookahead;
}

unsigned long Vault::generateLookaheadScripts_unwrapped(const std::string& account_name, unsigned long lookahead)
{
    odb::result<Account> account_r(db_->query<Account>(odb::query<Account>::name == account_name));
    if (account_r.empty()) {
        std::stringstream err;
        err << "Account " << account_name << " not found.";
        throw std::runtime_error(err.str());
    }

    std::shared_ptr<Account> account(account_r.begin().load());

    typedef odb::query<ScriptCountView> scriptcount_query;
    odb::result<ScriptCountView> lookaheadcount_r(db_->query<ScriptCountView>(scriptcount_query::Account::name == account_name && scriptcount_query::SigningScript::status == SigningScript::UNUSED));

    unsigned long lookaheadcount = lookaheadcount_r.begin()->count;
    if (lookaheadcount >= lookahead) return lookaheadcount;

    const std::set<bytes_t>& hashes = account->keychain_hashes();
    odb::result<Keychain> keychain_r(db_->query<Keychain>(odb::query<Keychain>::hash.in_range(hashes.begin(), hashes.end())));
    Account::keychains_t keychains;
    for (auto& keychain: keychain_r) { keychains.insert(std::make_shared<Keychain>(keychain)); }
    if (hashes.size() != keychains.size()) return lookaheadcount;

    uint32_t scriptcount = account->scripts().size();
    uint32_t newscriptcount = scriptcount + lookahead - lookaheadcount;

    // All keychains are in vault, so let's calculate the maximum number of new scripts we can get.
    for (auto& keychain: keychains) {
        if (!keychain->is_deterministic() && keychain->numkeys() < newscriptcount) {
            newscriptcount = keychain->numkeys();
        }
    }

    // we cannot create enough keys
    if (newscriptcount <= scriptcount) return lookaheadcount;

    for (auto& keychain: keychains) {
        if (keychain->is_deterministic()) {
            keychain->numkeys(newscriptcount);
            updateKeychain_unwrapped(keychain);
        }
    }

    account->extend(keychains);
    for (unsigned long i = scriptcount; i < newscriptcount; i++) { db_->persist(account->scripts()[i]); }
    db_->update(account);

    return lookahead;
}
 
bool Vault::scriptTagExists(const bytes_t& txoutscript) const
{
    boost::lock_guard<boost::mutex> lock(mutex);

    odb::core::transaction t(db_->begin());

    odb::result<ScriptTag> r(db_->query<ScriptTag>(odb::query<ScriptTag>::txoutscript == txoutscript));
    return (!r.empty()); 
}

void Vault::addScriptTag(const bytes_t& txoutscript, const std::string& description)
{
    boost::lock_guard<boost::mutex> lock(mutex);

    odb::core::transaction t(db_->begin());

    odb::result<ScriptTag> r(db_->query<ScriptTag>(odb::query<ScriptTag>::txoutscript == txoutscript));
    if (!r.empty()) {
        throw std::runtime_error("Script is already tagged.");
    }

    ScriptTag tag(txoutscript, description);
    db_->persist(tag);

    t.commit();
}

void Vault::updateScriptTag(const bytes_t& txoutscript, const std::string& description)
{
    boost::lock_guard<boost::mutex> lock(mutex);

    odb::core::transaction t(db_->begin());

    odb::result<ScriptTag> r(db_->query<ScriptTag>(odb::query<ScriptTag>::txoutscript == txoutscript));
    if (r.empty()) {
        throw std::runtime_error("Tag not found.");
    }

    std::shared_ptr<ScriptTag> tag(r.begin().load());
    tag->description(description);
    db_->update(tag);

    t.commit();
}

void Vault::deleteScriptTag(const bytes_t& txoutscript)
{
    boost::lock_guard<boost::mutex> lock(mutex);

    odb::core::transaction t(db_->begin());

    odb::result<ScriptTag> r(db_->query<ScriptTag>(odb::query<ScriptTag>::txoutscript == txoutscript));
    if (r.empty()) {
        throw std::runtime_error("Tag not found.");
    }

    std::shared_ptr<ScriptTag> tag(r.begin().load());
    db_->erase(tag);

    t.commit();
}

std::vector<std::shared_ptr<TxOut>> Vault::getTxOuts(const std::string& account_name) const
{
    if (!accountExists(account_name)) {
        throw std::runtime_error("Account not found.");
    }

    std::vector<std::shared_ptr<TxOut>> txouts;

    boost::lock_guard<boost::mutex> lock(mutex);

//    odb::core::session session;

    odb::core::transaction t(db_->begin());

    typedef odb::query<TxOut> query;

    // TODO: figure out this join query stuff once and for all
    odb::result<TxOut> r(db_->query<TxOut>(query::signingscript.is_not_null() && query::spent.is_null()));
    for (auto i = r.begin(); i != r.end(); ++i) {
        std::shared_ptr<TxOut> txout(i.load());
        if (txout->signingscript()->account()->name() == account_name) { txouts.push_back(txout); }
    }

    t.commit();

    return txouts;
}

std::vector<std::shared_ptr<TxOutView>> Vault::getTxOutViews(const std::string& account_name, bool unspentonly) const
{
    if (!accountExists(account_name)) {
        throw std::runtime_error("Account not found.");
    }

    std::vector<std::shared_ptr<TxOutView>> txoutviews;

    boost::lock_guard<boost::mutex> lock(mutex);

    odb::core::transaction t(db_->begin());

    typedef odb::query<TxOutView> query;
    odb::result<TxOutView> r;
    if (unspentonly) {
        r = db_->query<TxOutView>(query::TxOut::spent.is_null() && query::Account::name == account_name);
    }
    else {
        r = db_->query<TxOutView>(query::Account::name == account_name);
    }

    for (auto i = r.begin(); i != r.end(); ++i)  {
        std::shared_ptr<TxOutView> txoutview(i.load());
        txoutviews.push_back(txoutview);
    }
    return txoutviews;
}

std::shared_ptr<TxOut> Vault::newTxOut(const std::string& account_name, const std::string& label, uint64_t value)
{
    if (!accountExists(account_name)) {
        throw std::runtime_error("Account not found.");
    }

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session session;
    odb::core::transaction t(db_->begin());
    //t.tracer(odb::stderr_tracer);

    typedef odb::query<SigningScriptView> view_query;

    odb::result<SigningScriptView> view_result(db_->query<SigningScriptView>((view_query::Account::name == account_name && view_query::SigningScript::status == SigningScript::UNUSED) +
        "ORDER BY" + view_query::SigningScript::id + "LIMIT 1"));
    if (view_result.empty()) {
        throw std::runtime_error("No scripts available.");
    }

    std::shared_ptr<SigningScriptView> view(view_result.begin().load());

    typedef odb::query<SigningScript> script_query;
    odb::result<SigningScript> script_result(db_->query<SigningScript>(script_query::id == view->id));
    std::shared_ptr<SigningScript> script(script_result.begin().load());
    script->label(label);
    script->status(SigningScript::REQUEST);
    db_->update(script);

    // TODO: assert the following returns LOOKAHEAD
    generateLookaheadScripts_unwrapped(account_name, LOOKAHEAD);

    ScriptTag tag(script->txoutscript(), label);
    db_->persist(tag);

    t.commit();

    std::shared_ptr<TxOut> txout(new TxOut(value, script->txoutscript()));

    return txout;
}

std::vector<std::shared_ptr<AccountTxOutView>> Vault::getAccountHistory(const std::string& account_name, int type_flags, int status_flags, uint32_t min_height, uint32_t max_height) const
{
    if (!accountExists(account_name)) {
        throw std::runtime_error("Account not found.");
    }

    // Map type_flags to type vector
    std::vector<TxOut::type_t> types;
    if (type_flags & TxOut::NONE)   types.push_back(TxOut::NONE);
    if (type_flags & TxOut::CHANGE) types.push_back(TxOut::CHANGE);
    if (type_flags & TxOut::DEBIT)  types.push_back(TxOut::DEBIT);
    if (type_flags & TxOut::CREDIT) types.push_back(TxOut::CREDIT);

    // Map status_flags to type vector
    std::vector<Tx::status_t> stati;
    if (status_flags & Tx::UNSIGNED) stati.push_back(Tx::UNSIGNED);
    if (status_flags & Tx::UNSENT)   stati.push_back(Tx::UNSENT);
    if (status_flags & Tx::SENT)     stati.push_back(Tx::SENT);
    if (status_flags & Tx::RECEIVED) stati.push_back(Tx::RECEIVED);

    boost::lock_guard<boost::mutex> lock(mutex);

    odb::core::transaction t(db_->begin());

    typedef odb::query<AccountTxOutView> query;
    odb::result<AccountTxOutView> r;
    if (max_height < 0xffffffff) {
        r = db_->query<AccountTxOutView>((query::Account::name == account_name && query::TxOut::type.in_range(types.begin(), types.end()) && query::Tx::status.in_range(stati.begin(), stati.end()) && query::BlockHeader::height.is_not_null() && query::BlockHeader::height >= min_height && query::BlockHeader::height <= max_height) +
            "ORDER BY" + query::Tx::id + "ASC");
    }
    else {
        r = db_->query<AccountTxOutView>((query::Account::name == account_name && query::TxOut::type.in_range(types.begin(), types.end()) && query::Tx::status.in_range(stati.begin(), stati.end()) && (query::BlockHeader::height.is_null() || query::BlockHeader::height >= min_height)) +
            "ORDER BY" + query::Tx::id + "ASC");
    }

    std::vector<std::shared_ptr<AccountTxOutView>> history;
    for (auto i = r.begin(); i != r.end(); ++i) {
        std::shared_ptr<AccountTxOutView> item(i.load());
        history.push_back(item);
    }

    return history;
}

// TODO: greater flexibility in how conflicts are handled.
bool Vault::addTx(std::shared_ptr<Tx> tx, bool delete_conflicting_txs)
{
    LOGGER(trace) << "Vault::addTx(" << uchar_vector(tx->hash()).getHex() << ", " << (delete_conflicting_txs ? "true" : "false") << ")" << std::endl;

    if (!Vault::isSigned(tx)) {
        tx->status(Tx::UNSIGNED);
        tx->updateHash();
        LOGGER(debug) << "Vault::addTx - transaction is still missing signatures - unsigned_hash: " << uchar_vector(tx->hash()).getHex() << std::endl;
    }

    using namespace CoinQ::Script;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session session;
    odb::core::transaction t(db_->begin());
//    t.tracer(odb::stderr_tracer);

    odb::result<Tx> tx_result(db_->query<Tx>(odb::query<Tx>::hash == tx->hash() || odb::query<Tx>::hash == tx->unsigned_hash()));
    if (!tx_result.empty()) {
        LOGGER(debug) << "Vault::addTx - we have a transaction with the same hash. hash: " << uchar_vector(tx->hash()).getHex() << std::endl;
        // we already have it - check if status has changed.
        std::shared_ptr<Tx> stored_tx(tx_result.begin().load());
        if (stored_tx->status() == Tx::UNSIGNED && stored_tx->txins().size() == tx->txins().size()) {
            // we have an unsigned version - replace the inputs (TODO: only add new signatures, do not remove existing ones.)
            LOGGER(debug) << "Vault::addTx - we need to replace an older version of the transaction with a newer version." << std::endl;
            std::size_t i = 0;
            for (auto& txin: stored_tx->txins()) {
                txin->script(tx->txins()[i++]->script());
                db_->update(txin);
            }
            if (Vault::isSigned(stored_tx)) {
                if (tx->status() == Tx::RECEIVED) {
                    stored_tx->status(Tx::RECEIVED);
                }
                else {
                    stored_tx->status(Tx::UNSENT);
                }
                stored_tx->updateHash();
                db_->update(stored_tx);
            }
            updateUnconfirmed_unwrapped(stored_tx->hash());
            t.commit();
            notifyAddTx(stored_tx->toCoinClasses());
            return true;
        }
        if (tx->status() != stored_tx->status()) {
            LOGGER(debug) << "Vault::addTx - the status of the transaction has changed. old status: " << stored_tx->status() << " - new status: " << tx->status() << std::endl;
            stored_tx->timestamp(time(NULL));
            stored_tx->status(tx->status());
            db_->update(stored_tx);
            updateUnconfirmed_unwrapped(stored_tx->hash());
            t.commit();
            notifyAddTx(stored_tx->toCoinClasses());
            return true;
        }
        return false;
    }

    std::set<std::shared_ptr<Tx>> conflicting_txs;
    std::set<std::shared_ptr<TxOut>> updated_txouts;

    // Check inputs 
    bool sent_from_vault = false; // whether any of the inputs belong to vault
    bool have_all_inputs = true; // whether we have all outpoints (for fee calculation)
    uint64_t input_total = 0;
    std::shared_ptr<Account> signing_account;
    // TODO: check input redeemscripts
    for (auto& txin: tx->txins()) {
        // Check dependency - if inputs connect
        odb::result<Tx> tx_result(db_->query<Tx>(odb::query<Tx>::hash == txin->outhash()));
        if (tx_result.empty()) {
            have_all_inputs = false;
        }
        else {
            std::shared_ptr<Tx> spent_tx(tx_result.begin().load());
            const Tx::txouts_t& txouts = spent_tx->txouts();
            uint32_t outindex = txin->outindex();
            if (txouts.size() <= outindex) {
                throw std::runtime_error("Vault::addTx - outpoint out of range.");
            }
            std::shared_ptr<TxIn> conflict_txin = txouts[outindex]->spent();
            if (conflict_txin) {
                if (!delete_conflicting_txs) {
                    LOGGER(debug) << "Vault::addTx - outpoint already spent - outpoint: " << uchar_vector(conflict_txin->outhash()).getHex() << ":" << conflict_txin->outindex() << std::endl;
                    throw std::runtime_error("Vault::addTx - outpoint already spent.");
                }
                LOGGER(debug) << "Discovered conflicting transaction " << uchar_vector(conflict_txin->tx()->hash()).getHex() << std::endl;
                conflicting_txs.insert(conflict_txin->tx());
            }
            input_total += txouts[outindex]->value();
            // Is it our own siging script?
            odb::result<SigningScript> r(db_->query<SigningScript>(odb::query<SigningScript>::txoutscript == txouts[outindex]->script()));
            if (!r.empty()) {
                // Not only is the transaction in vault - we signed it.
                std::shared_ptr<SigningScript> signingscript(r.begin().load());
                sent_from_vault = true;
                signing_account = signingscript->account();
                txouts[outindex]->spent(txin);
                updated_txouts.insert(txouts[outindex]);
            }
        }
    }

    // Check outputs
    bool sent_to_vault = false; // whether any of the outputs belong to vault
    uint64_t output_total = 0;
    std::set<std::shared_ptr<SigningScript>> vault_scripts;
    for (auto& txout: tx->txouts()) {
        output_total += txout->value();
        odb::result<SigningScript> r(db_->query<SigningScript>(odb::query<SigningScript>::txoutscript == txout->script()));
        if (!r.empty()) {
            sent_to_vault = true;
            std::shared_ptr<SigningScript> vault_script(r.begin().load());
            txout->account_id(vault_script->account()->id());
            switch (vault_script->status()) {
            case SigningScript::UNUSED:
                if (sent_from_vault) {
                    vault_script->status(SigningScript::CHANGE);
                    txout->type(TxOut::CHANGE);
                }
                else {
                    vault_script->status(SigningScript::RECEIPT);
                    txout->type(TxOut::CREDIT);
                }
                generateLookaheadScripts_unwrapped(vault_script->account()->name(), LOOKAHEAD);
                break;

            case SigningScript::CHANGE:
                txout->type(TxOut::CHANGE);
                break;

            case SigningScript::REQUEST:
                vault_script->status(SigningScript::RECEIPT);
                txout->type(TxOut::CREDIT);
                break;

            case SigningScript::RECEIPT:
                txout->type(TxOut::CREDIT);
                break;

            default:
                break;
            }
            vault_script->addTxOut(txout);
            // Put into container so we can update once we've inserted the txout.
            vault_scripts.insert(vault_script);

            // We also need to check whether outputs belonging to us have been spent
            odb::result<TxIn> txin_r(db_->query<TxIn>(odb::query<TxIn>::outhash == tx->hash() && odb::query<TxIn>::outindex == txout->txindex()));
            if (!txin_r.empty()) {
                std::shared_ptr<TxIn> txin(txin_r.begin().load());
                txout->spent(txin);
            }
        }
        else if (signing_account) {
            // Assume we own all inputs.
            // TODO: allow coin mixing
            txout->account_id(signing_account->id());
            txout->type(TxOut::DEBIT);
        }
    }

    for (auto& tx: conflicting_txs) { deleteTx_unwrapped(tx->hash()); }

    if (sent_from_vault || sent_to_vault) {
        for (auto& txin:   tx->txins())   { db_->persist(*txin); }
        for (auto& txout:  tx->txouts())  { db_->persist(*txout); }
        for (auto& script: vault_scripts) { db_->update(*script); }
        if (have_all_inputs) { tx->fee(input_total - output_total); }
        db_->persist(*tx);

        for (auto& txout: updated_txouts) { db_->update(txout); }

        updateUnconfirmed_unwrapped(tx->hash());
        t.commit();
        notifyAddTx(tx->toCoinClasses());
        return true;
    }

    return false;
}

Vault::result_t Vault::insertTx_unwrapped(std::shared_ptr<Tx>& tx, bool delete_conflicting_txs)
{
    LOGGER(trace) << "Vault::insertTx_unwrapped(" << uchar_vector(tx->hash()).getHex() << ", " << (delete_conflicting_txs ? "true" : "false") << ")" << std::endl;

    return OBJECT_UNCHANGED;
}

bool Vault::deleteTx(const bytes_t& txhash)
{
    boost::lock_guard<boost::mutex> lock(mutex);

    odb::core::session session;
    odb::core::transaction t(db_->begin());
    if (!deleteTx_unwrapped(txhash)) return false;
    t.commit();
    return true;
}

bool Vault::deleteTx_unwrapped(const bytes_t& txhash)
{
    LOGGER(trace) << "Vault::deleteTx_unwrapped(" << uchar_vector(txhash).getHex() << std::endl;

    odb::result<Tx> tx_r(db_->query<Tx>(odb::query<Tx>::hash == txhash));
    if (tx_r.empty()) return false;

    std::shared_ptr<Tx> tx(tx_r.begin().load());

    // go through all claimed txouts, unclaim them
    typedef odb::result<TxParentChildView> deps_result;
    typedef odb::query<TxParentChildView> deps_query;
    deps_result deps_r(db_->query<TxParentChildView>(deps_query::child_tx::id == tx->id()));
    for (auto& parent: deps_r) {
        odb::result<TxOut> txout_r(db_->query<TxOut>(odb::query<TxOut>::id == parent.txout_id));
        std::shared_ptr<TxOut> txout(txout_r.begin().load());
        txout->spent(NULL);
        db_->update(txout);
    }

    for (auto& txin:  tx->txins())  { db_->erase(txin); }
    for (auto& txout: tx->txouts()) { db_->erase(txout); }
    db_->erase(tx);

    return true;
}

std::shared_ptr<Tx> Vault::newTx(const std::string& account_name, uint32_t tx_version, uint32_t tx_locktime, const Tx::txouts_t& payment_txouts, uint64_t fee, unsigned int maxchangeouts)
{
    if (!accountExists(account_name)) {
        throw std::runtime_error("Account not found.");
    }

    Tx::txins_t txins;
    Tx::txouts_t txouts(payment_txouts);

    // TODO: better fee calculation heuristics
    uint64_t desired_total = fee;
    for (auto& txout: txouts) { desired_total += txout->value(); }

    std::vector<std::shared_ptr<TxOutView>>  utxo_views = getTxOutViews(account_name, true);
    std::random_shuffle(utxo_views.begin(), utxo_views.end());

    int i = 0;
    uint64_t total = 0;
    for (auto& utxo_view: utxo_views) {
        total += utxo_view->value;
        std::shared_ptr<TxIn> txin(new TxIn(utxo_view->txhash, utxo_view->txindex, utxo_view->signingscript_txinscript, 0xffffffff));
        txins.push_back(txin);
        i++;
        if (total >= desired_total) break;
    }

    if (total < desired_total) {
        throw std::runtime_error("Insufficient funds.");
    }

    utxo_views.resize(i);
    uint64_t change = total - desired_total;

    // TODO: put the script update and addTx operations inside a single session, transaction
    if (change > 0) {
        boost::lock_guard<boost::mutex> lock(mutex);
        odb::core::session session;
        odb::core::transaction t(db_->begin());

        typedef odb::query<SigningScript> query;
        odb::result<SigningScript> changescript_result =
            db_->query<SigningScript>(query::account->name == account_name && query::status == SigningScript::UNUSED);

        if (changescript_result.empty()) {
            throw std::runtime_error("Out of change scripts.");
        }
        std::shared_ptr<SigningScript> changescript(changescript_result.begin().load());
        changescript->status(SigningScript::CHANGE);

        db_->update(changescript);

        generateLookaheadScripts_unwrapped(account_name, LOOKAHEAD);

        t.commit();

        std::shared_ptr<TxOut> txout(new TxOut(change, changescript->txoutscript()));
        txouts.push_back(txout);
    }
    std::random_shuffle(txouts.begin(), txouts.end());

    std::shared_ptr<Tx> tx(new Tx());
    tx->set(tx_version, txins, txouts, tx_locktime, time(NULL), Tx::UNSIGNED);

    // let's not complicate this yet
    // addTx(tx);

    return tx;
}


bool Vault::isSigned(std::shared_ptr<Tx> tx)
{
    for (auto& txin: tx->txins()) {
        CoinQ::Script::Script script(txin->script());
        if (script.sigsneeded() > 0) return false;
    }

    return true;
}

// TODO: Pass decryption keys
std::shared_ptr<Tx> Vault::signTx(std::shared_ptr<Tx> tx) const
{
    unsigned int sigsadded = 0;
    unsigned int txin_i = 0;
    for (auto& txin: tx->txins()) {
        CoinQ::Script::Script script(txin->script());
        unsigned int sigsneeded = script.sigsneeded();
        if (sigsneeded > 0 ) {
            std::vector<bytes_t> pubkeys = script.missingsigs();
            if (!pubkeys.empty()) {
                boost::lock_guard<boost::mutex> lock(mutex);

                odb::core::transaction t(db_->begin());

                typedef odb::query<Key> query;
                odb::result<Key> r(db_->query<Key>(query::is_private != 0 && query::pubkey.in_range(pubkeys.begin(), pubkeys.end())));
                if (!r.empty()) {
                    // We've found an input we can sign. Compute the hash to sign.
                    // TODO: compute the hash without constructing a new instance and directly accessing Coin::Transaction internal data
                    Coin::Transaction cointx = tx->toCoinClasses();
                    unsigned int i = 0;
                    for (auto& coininput: cointx.inputs) {
                        if (i == txin_i) {
                            coininput.scriptSig = script.txinscript(CoinQ::Script::Script::SIGN);
                        }
                        else {
                            coininput.scriptSig.clear();
                        }
                        i++;
                    }
                    bytes_t hash = cointx.getHashWithAppendedCode(CoinQ::Script::SIGHASH_ALL);
//    std::cout << "tx to hash: " << cointx.getSerialized().getHex() << std::endl;
//    std::cout << "hash to sign: " << uchar_vector(hash).getHex() << std::endl;

                    for (auto& key: r) {
                        // TODO: replace CoinKey with secp256k1 class
                        CoinKey coinkey;
                        if (!coinkey.setPrivateKey(key.privkey())) {
                            std::stringstream err;
                            err << "Invalid private key for public key " << uchar_vector(key.pubkey()).getHex() << ".";
                            throw std::runtime_error(err.str());
                        }
                        if (coinkey.getPublicKey() != key.pubkey()) {
                            std::stringstream err;
                            err << "Private key does not correspond to public key " << uchar_vector(key.pubkey()).getHex() << ".";
                            throw std::runtime_error(err.str());
                        }

                        uchar_vector sig;
                        if (!coinkey.sign(hash, sig)) {
                            std::stringstream err;
                            err << "Signing failed for public key " << uchar_vector(key.pubkey()).getHex() << ".";
                            throw std::runtime_error(err.str());
                        }
                        sig.push_back(CoinQ::Script::SIGHASH_ALL);

                        script.addSig(key.pubkey(), sig);
                        sigsadded++;

                        sigsneeded--;
                        if (sigsneeded == 0) break;
                    }
                    CoinQ::Script::Script::sigtype_t sigtype = (sigsneeded == 0) ? CoinQ::Script::Script::BROADCAST : CoinQ::Script::Script::EDIT;
                    txin->script(script.txinscript(sigtype));
                }
            }
        }
        txin_i++;
    }

    if (sigsadded == 0) {
        throw std::runtime_error("No new signatures were added.");
    }

    if (Vault::isSigned(tx)) {
        tx->status(Tx::UNSENT);
    }

    tx->updateHash();

    return tx;
}

std::shared_ptr<Tx> Vault::getTx(const bytes_t& hash) const
{
    boost::lock_guard<boost::mutex> lock(mutex);

    odb::core::session session;

    odb::core::transaction t(db_->begin());

    odb::result<Tx> r(db_->query<Tx>(odb::query<Tx>::hash == hash));
    if (r.empty()) {
        throw std::runtime_error("Transaction not found");
    }

    std::shared_ptr<Tx> tx(r.begin().load());
    return tx;
}

bool Vault::insertBlock(const ChainBlock& block, bool reprocess_txs)
{
    bytes_t prev_block_hash = block.blockHeader.prevBlockHash;
    bytes_t block_hash = block.blockHeader.getHashLittleEndian();
    LOGGER(trace) << "Vault::insertBlock(" << uchar_vector(block_hash).getHex() << (reprocess_txs ? ", true" : ", false") << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);

    odb::core::session session;

    odb::core::transaction t(db_->begin());

    odb::result<BlockHeader> block_r;
    block_r = db_->query<BlockHeader>(odb::query<BlockHeader>::hash == prev_block_hash);
    if (block_r.empty() && block.blockHeader.timestamp > getFirstAccountTimeCreated_unwrapped()) return false;

    block_r = db_->query<BlockHeader>(odb::query<BlockHeader>::hash == block_hash);
    std::shared_ptr<BlockHeader> header;
    if (!block_r.empty()) {
        header = block_r.begin().load();
        if (header->hash() == block_hash) {
            LOGGER(debug) << "Vault::insertBlock - already have block - hash: " << uchar_vector(block_hash).getHex() << " height: " << header->height() << std::endl;
            return true;
        }
        else {
            LOGGER(debug) << "Vault::insertBlock - block collision - new hash: " << uchar_vector(block_hash).getHex() << " old hash: " << uchar_vector(header->hash()).getHex() << " height: " << header->height() << std::endl;
            deleteBlock_unwrapped(header->hash());
        }
    }

    LOGGER(debug) << "Vault::insertBlock - inserting new block: " << uchar_vector(block_hash).getHex() << " height: " << block.height << std::endl;
    header = std::make_shared<BlockHeader>(
        block_hash,
        block.height,
        block.blockHeader.version,
        block.blockHeader.prevBlockHash,
        block.blockHeader.merkleRoot,
        block.blockHeader.timestamp,
        block.blockHeader.bits,
        block.blockHeader.nonce
    );

    db_->persist(header);

    std::vector<bytes_t> tx_hashes;
    std::map<bytes_t, uint32_t> index_map;
    uint32_t i = 0;
    for (auto& coin_tx: block.txs) {
        bytes_t hash = coin_tx.getHashLittleEndian();
        index_map[hash] = i++;
        tx_hashes.push_back(coin_tx.getHashLittleEndian());
    }

    odb::result<Tx> tx_r;
    std::size_t num_txs = tx_hashes.size();
    std::size_t startpos = 0;
    while (startpos < num_txs) {
        std::size_t endpos = std::min(num_txs, startpos + MAXSQLCLAUSES);
        tx_r = db_->query<Tx>(odb::query<Tx>::hash.in_range(tx_hashes.begin() + startpos, tx_hashes.begin() + endpos));
        startpos = endpos;
        for (auto& tx: tx_r) {
            LOGGER(debug) << "Vault::insertBlock - setting tx block header for tx " << uchar_vector(tx.hash()).getHex() << std::endl;
            tx.block(header, index_map[tx.hash()]);
            db_->update(tx);
        }
    }

    t.commit();
    notifyInsertBlock(block);
    return true;
}

bool Vault::deleteBlock_unwrapped(const bytes_t& hash)
{
    // TODO
    return false;
}

bool Vault::deleteBlock(const bytes_t& hash)
{
    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session session;
    odb::core::transaction t(db_->begin());
    return deleteBlock_unwrapped(hash); 
}

bool Vault::insertMerkleBlock(const ChainMerkleBlock& merkleblock)
{
    Coin::PartialMerkleTree partialmerkletree(merkleblock.nTxs, merkleblock.hashes, merkleblock.flags, merkleblock.blockHeader.merkleRoot);
    std::vector<uchar_vector> tx_hashes = partialmerkletree.getTxHashesLittleEndianVector();
    bytes_t prev_block_hash = merkleblock.blockHeader.prevBlockHash;
    bytes_t block_hash = merkleblock.blockHeader.getHashLittleEndian();
    LOGGER(trace) << "Vault::insertMerkleBlock(" << uchar_vector(block_hash).getHex() << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session session;
    odb::core::transaction t(db_->begin());
    //t.tracer(odb::stderr_tracer);

    odb::result<BlockHeader> block_r;
    block_r = db_->query<BlockHeader>(odb::query<BlockHeader>::hash == prev_block_hash);
    if (block_r.empty() && merkleblock.blockHeader.timestamp > getFirstAccountTimeCreated_unwrapped()) return false;

    block_r = db_->query<BlockHeader>(odb::query<BlockHeader>::hash == block_hash);
    std::shared_ptr<BlockHeader> header;
    if (!block_r.empty()) {
        header = block_r.begin().load();
        LOGGER(debug) << "Vault::insertMerkleBlock - already have block - hash: " << uchar_vector(block_hash).getHex() << " height: " << header->height() << std::endl;
        return false;
    }
    else {
        block_r = db_->query<BlockHeader>(odb::query<BlockHeader>::height >= merkleblock.height);
        if (!block_r.empty()) {
            LOGGER(debug) << "Vault::insertMerkleBlock - reorganization - hash: " << uchar_vector(block_hash).getHex() << " height: " << merkleblock.height << std::endl;
            for (auto& h: block_r) {
                db_->erase_query<MerkleBlock>(odb::query<MerkleBlock>::blockheader == h.id());
                odb::result<Tx> tx_r = db_->query<Tx>(odb::query<Tx>::blockheader == h.id());
                for (auto& tx: tx_r) {
                    tx.blockheader(NULL);
                    db_->update(tx);
                }
            }
            db_->erase_query<BlockHeader>(odb::query<BlockHeader>::height >= merkleblock.height);
        }
    }

    LOGGER(debug) << "Vault::insertMerkleBlock - inserting new merkle block: " << uchar_vector(block_hash).getHex() << " height: " << merkleblock.height << std::endl;
    header = std::make_shared<BlockHeader>(
        block_hash,
        merkleblock.height,
        merkleblock.blockHeader.version,
        merkleblock.blockHeader.prevBlockHash,
        merkleblock.blockHeader.merkleRoot,
        merkleblock.blockHeader.timestamp,
        merkleblock.blockHeader.bits,
        merkleblock.blockHeader.nonce
    );

    std::vector<bytes_t> treehashes;
    for (auto& treehash: merkleblock.hashes) { treehashes.push_back(uchar_vector(treehash).getReverse()); }
    std::shared_ptr<MerkleBlock> db_merkleblock = std::make_shared<MerkleBlock>(
        header,
        merkleblock.nTxs,
        treehashes,
        merkleblock.flags
    );

    db_->persist(header);
    db_->persist(db_merkleblock);

    LOGGER(debug) << "Vault::insertMerkleBlock - updating transactions in block: " << uchar_vector(block_hash).getHex() << " height: " << merkleblock.height << std::endl;
    odb::result<Tx> tx_r = db_->query<Tx>(odb::query<Tx>::hash.in_range(tx_hashes.begin(), tx_hashes.end()));
    for (auto& tx: tx_r) {
        LOGGER(debug) << "Vault::insertMerkleBlock - updating transaction: " << uchar_vector(tx.hash()).getHex() << std::endl;
        tx.block(header, 0xffffffff); // TODO: compute correct index or eliminate index altogether.
        db_->update(tx);
    }

    LOGGER(debug) << "Vault::insertMerkleBlock - updating all other unconfirmed transactions" << std::endl;
    int count = updateUnconfirmed_unwrapped();
    LOGGER(debug) << "Vault::insertMerkleBlock - " << count << " transactions confirmed" << std::endl;

    t.commit();

    return true;
}


bool Vault::deleteMerkleBlock_unwrapped(const bytes_t& hash)
{
/
    odb::result<BlockHeader> blockheader_r = db_->query<BlockHeader>(odb::query<BlockHeader>::hash == hash);
    if (blockheader_r.empty()) {
        LOGGER(debug) << "Vault::deleteMerkleBlock_unwrapped - header not found for hash " << uchar_vector(hash).getHex() << std::endl;
        return false;
    }
/
//    std::shared_ptr<BlockHeader> header = blockheader_r.begin().load();
    odb::result<MerkleBlock> merkleblock_r = db_->query<MerkleBlock>(odb::query<MerkleBlock>::blockheader->hash == hash);
    if (merkleblock_r.empty()) {
        LOGGER(debug) << "Vault::deleteMerkleBlock_unwrapped - no merkle block found for hash " << uchar_vector(hash).getHex() << std::endl;
        return false;
    }

    LOGGER(debug) << "Vault::deleteMerkleBlock_unwrapped - deleting merkle block and header for hash " << uchar_vector(hash).getHex() << std::endl;
    db_->erase_query<MerkleBlock>(odb::query<MerkleBlock>::blockheader->hash == hash);
    db_->erase_query<BlockHeader>(odb::query<BlockHeader>::hash == hash);

    return true;
}

bool Vault::deleteMerkleBlock(const bytes_t& hash)
{
    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session session;
    odb::core::transaction t(db_->begin());
    bool changed = deleteMerkleBlock_unwrapped(hash);
    if (changed) { t.commit(); }
    return changed;
}

unsigned int Vault::updateUnconfirmed_unwrapped(const bytes_t& txhash)
{
    unsigned int count = 0;
    typedef odb::query<ConfirmedTxView> view_query;
    odb::result<ConfirmedTxView> txview_r;
    if (txhash.empty()) {
        txview_r = db_->query<ConfirmedTxView>(view_query::Tx::blockheader.is_null());
    }
    else {
        txview_r = db_->query<ConfirmedTxView>(view_query::Tx::hash == txhash && view_query::Tx::blockheader.is_null());
    }
    for (auto& txview: txview_r) {
        LOGGER(debug) << "Vault::updateUnconfirmed_unwrapped() - tx_id: " << txview.tx_id << " / blockheader_id: " << txview.blockheader_id << std::endl;
        if (txview.blockheader_id == 0) continue;

        odb::result<Tx> tx_r(db_->query<Tx>(txview.tx_id));
        if (tx_r.empty()) {
            // This should NEVER happen - but if it does, let's fail gracefully.
            throw std::runtime_error("Vault::updateUnconfirmed_unwrapped() - tx not found despite being in view.");
        }
        odb::result<BlockHeader> blockheader_r(db_->query<BlockHeader>(txview.blockheader_id));
        if (blockheader_r.empty()) {
            // This should NEVER happen - but if it does, let's fail gracefully.
            throw std::runtime_error("Vault::updateUnconfirmed_unwrapped() - blockheader not found despite being in view.");
        }
        std::shared_ptr<Tx> tx(db_->load<Tx>(txview.tx_id));
        std::shared_ptr<BlockHeader> blockheader(db_->load<BlockHeader>(txview.blockheader_id));
        tx->blockheader(blockheader);
        LOGGER(debug) << "Vault::updateUnconfirmed_unwrapped() - block hash: " << uchar_vector(tx->blockheader()->hash()).getHex() << std::endl;
        db_->update(*tx);
        count++;
    } 

    return count;
}

unsigned int Vault::updateUnconfirmed(const bytes_t& txhash)
{
    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session session;
    odb::core::transaction t(db_->begin());
    unsigned int count = updateUnconfirmed_unwrapped(txhash);
    if (count > 0) { t.commit(); }
    return count;
} 

uint32_t Vault::getFirstAccountTimeCreated_unwrapped() const
{
    odb::result<FirstAccountTimeCreatedView> r(db_->query<FirstAccountTimeCreatedView>());
    if (r.empty()) return 0xffffffff;

    std::shared_ptr<FirstAccountTimeCreatedView> view(r.begin().load());
    return view->time_created;
}

uint32_t Vault::getFirstAccountTimeCreated() const
{
    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::transaction t(db_->begin());
    return getFirstAccountTimeCreated_unwrapped();
}

uint32_t Vault::getBestHeight() const
{
    boost::lock_guard<boost::mutex> lock(mutex);

    odb::core::transaction t(db_->begin());

    odb::result<BestHeightView> r(db_->query<BestHeightView>());
    if (r.empty()) return 0;

    std::shared_ptr<BestHeightView> view(r.begin().load());
    return view->best_height;
}

std::shared_ptr<BlockHeader> Vault::getBestBlockHeader() const
{
    boost::lock_guard<boost::mutex> lock(mutex);

    odb::core::transaction t(db_->begin());

    odb::result<BlockHeader> r(db_->query<BlockHeader>("ORDER BY" + odb::query<BlockHeader>::height + "DESC LIMIT 1"));
    if (r.empty()) return NULL; 

    std::shared_ptr<BlockHeader> header(r.begin().load());
    return header;
}

std::vector<bytes_t> Vault::getLocatorHashes() const
{
    std::vector<bytes_t> hashes;
    std::vector<uint32_t> heights;

    boost::lock_guard<boost::mutex> lock(mutex);

    odb::core::transaction t(db_->begin());

    odb::result<BlockHeader> r(db_->query<BlockHeader>("ORDER BY" + odb::query<BlockHeader>::height + "DESC LIMIT 1"));
    if (!r.empty()) {
        std::shared_ptr<BlockHeader> header(r.begin().load());
        uint32_t i = header->height();
        uint32_t n = 1;
        uint32_t step = 1;
        heights.push_back(i);
        while (step <= i) {
            i -= step;
            n++;
            if (n > 10) step *= 2;
            heights.push_back(i);
        }
        r = db_->query<BlockHeader>(odb::query<BlockHeader>::height.in_range(heights.begin(), heights.end()) + "ORDER BY" + odb::query<BlockHeader>::height + "DESC");
        for (auto& header: r) {
            hashes.push_back(header.hash());
        }
    }

    return hashes;
}

Coin::BloomFilter Vault::getBloomFilter(double falsePositiveRate, uint32_t nTweak, uint32_t nFlags) const
{
    std::vector<bytes_t> elements;

    {
        // Get signing scripts from database
        boost::lock_guard<boost::mutex> lock(mutex);
        odb::core::transaction t(db_->begin());

        odb::result<SigningScriptView> scriptview_r = db_->query<SigningScriptView>();
        for (auto& scriptview: scriptview_r) {
            CoinQ::Script::Script script(scriptview.txinscript);
            elements.push_back(script.txinscript(CoinQ::Script::Script::SIGN));                     // Add input script element
            elements.push_back(CoinQ::Script::getScriptPubKeyPayee(scriptview.txoutscript).second); // Add output script element
        }
    }

    if (elements.size() == 0) return Coin::BloomFilter();

    Coin::BloomFilter bloomfilter(elements.size(), falsePositiveRate, nTweak, nFlags);
    for (auto& e: elements) { bloomfilter.insert(e); }

    return bloomfilter;
}
*/
