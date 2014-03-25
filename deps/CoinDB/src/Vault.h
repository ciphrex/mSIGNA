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
    std::shared_ptr<Keychain> newKeychain(const std::string& keychain_name, const secure_bytes_t& entropy, const secure_bytes_t& lockKey = secure_bytes_t(), const bytes_t& salt = bytes_t());
    //void eraseKeychain(const std::string& keychain_name) const;
    void renameKeychain(const std::string& old_name, const std::string& new_name);
    std::shared_ptr<Keychain> getKeychain(const std::string& keychain_name) const;
    std::vector<std::shared_ptr<Keychain>> getAllKeychains(bool root_only = false) const;

    // The following chain code and private key lock/unlock methods do not maintain a database session open so they only
    // store and erase the unlock keys in member maps to be used by the other class methods.
    void lockAllKeychainChainCodes();
    void lockKeychainChainCode(const std::string& keychain_name);
    void unlockKeychainChainCode(const std::string& keychain_name, const secure_bytes_t& unlock_key);

    void lockAllKeychainPrivateKeys();
    void lockKeychainPrivateKey(const std::string& keychain_name);
    void unlockKeychainPrivateKey(const std::string& keychain_name, const secure_bytes_t& unlock_key);

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

    uint64_t getAccountBalance(const std::string& account_name, unsigned int min_confirmations = 1, int tx_flags = Tx::ALL) const;

    std::shared_ptr<AccountBin> addAccountBin(const std::string& account_name, const std::string& bin_name);
    std::shared_ptr<SigningScript> newSigningScript(const std::string& account_name, const std::string& bin_name = DEFAULT_BIN_NAME, const std::string& label = "");

    void refillAccountPool(const std::string& account_name);

    // empty account_name or bin_name means do not filter on those fields
    std::vector<SigningScriptView> getSigningScriptViews(const std::string& account_name = "@all", const std::string& bin_name = "@all", int flags = SigningScript::ALL) const;
    std::vector<TxOutView> getTxOutViews(const std::string& account_name = "@all", const std::string& bin_name = "@all", int txout_status_flags = TxOut::BOTH, int tx_status_flags = Tx::ALL) const;

    ////////////////////////////
    // ACCOUNT BIN OPERATIONS //
    ////////////////////////////
    std::shared_ptr<AccountBin> getAccountBin(const std::string& account_name, const std::string& bin_name) const;

    ///////////////////
    // TX OPERATIONS //
    ///////////////////
    std::shared_ptr<Tx> insertTx(std::shared_ptr<Tx> tx); // Inserts transaction only if it affects one of our accounts. Returns transaction in vault if change occured. Otherwise returns nullptr.
    std::shared_ptr<Tx> createTx(const std::string& account_name, uint32_t tx_version, uint32_t tx_locktime, txouts_t txouts, uint64_t fee, unsigned int maxchangeouts = 1, bool insert = false);

protected:
    // Keychain operations
    std::shared_ptr<Keychain> getKeychain_unwrapped(const std::string& keychain_name) const;
    void persistKeychain_unwrapped(std::shared_ptr<Keychain> keychain);

    // Account operations
    std::shared_ptr<Account> getAccount_unwrapped(const std::string& account_name) const;
    void tryUnlockAccountChainCodes_unwrapped(std::shared_ptr<Account> account);
    void refillAccountPool_unwrapped(std::shared_ptr<Account> account);

    // AccountBin operations
    std::shared_ptr<AccountBin> getAccountBin_unwrapped(const std::string& account_name, const std::string& bin_name) const;
    std::shared_ptr<SigningScript> newAccountBinSigningScript_unwrapped(std::shared_ptr<AccountBin> account_bin, const std::string& label = "");
    void refillAccountBinPool_unwrapped(std::shared_ptr<AccountBin> bin);

    // Tx operations
    std::shared_ptr<Tx> insertTx_unwrapped(std::shared_ptr<Tx> tx);
    std::shared_ptr<Tx> createTx_unwrapped(const std::string& account_name, uint32_t tx_version, uint32_t tx_locktime, txouts_t txouts, uint64_t fee, unsigned int maxchangeouts = 1);

private:
    mutable boost::mutex mutex;
    std::shared_ptr<odb::core::database> db_;

    std::map<std::string, secure_bytes_t> mapChainCodeUnlock;
    std::map<std::string, secure_bytes_t> mapPrivateKeyUnlock;
};

}
