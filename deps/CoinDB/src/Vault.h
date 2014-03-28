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
#include "SigningRequest.h"

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

    ///////////////////////
    // GLOBAL OPERATIONS //
    ///////////////////////
     uint32_t MIN_HORIZON_TIMESTAMP_OFFSET = 6 * 60 * 60; // a good six hours initial reorganization tolerance period.
    void updateHorizonStatus();
    uint32_t getHorizonTimestamp() const; // nothing that happened before this should matter to us.
    uint32_t getHorizonHeight() const;
    std::vector<bytes_t> getLocatorHashes() const;
    Coin::BloomFilter getBloomFilter(double falsePositiveRate, uint32_t nTweak, uint32_t nFlags) const;

    /////////////////////
    // FILE OPERATIONS //
    /////////////////////
    void exportKeychain(const std::string& keychain_name, const std::string& filepath, bool exportprivkeys = false) const;
    std::shared_ptr<Keychain> importKeychain(const std::string& filepath, bool& importprivkeys);
    void exportAccount(const std::string& account_name, const std::string& filepath, const secure_bytes_t& chain_code_key = secure_bytes_t(), const bytes_t& salt = bytes_t(), bool exportprivkeys = false) const;
    std::shared_ptr<Account> importAccount(const std::string& filepath, const secure_bytes_t& chain_code_key, unsigned int& privkeysimported); // pass privkeysimported = 0 to not inport any private keys.

    /////////////////////////
    // KEYCHAIN OPERATIONS //
    /////////////////////////
    bool keychainExists(const std::string& keychain_name) const;
    bool keychainExists(const bytes_t& keychain_hash) const;
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
    std::shared_ptr<SigningScript> issueSigningScript(const std::string& account_name, const std::string& bin_name = DEFAULT_BIN_NAME, const std::string& label = "");

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
    std::shared_ptr<Tx> getTx(const bytes_t& hash) const; // Tries both signed and unsigned hashes. Throws TxNotFoundException/
    std::shared_ptr<Tx> insertTx(std::shared_ptr<Tx> tx); // Inserts transaction only if it affects one of our accounts. Returns transaction in vault if change occured. Otherwise returns nullptr.
    std::shared_ptr<Tx> createTx(const std::string& account_name, uint32_t tx_version, uint32_t tx_locktime, txouts_t txouts, uint64_t fee, unsigned int maxchangeouts = 1, bool insert = false);
    void deleteTx(const bytes_t& tx_hash); // Tries both signed and unsigned hashes. Throws TxNotFoundException.
    SigningRequest getSigningRequest(const bytes_t& unsigned_hash, bool include_raw_tx = false) const; // Tries only unsigned hashes. Throws TxNotFoundException.
    bool signTx(const bytes_t& unsigned_hash, bool update = false); // Tries only unsigned hashes. Throws TxNotFoundException.

    //////////////////////
    // BLOCK OPERATIONS //
    //////////////////////
    uint32_t getBestHeight() const;
    std::shared_ptr<BlockHeader> getBlockHeader(const bytes_t& hash) const;
    std::shared_ptr<BlockHeader> getBlockHeader(uint32_t height) const;
    std::shared_ptr<BlockHeader> getBestBlockHeader() const;
    std::shared_ptr<MerkleBlock> insertMerkleBlock(std::shared_ptr<MerkleBlock> merkleblock);
    unsigned int deleteMerkleBlock(const bytes_t& hash);
    unsigned int deleteMerkleBlock(uint32_t height);

protected:
    // Global operations
    uint32_t                        getHorizonTimestamp_unwrapped() const;
    uint32_t                        getHorizonHeight_unwrapped() const;
    std::vector<bytes_t>            getLocatorHashes_unwrapped() const;
    Coin::BloomFilter               getBloomFilter_unwrapped(double falsePositiveRate, uint32_t nTweak, uint32_t nFlags) const;

    // File operations
    void                            exportKeychain_unwrapped(std::shared_ptr<Keychain> keychain, const std::string& filepath) const;
    std::shared_ptr<Keychain>       importKeychain_unwrapped(const std::string& filepath, bool& importprivkeys);
    void                            exportAccount_unwrapped(const std::shared_ptr<Account> account, const std::string& filepath) const;
    std::shared_ptr<Account>        importAccount_unwrapped(const std::string& filepath, const secure_bytes_t& chain_code_key, unsigned int& privkeysimported);

    // Keychain operations
    bool                            keychainExists_unwrapped(const std::string& keychain_name) const;
    bool                            keychainExists_unwrapped(const bytes_t& keychain_hash) const;
    std::shared_ptr<Keychain>       getKeychain_unwrapped(const std::string& keychain_name) const;
    void                            persistKeychain_unwrapped(std::shared_ptr<Keychain> keychain);
    bool                            tryUnlockKeychainChainCode_unwrapped(std::shared_ptr<Keychain> keychain) const;
    bool                            tryUnlockKeychainPrivateKey_unwrapped(std::shared_ptr<Keychain> keychain) const;

    // Account operations
    bool                            accountExists_unwrapped(const std::string& account_name) const;
    std::shared_ptr<Account>        getAccount_unwrapped(const std::string& account_name) const;
    void                            tryUnlockAccountChainCodes_unwrapped(std::shared_ptr<Account> account) const;
    void                            refillAccountPool_unwrapped(std::shared_ptr<Account> account);
    void                            trySetAccountChainCodesLockKey_unwrapped(std::shared_ptr<Account> account, const secure_bytes_t& new_lock_key, const bytes_t& salt) const;

    // AccountBin operations
    std::shared_ptr<AccountBin>     getAccountBin_unwrapped(const std::string& account_name, const std::string& bin_name) const;
    std::shared_ptr<SigningScript>  issueAccountBinSigningScript_unwrapped(std::shared_ptr<AccountBin> account_bin, const std::string& label = "");
    void                            refillAccountBinPool_unwrapped(std::shared_ptr<AccountBin> bin);

    // Tx operations
    std::shared_ptr<Tx>             getTx_unwrapped(const bytes_t& hash) const; // Tries both signed and unsigned hashes. Throws TxNotFoundException/
    std::shared_ptr<Tx>             insertTx_unwrapped(std::shared_ptr<Tx> tx);
    std::shared_ptr<Tx>             createTx_unwrapped(const std::string& account_name, uint32_t tx_version, uint32_t tx_locktime, txouts_t txouts, uint64_t fee, unsigned int maxchangeouts = 1);
    void                            deleteTx_unwrapped(std::shared_ptr<Tx> tx);
    void                            updateTx_unwrapped(std::shared_ptr<Tx> tx);
    SigningRequest                  getSigningRequest_unwrapped(std::shared_ptr<Tx> tx, bool include_raw_tx = false) const;
    bool                            signTx_unwrapped(std::shared_ptr<Tx> tx); // Tries to sign as many as it can with the unlocked keychains.

    // Block operations
    uint32_t                        getBestHeight_unwrapped() const;
    std::shared_ptr<BlockHeader>    getBlockHeader_unwrapped(const bytes_t& hash) const;
    std::shared_ptr<BlockHeader>    getBlockHeader_unwrapped(uint32_t height) const;
    std::shared_ptr<BlockHeader>    getBestBlockHeader_unwrapped() const;
    std::shared_ptr<MerkleBlock>    insertMerkleBlock_unwrapped(std::shared_ptr<MerkleBlock> merkleblock);
    unsigned int                    deleteMerkleBlock_unwrapped(std::shared_ptr<MerkleBlock> merkleblock);
    unsigned int                    updateConfirmations_unwrapped(std::shared_ptr<Tx> tx = nullptr); // If parameter is null, updates all unconfirmed transactions.
                                                                                                     // Returns the number of transaction previously unconfirmed that are now confirmed.

    // Used for initial synching
    bool haveHorizonBlock;
    uint32_t minHorizonTimestamp;

private:
    mutable boost::mutex mutex;
    std::shared_ptr<odb::core::database> db_;

    mutable std::map<std::string, secure_bytes_t> mapChainCodeUnlock;
    mutable std::map<std::string, secure_bytes_t> mapPrivateKeyUnlock;
};

}
