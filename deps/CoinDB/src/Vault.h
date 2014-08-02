///////////////////////////////////////////////////////////////////////////////
//
// Vault.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.
//

#pragma once

#include "Schema.h"
#include "VaultExceptions.h"
#include "SigningRequest.h"
#include "SignatureInfo.h"

#include <Signals/Signals.h>
#include <Signals/SignalQueue.h>

#include <CoinQ/CoinQ_blocks.h>

#include <CoinCore/BloomFilter.h>

#include <boost/thread.hpp>

// support for boost serialization
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

namespace CoinDB
{

typedef Signals::Signal<std::shared_ptr<Tx>> TxSignal;
typedef Signals::Signal<std::shared_ptr<MerkleBlock>> MerkleBlockSignal;

class Vault
{
public:
    Vault() : db_(nullptr) { }
    Vault(int argc, char** argv, bool create = false, uint32_t version = SCHEMA_VERSION);
    Vault(const std::string& dbname, bool create = false, uint32_t version = SCHEMA_VERSION);
    Vault(const std::string& dbuser, const std::string& dbpasswd, const std::string& dbname, bool create = false, uint32_t version = SCHEMA_VERSION);

    virtual ~Vault();

    ///////////////////////
    // GLOBAL OPERATIONS //
    ///////////////////////
    void                                    open(int argc, char** argv, bool create = false, uint32_t version = SCHEMA_VERSION);
    void                                    open(const std::string& dbuser, const std::string& dbpasswd, const std::string& dbname, bool create = false, uint32_t version = SCHEMA_VERSION);
    void                                    close();

    const std::string&                      getName() const { return name_; }
    uint32_t                                getSchemaVersion() const;
    void                                    setSchemaVersion(uint32_t version);

    static const uint32_t                   MAX_HORIZON_TIMESTAMP_OFFSET = 6 * 60 * 60; // a good six hours initial tolerance for incorrect clock
    uint32_t                                getHorizonTimestamp() const; // nothing that happened before this should matter to us.
    uint32_t                                getMaxFirstBlockTimestamp() const; // convenience method. getHorizonTimestamp() - MIN_HORIZON_TIMESTAMP_OFFSET
    uint32_t                                getHorizonHeight() const;
    std::vector<bytes_t>                    getLocatorHashes() const;
    Coin::BloomFilter                       getBloomFilter(double falsePositiveRate, uint32_t nTweak, uint32_t nFlags) const;

    void                                    exportVault(const std::string& filepath, bool exportprivkeys = true, const secure_bytes_t& exportChainCodeUnlockKey = secure_bytes_t()) const;

    void                                    importVault(const std::string& filepath, bool importprivkeys = true, const secure_bytes_t& importChainCodeUnlockKey = secure_bytes_t());

    ///////////////////////////
    // CHAIN CODE OPERATIONS //
    ///////////////////////////
    bool                                    areChainCodesLocked() const;
    void                                    lockChainCodes() const;
    void                                    unlockChainCodes(const secure_bytes_t& unlockKey) const;
    void                                    setChainCodeUnlockKey(const secure_bytes_t& newUnlockKey);

    /////////////////////////
    // KEYCHAIN OPERATIONS //
    /////////////////////////
    void                                    exportKeychain(const std::string& keychain_name, const std::string& filepath, bool exportprivkeys = false, const secure_bytes_t& exportChainCodeUnlockKey = secure_bytes_t()) const;
    std::shared_ptr<Keychain>               importKeychain(const std::string& filepath, bool& importprivkeys, const secure_bytes_t& importChainCodeUnlockKey = secure_bytes_t());
    bool                                    keychainExists(const std::string& keychain_name) const;
    bool                                    keychainExists(const bytes_t& keychain_hash) const;
    std::shared_ptr<Keychain>               newKeychain(const std::string& keychain_name, const secure_bytes_t& entropy, const secure_bytes_t& lockKey = secure_bytes_t(), const bytes_t& salt = bytes_t());
    //void eraseKeychain(const std::string& keychain_name) const;
    void                                    renameKeychain(const std::string& old_name, const std::string& new_name);
    std::shared_ptr<Keychain>               getKeychain(const std::string& keychain_name) const;
    std::vector<std::shared_ptr<Keychain>>  getAllKeychains(bool root_only = false, bool get_hidden = false) const;
    std::vector<KeychainView>               getRootKeychainViews(const std::string& account_name = "", bool get_hidden = false) const;
    secure_bytes_t                          getKeychainExtendedKey(const std::string& keychain_name, bool get_private) const;
    std::shared_ptr<Keychain>               importKeychainExtendedKey(const std::string& keychain_name, const secure_bytes_t& extkey, bool try_private, const secure_bytes_t& lockKey = secure_bytes_t(), const bytes_t& salt = bytes_t());

    // The following private key lock/unlock methods do not maintain a database session open so they only
    // store and erase the unlock keys in a member map to be used by the other class methods.
    void                                    lockAllKeychains();
    void                                    lockKeychain(const std::string& keychain_name);
    void                                    unlockKeychain(const std::string& keychain_name, const secure_bytes_t& unlock_key);
    bool                                    isKeychainPrivateKeyLocked(const std::string& keychainName) const;


    ////////////////////////
    // ACCOUNT OPERATIONS //
    ////////////////////////
    void                                    exportAccount(const std::string& account_name, const std::string& filepath, bool exportprivkeys = false, const secure_bytes_t& exportChainCodeUnlockKey = secure_bytes_t()) const;
    std::shared_ptr<Account>                importAccount(const std::string& filepath, unsigned int& privkeysimported, const secure_bytes_t& importChainCodeUnlockKey = secure_bytes_t()); // pass privkeysimported = 0 to not inport any private keys.
    bool                                    accountExists(const std::string& account_name) const;
    void                                    newAccount(const std::string& account_name, unsigned int minsigs, const std::vector<std::string>& keychain_names, uint32_t unused_pool_size = 25, uint32_t time_created = time(NULL));
    //void                                  eraseAccount(const std::string& name) const;
    void                                    renameAccount(const std::string& old_name, const std::string& new_name);
    std::shared_ptr<Account>                getAccount(const std::string& account_name) const;
    AccountInfo                             getAccountInfo(const std::string& account_name) const;
    std::vector<AccountInfo>                getAllAccountInfo() const;
    uint64_t                                getAccountBalance(const std::string& account_name, unsigned int min_confirmations = 1, int tx_flags = Tx::ALL) const;
    std::shared_ptr<AccountBin>             addAccountBin(const std::string& account_name, const std::string& bin_name);
    std::shared_ptr<SigningScript>          issueSigningScript(const std::string& account_name, const std::string& bin_name = DEFAULT_BIN_NAME, const std::string& label = "");
    void                                    refillAccountPool(const std::string& account_name);

    // empty account_name or bin_name means do not filter on those fields
    std::vector<SigningScriptView>          getSigningScriptViews(const std::string& account_name = "", const std::string& bin_name = "", int flags = SigningScript::ALL) const;
    std::vector<TxOutView>                  getTxOutViews(const std::string& account_name = "", const std::string& bin_name = "", int role_flags = TxOut::ROLE_BOTH, int txout_status_flags = TxOut::BOTH, int tx_status_flags = Tx::ALL, bool hide_change = true) const;
    std::vector<TxOutView>                  getUnspentTxOutViews(const std::string& account_name, uint32_t min_confirmations = 0) const;

    ////////////////////////////
    // ACCOUNT BIN OPERATIONS //
    ////////////////////////////
    std::shared_ptr<AccountBin>             getAccountBin(const std::string& account_name, const std::string& bin_name) const;
    std::vector<AccountBinView>             getAllAccountBinViews() const;
    void                                    exportAccountBin(const std::string& account_name, const std::string& bin_name, const std::string& export_name, const std::string& filepath, const secure_bytes_t& exportChainCodeUnlockKey = secure_bytes_t()) const;
    std::shared_ptr<AccountBin>             importAccountBin(const std::string& filepath, const secure_bytes_t& importChainCodeUnlockKey = secure_bytes_t()); 

    ///////////////////
    // TX OPERATIONS //
    ///////////////////
    std::shared_ptr<Tx>                     getTx(const bytes_t& hash) const; // Tries both signed and unsigned hashes. Throws TxNotFoundException.
    std::shared_ptr<Tx>                     getTx(unsigned long tx_id) const; // Uses the database id. Throws TxNotFoundException.
    std::vector<TxView>                     getTxViews(int tx_status_flags = Tx::ALL, unsigned long start = 0, int count = -1, uint32_t minheight = 0) const; // count = -1 means display all
    std::shared_ptr<Tx>                     insertTx(std::shared_ptr<Tx> tx); // Inserts transaction only if it affects one of our accounts. Returns transaction in vault if change occured. Otherwise returns nullptr.
    std::shared_ptr<Tx>                     createTx(const std::string& account_name, uint32_t tx_version, uint32_t tx_locktime, txouts_t txouts, uint64_t fee, unsigned int maxchangeouts = 1, bool insert = false);
    std::shared_ptr<Tx>                     createTx(const std::string& account_name, uint32_t tx_version, uint32_t tx_locktime, ids_t coin_ids, txouts_t txouts, uint64_t fee, uint32_t min_confirmations, bool insert = false); // Pass empty output scripts to generate change outputs.
    void                                    deleteTx(const bytes_t& tx_hash); // Tries both signed and unsigned hashes. Throws TxNotFoundException.
    void                                    deleteTx(unsigned long tx_id); // Throws TxNotFoundException.
    SigningRequest                          getSigningRequest(const bytes_t& hash, bool include_raw_tx = false) const; // Tries both signed and unsigned hashes. Throws TxNotFoundException.
    SigningRequest                          getSigningRequest(unsigned long tx_id, bool include_raw_tx = false) const; // Throws TxNotFoundException.
    SignatureInfo                           getSignatureInfo(const bytes_t& hash) const; // Tries both signed and unsigned hashes. Throws TxNotFoundException.
    SignatureInfo                           getSignatureInfo(unsigned long tx_id) const; // Throws TxNotFoundException.
    // signTx tries only unsigned hashes for named keychains. If no keychains are named, tries all keychains. For signed hashes, signTx just returns the already signed transaction. Throws TxNotFoundException.
    std::shared_ptr<Tx>                     signTx(const bytes_t& hash, std::vector<std::string>& keychain_names, bool update = false);
    std::shared_ptr<Tx>                     signTx(unsigned long tx_id, std::vector<std::string>& keychain_names, bool update = false);

    std::shared_ptr<Tx>                     exportTx(const bytes_t& hash, const std::string& filepath) const;
    std::shared_ptr<Tx>                     exportTx(unsigned long tx_id, const std::string& filepath) const;
    void                                    exportTx(std::shared_ptr<Tx> tx, const std::string& filepath) const;
    std::shared_ptr<Tx>                     importTx(const std::string& filepath);
    void                                    exportTxs(const std::string& filepath, uint32_t minheight = 0) const;
    void                                    importTxs(const std::string& filepath);

    //////////////////////
    // BLOCK OPERATIONS //
    //////////////////////
    uint32_t                                getBestHeight() const;
    std::shared_ptr<BlockHeader>            getBlockHeader(const bytes_t& hash) const;
    std::shared_ptr<BlockHeader>            getBlockHeader(uint32_t height) const;
    std::shared_ptr<BlockHeader>            getBestBlockHeader() const;
    std::shared_ptr<MerkleBlock>            insertMerkleBlock(std::shared_ptr<MerkleBlock> merkleblock);
    unsigned int                            deleteMerkleBlock(const bytes_t& hash);
    unsigned int                            deleteMerkleBlock(uint32_t height);
    void                                    exportMerkleBlocks(const std::string& filepath) const;
    void                                    importMerkleBlocks(const std::string& filepath);

    ////////////////////////
    // SLOT SUBSCRIPTIONS //
    ////////////////////////
    Signals::Connection subscribeTxInserted(TxSignal::Slot slot) { return notifyTxInserted.connect(slot); }
    Signals::Connection subscribeTxStatusChanged(TxSignal::Slot slot) { return notifyTxStatusChanged.connect(slot); }
    Signals::Connection subscribeMerkleBlockInserted(MerkleBlockSignal::Slot slot) { return notifyMerkleBlockInserted.connect(slot); }
    void clearAllSlots()
    {
        notifyTxInserted.clear();
        notifyTxStatusChanged.clear();
        notifyMerkleBlockInserted.clear();
    }

protected:
    ///////////////////////
    // GLOBAL OPERATIONS //
    ///////////////////////
    uint32_t                                getSchemaVersion_unwrapped() const;
    void                                    setSchemaVersion_unwrapped(uint32_t version);

    uint32_t                                getHorizonTimestamp_unwrapped() const;
    uint32_t                                getMaxFirstBlockTimestamp_unwrapped() const;
    uint32_t                                getHorizonHeight_unwrapped() const;
    std::vector<bytes_t>                    getLocatorHashes_unwrapped() const;
    Coin::BloomFilter                       getBloomFilter_unwrapped(double falsePositiveRate, uint32_t nTweak, uint32_t nFlags) const;

    ///////////////////////////
    // CHAIN CODE OPERATIONS //
    ///////////////////////////
    void                                    verifyChainCodeUnlockKey_unwrapped(const secure_bytes_t& unlockKey) const;
    void                                    setChainCodeUnlockKey_unwrapped(const secure_bytes_t& newUnlockKey);

    /////////////////////////
    // KEYCHAIN OPERATIONS //
    /////////////////////////
    bool                                    keychainExists_unwrapped(const std::string& keychain_name) const;
    bool                                    keychainExists_unwrapped(const bytes_t& keychain_hash) const;
    std::shared_ptr<Keychain>               getKeychain_unwrapped(const std::string& keychain_name) const;
    void                                    persistKeychain_unwrapped(std::shared_ptr<Keychain> keychain);
    void                                    exportKeychain_unwrapped(std::shared_ptr<Keychain> keychain, const std::string& filepath, const secure_bytes_t& exportChainCodeUnlockKey = secure_bytes_t()) const;
    std::shared_ptr<Keychain>               importKeychain_unwrapped(const std::string& filepath, bool& importprivkeys, const secure_bytes_t& importChainCodeUnlockKey = secure_bytes_t());
    secure_bytes_t                          getKeychainExtendedKey_unwrapped(std::shared_ptr<Keychain> keychain, bool get_private) const;
    std::vector<KeychainView>               getRootKeychainViews_unwrapped(const std::string& account_name = "", bool get_hidden = false) const;

    // All the keychain unlock methods use the global keys by default

    // The following methods throw KeychainChainCodeUnlockFailedException
    void                                    unlockKeychainChainCode_unwrapped(std::shared_ptr<Keychain> keychain, const secure_bytes_t& overrideChainCodeUnlockKey = secure_bytes_t()) const;
    void                                    unlockKeychainPrivateKey_unwrapped(std::shared_ptr<Keychain> keychain, const secure_bytes_t& overridePrivateKeyUnlockKey = secure_bytes_t()) const;

    // The following methods return true iff successful
    bool                                    tryUnlockKeychainChainCode_unwrapped(std::shared_ptr<Keychain> keychain, const secure_bytes_t& overrideChainCodeUnlockKey = secure_bytes_t()) const;
    bool                                    tryUnlockKeychainPrivateKey_unwrapped(std::shared_ptr<Keychain> keychain, const secure_bytes_t& overridePrivateKeyUnlockKey = secure_bytes_t()) const;

    ////////////////////////
    // Account operations //
    ////////////////////////
    // The following methods try to use the vault's global chainCodeUnlockKey by default.
    void                                    exportAccount_unwrapped(Account& account, boost::archive::text_oarchive& oa, bool exportprivkeys, const secure_bytes_t& exportChainCodeUnlockKey = secure_bytes_t()) const;
    std::shared_ptr<Account>                importAccount_unwrapped(boost::archive::text_iarchive& ia, unsigned int& privkeysimported, const secure_bytes_t& importChainCodeUnlockKey = secure_bytes_t());

    // The following method throws KeychainChainCodeUnlockFailedException
    void                                    unlockAccountChainCodes_unwrapped(std::shared_ptr<Account> account, const secure_bytes_t& overrideChainCodeUnlockKey = secure_bytes_t()) const;

    // The following method returns true iff successful for all keychains
    bool                                    tryUnlockAccountChainCodes_unwrapped(std::shared_ptr<Account> account, const secure_bytes_t& overrideChainCodeUnlockKey = secure_bytes_t()) const;

    // The following method throws KeychainChainCodeLockedException
    void                                    refillAccountPool_unwrapped(std::shared_ptr<Account> account);

    bool                                    accountExists_unwrapped(const std::string& account_name) const;
    std::shared_ptr<Account>                getAccount_unwrapped(const std::string& account_name) const; // throws AccountNotFoundException

    std::vector<TxOutView>                  getUnspentTxOutViews_unwrapped(std::shared_ptr<Account> account, uint32_t min_confirmations = 0) const;

    ////////////////////////////
    // ACCOUNT BIN OPERATIONS //
    ////////////////////////////
    std::shared_ptr<AccountBin>             getAccountBin_unwrapped(const std::string& account_name, const std::string& bin_name) const;
    std::shared_ptr<SigningScript>          issueAccountBinSigningScript_unwrapped(std::shared_ptr<AccountBin> account_bin, const std::string& label = "");
    void                                    unlockAccountBinChainCodes_unwrapped(std::shared_ptr<AccountBin> bin, const secure_bytes_t& overrideChainCodeUnlockKey = secure_bytes_t()) const;
    void                                    refillAccountBinPool_unwrapped(std::shared_ptr<AccountBin> bin);
    void                                    exportAccountBin_unwrapped(const std::shared_ptr<AccountBin> account_bin, const std::string& export_name, const std::string& filepath, const secure_bytes_t& exportChainCodeUnlockKey = secure_bytes_t()) const;
    std::shared_ptr<AccountBin>             importAccountBin_unwrapped(const std::string& filepath, const secure_bytes_t& importChainCodeUnlockKey = secure_bytes_t()); 

    ///////////////////
    // TX OPERATIONS //
    ///////////////////
    std::shared_ptr<Tx>                     getTx_unwrapped(const bytes_t& hash) const; // Tries both signed and unsigned hashes. Throws TxNotFoundException.
    std::shared_ptr<Tx>                     getTx_unwrapped(unsigned long tx_id) const; // Uses database id. Throws TxNotFoundException.
    std::shared_ptr<Tx>                     insertTx_unwrapped(std::shared_ptr<Tx> tx);
    std::shared_ptr<Tx>                     createTx_unwrapped(const std::string& account_name, uint32_t tx_version, uint32_t tx_locktime, txouts_t txouts, uint64_t fee, unsigned int maxchangeouts = 1);
    std::shared_ptr<Tx>                     createTx_unwrapped(const std::string& account_name, uint32_t tx_version, uint32_t tx_locktime, ids_t coin_ids, txouts_t txouts, uint64_t fee, uint32_t min_confirmations);
    void                                    deleteTx_unwrapped(std::shared_ptr<Tx> tx);
    void                                    updateTx_unwrapped(std::shared_ptr<Tx> tx);
    SigningRequest                          getSigningRequest_unwrapped(std::shared_ptr<Tx> tx, bool include_raw_tx = false) const;
    SignatureInfo                           getSignatureInfo_unwrapped(std::shared_ptr<Tx> tx) const;
    unsigned int                            signTx_unwrapped(std::shared_ptr<Tx> tx, std::vector<std::string>& keychain_names); // Tries to sign as many as it can with the unlocked keychains.

    void                                    exportTxs_unwrapped(boost::archive::text_oarchive& oa, uint32_t minheight) const;
    void                                    importTxs_unwrapped(boost::archive::text_iarchive& ia);

    ///////////////////////////
    // BLOCKCHAIN OPERATIONS //
    ///////////////////////////
    uint32_t                                getBestHeight_unwrapped() const;
    std::shared_ptr<BlockHeader>            getBlockHeader_unwrapped(const bytes_t& hash) const;
    std::shared_ptr<BlockHeader>            getBlockHeader_unwrapped(uint32_t height) const;
    std::shared_ptr<BlockHeader>            getBestBlockHeader_unwrapped() const;
    std::shared_ptr<MerkleBlock>            insertMerkleBlock_unwrapped(std::shared_ptr<MerkleBlock> merkleblock);
    unsigned int                            deleteMerkleBlock_unwrapped(std::shared_ptr<MerkleBlock> merkleblock);
    unsigned int                            deleteMerkleBlock_unwrapped(uint32_t height);
    unsigned int                            updateConfirmations_unwrapped(std::shared_ptr<Tx> tx = nullptr); // If parameter is null, updates all unconfirmed transactions.
                                                                                                     // Returns the number of transaction previously unconfirmed that are now confirmed.

    void                                    exportMerkleBlocks_unwrapped(boost::archive::text_oarchive& oa) const;
    void                                    importMerkleBlocks_unwrapped(boost::archive::text_iarchive& ia);

    /////////////
    // SIGNALS //
    /////////////
    Signals::SignalQueue                    signalQueue;

    TxSignal                                notifyTxInserted;
    TxSignal                                notifyTxStatusChanged;
    MerkleBlockSignal                       notifyMerkleBlockInserted;

private:
    mutable boost::mutex mutex;
    std::shared_ptr<odb::core::database> db_;
    std::string name_;

    mutable secure_bytes_t chainCodeUnlockKey;
    mutable std::map<std::string, secure_bytes_t> mapPrivateKeyUnlock;
};

}
