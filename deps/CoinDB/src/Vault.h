///////////////////////////////////////////////////////////////////////////////
//
// Vault.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.
//

#pragma once

#include "Schema.hxx"
#include "VaultExceptions.h"

#include <CoinQ_signals.h>
#include <CoinQ_slots.h>
#include <CoinQ_blocks.h>

#include <BloomFilter.h>

#include <boost/thread.hpp>

namespace CoinDB
{

class Vault
{
public:
    Vault(int argc, char** argv, bool create = false, uint32_t version = SCHEMA_VERSION);

#if defined(DATABASE_SQLITE)
    Vault(const std::string& filename, bool create = false, uint32_t version = SCHEMA_VERSION);
#endif

    /////////////////////////
    // KEYCHAIN OPERATIONS //
    /////////////////////////
    bool keychainExists(const std::string& keychain_name) const;
    std::shared_ptr<Keychain> newKeychain(const std::string& name, const secure_bytes_t& entropy, const secure_bytes_t& lockKey = secure_bytes_t(), const bytes_t& salt = bytes_t());
    //void eraseKeychain(const std::string& keychain_name) const;
    void renameKeychain(const std::string& old_name, const std::string& new_name);
    std::shared_ptr<Keychain> getKeychain(const std::string& keychain_name) const;
    std::vector<std::shared_ptr<Keychain>> getAllKeychains(bool root_only = false) const;
    //bytes_t exportKeychain(std::shared_ptr<Keychain> keychain, const std::string& filepath, bool exportprivkeys = false) const;
    //bytes_t importKeychain(const std::string& keychain_name, const std::string& filepath, bool& importprivkeys);
    //bool isKeychainFilePrivate(const std::string& filepath) const;

    ////////////////////////
    // ACCOUNT OPERATIONS //
    ////////////////////////
    bool accountExists(const std::string& account_name) const;
    void newAccount(const std::string& account_name, unsigned int minsigs, const std::vector<std::string>& keychain_names, uint32_t unused_pool_size = 25, uint32_t time_created = time(NULL));
    //void eraseAccount(const std::string& name) const;
    void renameAccount(const std::string& old_name, const std::string& new_name);
    std::shared_ptr<Account> getAccount(const std::string& account_name) const;
    AccountInfo getAccountInfo(const std::string& account_name) const;
    std::vector<AccountInfo> getAllAccountInfo() const;

    std::shared_ptr<AccountBin> addAccountBin(const std::string& account_name, const std::string& bin_name);
    std::shared_ptr<SigningScript> newSigningScript(const std::string& account_name, const std::string& bin_name = "@default", const std::string& label = "");

    // empty account_name or bin_name means do not filter on those fields
    std::vector<SigningScriptView> getSigningScriptViews(const std::string& account_name = "", const std::string& bin_name = "", int flags = SigningScript::ALL) const;

    ////////////////////////////
    // ACCOUNT BIN OPERATIONS //
    ////////////////////////////
    std::shared_ptr<AccountBin> getAccountBin(const std::string& account_name, const std::string& bin_name) const;

protected:
    // Keychain operations
    std::shared_ptr<Keychain> getKeychain_unwrapped(const std::string& keychain_name) const;
    void persistKeychain_unwrapped(std::shared_ptr<Keychain> keychain);

    // Account operations
    std::shared_ptr<Account> getAccount_unwrapped(const std::string& account_name) const;
    std::shared_ptr<SigningScript> newSigningScript_unwrapped(const std::string& account_name, const std::string& bin_name = "@default", const std::string& label = "");

    // AccountBin operations
    std::shared_ptr<AccountBin> getAccountBin_unwrapped(const std::string& account_name, const std::string& bin_name) const;

    mutable boost::mutex mutex;

private:
    std::shared_ptr<odb::core::database> db_;
};

}
