///////////////////////////////////////////////////////////////////////////////
//
// Vault.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
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

typedef Signals::Signal<const std::string& /*keychain_name*/> KeychainUnlockedSignal;
typedef Signals::Signal<const std::string& /*keychain_name*/> KeychainLockedSignal;

typedef Signals::Signal<std::shared_ptr<Tx>> TxSignal;
typedef Signals::Signal<std::shared_ptr<MerkleBlock>> MerkleBlockSignal;

typedef Signals::Signal<std::shared_ptr<Tx>, const std::string& /*description*/> TxErrorSignal;
typedef Signals::Signal<std::shared_ptr<MerkleBlock>, const std::string& /*description*/> MerkleBlockErrorSignal;

typedef Signals::Signal<std::shared_ptr<MerkleBlock>, bytes_t> TxConfirmationErrorSignal;

class Vault
{
public:
    Vault() : db_(nullptr) { }
    Vault(int argc, char** argv, bool create = false, uint32_t version = SCHEMA_VERSION, const std::string& network = "", bool migrate = false);
    Vault(const std::string& dbname, bool create = false, uint32_t version = SCHEMA_VERSION, const std::string& network = "", bool migrate = false);
    Vault(const std::string& dbuser, const std::string& dbpasswd, const std::string& dbname, bool create = false, uint32_t version = SCHEMA_VERSION, const std::string& network = "", bool migrate = false);

    virtual ~Vault();

    ////////////////////
    // STATIC METHODS //
    ////////////////////
    static bool                             isValidObjectName(const std::string& name);

    typedef std::pair<std::string, unsigned int> split_name_t;
    static split_name_t                     getSplitObjectName(const std::string& name);

    ///////////////////////
    // GLOBAL OPERATIONS //
    ///////////////////////
    void                                    open(int argc, char** argv, bool create = false, uint32_t version = SCHEMA_VERSION, const std::string& network = "", bool migrate = false);
    void                                    open(const std::string& dbuser, const std::string& dbpasswd, const std::string& dbname, bool create = false, uint32_t version = SCHEMA_VERSION, const std::string& network = "", bool migrate = false);
    void                                    close();

    const std::string&                      getName() const { return name_; }
    uint32_t                                getSchemaVersion() const;
    void                                    setSchemaVersion(uint32_t version);
    std::string                             getNetwork() const;
    void                                    setNetwork(const std::string& network);

    static const uint32_t                   MAX_HORIZON_TIMESTAMP_OFFSET = 6 * 60 * 60; // a good six hours initial tolerance for incorrect clock
    uint32_t                                getHorizonTimestamp() const; // nothing that happened before this should matter to us.
    uint32_t                                getMaxFirstBlockTimestamp() const; // convenience method. getHorizonTimestamp() - MIN_HORIZON_TIMESTAMP_OFFSET
    uint32_t                                getHorizonHeight() const;
    std::vector<bytes_t>                    getLocatorHashes() const;
    Coin::BloomFilter                       getBloomFilter(double falsePositiveRate, uint32_t nTweak, uint32_t nFlags) const;
    hashvector_t                            getIncompleteBlockHashes() const;

    void                                    exportVault(const std::string& filepath, bool exportprivkeys = true) const;

    void                                    importVault(const std::string& filepath, bool importprivkeys = true);

    ////////////////////////
    // CONTACT OPERATIONS //
    ////////////////////////
    std::shared_ptr<Contact>                newContact(const std::string& username);
    std::shared_ptr<Contact>                getContact(const std::string& username) const;
    ContactVector                           getAllContacts() const;
    bool                                    contactExists(const std::string& username) const;
    std::shared_ptr<Contact>                renameContact(const std::string& old_username, const std::string& new_username);

    /////////////////////////
    // KEYCHAIN OPERATIONS //
    /////////////////////////
    void                                    exportKeychain(const std::string& keychain_name, const std::string& filepath, bool exportprivkeys = false) const;
    std::shared_ptr<Keychain>               importKeychain(const std::string& filepath, bool& importprivkeys);
    bool                                    keychainExists(const std::string& keychain_name) const;
    bool                                    keychainExists(const bytes_t& keychain_hash) const;
    bool                                    isKeychainPrivate(const std::string& keychain_name) const;
    bool                                    isKeychainEncrypted(const std::string& keychain_name) const;
    std::shared_ptr<Keychain>               newKeychain(const std::string& keychain_name, const secure_bytes_t& entropy, const secure_bytes_t& lockKey = secure_bytes_t());
    //void eraseKeychain(const std::string& keychain_name) const;
    void                                    renameKeychain(const std::string& old_name, const std::string& new_name);
    std::shared_ptr<Keychain>               getKeychain(const std::string& keychain_name) const;
    std::vector<std::shared_ptr<Keychain>>  getAllKeychains(bool root_only = false, bool get_hidden = false) const;
    std::vector<KeychainView>               getRootKeychainViews(const std::string& account_name = "", bool get_hidden = false) const;
    secure_bytes_t                          exportBIP32(const std::string& keychain_name, bool export_private) const;
    std::shared_ptr<Keychain>               importBIP32(const std::string& keychain_name, const secure_bytes_t& extkey, const secure_bytes_t& lock_key = secure_bytes_t());
    secure_bytes_t                          exportBIP39(const std::string& keychain_name) const;

    // The following methods change the persisted encryption state using the in-memory unlock key map.
    void                                    encryptKeychain(const std::string& keychain_name, const secure_bytes_t& lock_key);
    void                                    decryptKeychain(const std::string& keychain_name);

    // The following private key lock/unlock methods do not maintain a database session open so they only
    // store and erase the unlock keys in a member map to be used by the other class methods.
    void                                    lockAllKeychains();
    void                                    lockKeychain(const std::string& keychain_name);
    void                                    unlockKeychain(const std::string& keychain_name, const secure_bytes_t& unlock_key = secure_bytes_t());
    bool                                    isKeychainLocked(const std::string& keychainName) const;


    ////////////////////////
    // ACCOUNT OPERATIONS //
    ////////////////////////
    void                                    exportAccount(const std::string& account_name, const std::string& filepath, bool exportprivkeys = false) const;
    std::shared_ptr<Account>                importAccount(const std::string& filepath, unsigned int& privkeysimported); // pass privkeysimported = 0 to not inport any private keys.
    bool                                    accountExists(const std::string& account_name) const;
    void                                    newAccount(const std::string& account_name, unsigned int minsigs, const std::vector<std::string>& keychain_names, uint32_t unused_pool_size = DEFAULT_UNUSED_POOL_SIZE, uint32_t time_created = time(NULL), bool compressed_keys = true, bool use_witness = false, bool use_witness_p2sh = false);
    void                                    newAccount(bool use_witness, bool use_witness_p2sh, const std::string& account_name, unsigned int minsigs, const std::vector<std::string>& keychain_names, uint32_t unused_pool_size = DEFAULT_UNUSED_POOL_SIZE, uint32_t time_created = time(NULL), bool compressed_keys = true);
    //void                                  eraseAccount(const std::string& name) const;
    void                                    renameAccount(const std::string& old_name, const std::string& new_name);
    std::shared_ptr<Account>                getAccount(const std::string& account_name) const;
    AccountInfo                             getAccountInfo(const std::string& account_name) const;
    std::vector<AccountInfo>                getAllAccountInfo() const;
    uint64_t                                getAccountBalance(const std::string& account_name, unsigned int min_confirmations = 1, int tx_flags = Tx::ALL) const;
    std::shared_ptr<AccountBin>             addAccountBin(const std::string& account_name, const std::string& bin_name);
    std::shared_ptr<SigningScript>          issueSigningScript(const std::string& account_name, const std::string& bin_name = DEFAULT_BIN_NAME, const std::string& label = "", uint32_t index = 0, const std::string& username = std::string());
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
    void                                    exportAccountBin(const std::string& account_name, const std::string& bin_name, const std::string& export_name, const std::string& filepath) const;
    std::shared_ptr<AccountBin>             importAccountBin(const std::string& filepath); 

    ///////////////////
    // TX OPERATIONS //
    ///////////////////
    std::shared_ptr<Tx>                     getTx(const bytes_t& hash) const; // Tries both signed and unsigned hashes. Throws TxNotFoundException.
    std::shared_ptr<Tx>                     getTx(unsigned long tx_id) const; // Uses the database id. Throws TxNotFoundException.
    txs_t                                   getTxs(int tx_status_flags = Tx::ALL, unsigned long start = 0, int count = -1, uint32_t minheight = 0) const;
    uint32_t                                getTxConfirmations(const bytes_t& hash) const;
    uint32_t                                getTxConfirmations(unsigned long tx_id) const;
    uint32_t                                getTxConfirmations(std::shared_ptr<Tx> tx) const;
    std::vector<TxView>                     getTxViews(int tx_status_flags = Tx::ALL, unsigned long start = 0, int count = -1, uint32_t minheight = 0) const; // count = -1 means display all
    std::vector<std::string>                getSerializedUnsignedTxs(const std::string& account_name) const;
    std::shared_ptr<Tx>                     insertTx(std::shared_ptr<Tx> tx, bool replace_labels = false); // Inserts transaction only if it affects one of our accounts. Returns transaction in vault if change occured. Otherwise returns nullptr.
    std::shared_ptr<Tx>                     insertNewTx(const Coin::Transaction& cointx, std::shared_ptr<BlockHeader> blockheader = nullptr, bool verifysigs = false, bool isCoinbase = false);
    std::shared_ptr<Tx>                     insertMerkleTx(const ChainMerkleBlock& chainmerkleblock, const Coin::Transaction& cointx, unsigned int txindex, unsigned int txcount, bool verifysigs = false, bool isCoinbase = false);
    std::shared_ptr<Tx>                     confirmMerkleTx(const ChainMerkleBlock& chainmerkleblock, const bytes_t& txhash, unsigned int txindex, unsigned int txcount);
    std::shared_ptr<Tx>                     createTx(const std::string& account_name, uint32_t tx_version, uint32_t tx_locktime, txouts_t txouts, uint64_t fee, unsigned int maxchangeouts = 1, bool insert = false);
    std::shared_ptr<Tx>                     createTx(const std::string& username, const std::string& account_name, uint32_t tx_version, uint32_t tx_locktime, txouts_t txouts, uint64_t fee, unsigned int maxchangeouts = 1, bool insert = false);
    std::shared_ptr<Tx>                     createTx(const std::string& account_name, uint32_t tx_version, uint32_t tx_locktime, ids_t coin_ids, txouts_t txouts, uint64_t fee, uint32_t min_confirmations, bool insert = false); // Pass empty output scripts to generate change outputs.
    std::shared_ptr<Tx>                     createTx(const std::string& username, const std::string& account_name, uint32_t tx_version, uint32_t tx_locktime, ids_t coin_ids, txouts_t txouts, uint64_t fee, uint32_t min_confirmations, bool insert = false); // Pass empty output scripts to generate change outputs.
    txs_t                                   consolidateTxOuts(const std::string& account_name, uint32_t max_tx_size /* in bytes */, uint32_t tx_version, uint32_t tx_locktime, ids_t coin_ids, const bytes_t& txoutscript, uint64_t min_fee, uint32_t min_confirmations, bool insert = false);
    txs_t                                   consolidateTxOuts(const std::string& username, const std::string& account_name, uint32_t max_tx_size /* in bytes */, uint32_t tx_version, uint32_t tx_locktime, ids_t coin_ids, const bytes_t& txoutscript, uint64_t min_fee, uint32_t min_confirmations, bool insert = false);
    void                                    deleteTx(const bytes_t& tx_hash); // Tries both signed and unsigned hashes. Throws TxNotFoundException.
    void                                    deleteTx(unsigned long tx_id); // Throws TxNotFoundException.
    SigningRequest                          getSigningRequest(const bytes_t& hash, bool include_raw_tx = false) const; // Tries both signed and unsigned hashes. Throws TxNotFoundException.
    SigningRequest                          getSigningRequest(unsigned long tx_id, bool include_raw_tx = false) const; // Throws TxNotFoundException.
    SignatureInfo                           getSignatureInfo(const bytes_t& hash) const; // Tries both signed and unsigned hashes. Throws TxNotFoundException.
    SignatureInfo                           getSignatureInfo(unsigned long tx_id) const; // Throws TxNotFoundException.
    // signTx tries only unsigned hashes for named keychains. If no keychains are named, tries all keychains. For signed hashes, signTx just returns the already signed transaction. Throws TxNotFoundException.
    std::shared_ptr<Tx>                     signTx(const bytes_t& hash, std::vector<std::string>& keychain_names, bool update = false);
    std::shared_ptr<Tx>                     signTx(unsigned long tx_id, std::vector<std::string>& keychain_names, bool update = false);

    std::shared_ptr<TxOut>                  getTxOut(const bytes_t& outhash, uint32_t outindex) const;
    std::shared_ptr<TxOut>                  setSendingLabel(const bytes_t& outhash, uint32_t outindex, const std::string& label);
    std::shared_ptr<TxOut>                  setReceivingLabel(const bytes_t& outhash, uint32_t outindex, const std::string& label);

    std::shared_ptr<Tx>                     exportTx(const bytes_t& hash, const std::string& filepath) const;
    std::shared_ptr<Tx>                     exportTx(unsigned long tx_id, const std::string& filepath) const;
    void                                    exportTx(std::shared_ptr<Tx> tx, const std::string& filepath) const;
    std::string                             exportTx(const bytes_t& hash) const;
    std::string                             exportTx(unsigned long tx_id) const;
    std::string                             exportTx(std::shared_ptr<Tx> tx) const;
    std::shared_ptr<Tx>                     importTx(const std::string& filepath);
    std::shared_ptr<Tx>                     importTxFromString(const std::string& txstr);
    unsigned int                            exportTxs(const std::string& filepath, uint32_t minheight = 0) const;
    unsigned int                            importTxs(const std::string& filepath);

    //////////////////////////////
    // SIGNINGSCRIPT OPERATIONS //
    //////////////////////////////
    std::shared_ptr<SigningScript>          getSigningScript(const bytes_t& script) const;

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

    /////////////////////
    // USER OPERATIONS //
    /////////////////////
    std::shared_ptr<User>                   addUser(const std::string& username, bool txoutscript_whitelist_enabled = false);
    std::shared_ptr<User>                   getUser(const std::string& username) const;
    const std::set<bytes_t>&                getTxOutScriptWhitelist(const std::string& username) const;
    std::shared_ptr<User>                   setTxOutScriptWhitelist(const std::string& username, const std::set<bytes_t>& txoutscripts);
    std::shared_ptr<User>                   addTxOutScriptToWhitelist(const std::string& username, const bytes_t& txoutscript);
    std::shared_ptr<User>                   removeTxOutScriptFromWhitelist(const std::string& username, const bytes_t& txoutscript);
    std::shared_ptr<User>                   clearTxOutScriptWhitelist(const std::string& username);
    std::shared_ptr<User>                   enableTxOutScriptWhitelist(const std::string& username, bool enabled = true);
    bool                                    isTxOutScriptWhitelistEnabled(const std::string& username) const;

    ////////////////////////
    // SLOT SUBSCRIPTIONS //
    ////////////////////////
    Signals::Connection subscribeKeychainUnlocked(KeychainUnlockedSignal::Slot slot) { return notifyKeychainUnlocked.connect(slot); }
    Signals::Connection subscribeKeychainLocked(KeychainLockedSignal::Slot slot) { return notifyKeychainLocked.connect(slot); }

    Signals::Connection subscribeTxInserted(TxSignal::Slot slot) { return notifyTxInserted.connect(slot); }
    Signals::Connection subscribeTxUpdated(TxSignal::Slot slot) { return notifyTxUpdated.connect(slot); }
    Signals::Connection subscribeTxDeleted(TxSignal::Slot slot) { return notifyTxDeleted.connect(slot); }
    Signals::Connection subscribeMerkleBlockInserted(MerkleBlockSignal::Slot slot) { return notifyMerkleBlockInserted.connect(slot); }

    Signals::Connection subscribeTxInsertionError(TxErrorSignal::Slot slot) { return notifyTxInsertionError.connect(slot); }
    Signals::Connection subscribeMerkleBlockInsertionError(MerkleBlockErrorSignal::Slot slot) { return notifyMerkleBlockInsertionError.connect(slot); }

    Signals::Connection subscribeTxConfirmationError(TxConfirmationErrorSignal::Slot slot) { return notifyTxConfirmationError.connect(slot); }

    void clearAllSlots()
    {
        notifyKeychainUnlocked.clear();
        notifyKeychainLocked.clear();

        notifyTxInserted.clear();
        notifyTxUpdated.clear();
        notifyTxDeleted.clear();
        notifyMerkleBlockInserted.clear();

        notifyTxInsertionError.clear();
        notifyMerkleBlockInsertionError.clear();

        notifyTxConfirmationError.clear();
    }

protected:
    ///////////////////////
    // GLOBAL OPERATIONS //
    ///////////////////////
    uint32_t                                getSchemaVersion_unwrapped() const;
    void                                    setSchemaVersion_unwrapped(uint32_t version);

    std::string                             getNetwork_unwrapped() const;
    void                                    setNetwork_unwrapped(const std::string& network);

    uint32_t                                getHorizonTimestamp_unwrapped() const;
    uint32_t                                getMaxFirstBlockTimestamp_unwrapped() const;
    uint32_t                                getHorizonHeight_unwrapped() const;
    std::vector<bytes_t>                    getLocatorHashes_unwrapped() const;
    Coin::BloomFilter                       getBloomFilter_unwrapped(double falsePositiveRate, uint32_t nTweak, uint32_t nFlags) const;
    hashvector_t                            getIncompleteBlockHashes_unwrapped() const;

    ////////////////////////
    // CONTACT OPERATIONS //
    ////////////////////////
    std::shared_ptr<Contact>                newContact_unwrapped(const std::string& username);
    std::shared_ptr<Contact>                getContact_unwrapped(const std::string& username) const;
    ContactVector                           getAllContacts_unwrapped() const;
    bool                                    contactExists_unwrapped(const std::string& username) const;
    std::shared_ptr<Contact>                renameContact_unwrapped(const std::string& old_username, const std::string& new_username);

    /////////////////////////
    // KEYCHAIN OPERATIONS //
    /////////////////////////
    bool                                    keychainExists_unwrapped(const std::string& keychain_name) const;
    bool                                    keychainExists_unwrapped(const bytes_t& keychain_hash) const;
    std::string                             getNextAvailableKeychainName_unwrapped(const std::string& desired_keychain_name) const;
    bool                                    isKeychainPrivate_unwrapped(const std::string& keychain_name) const;
    std::shared_ptr<Keychain>               getKeychain_unwrapped(const std::string& keychain_name) const;
    void                                    persistKeychain_unwrapped(std::shared_ptr<Keychain> keychain);
    void                                    updateKeychain_unwrapped(std::shared_ptr<Keychain> keychain);
    void                                    exportKeychain_unwrapped(std::shared_ptr<Keychain> keychain, const std::string& filepath) const;
    std::shared_ptr<Keychain>               importKeychain_unwrapped(const std::string& filepath, bool& importprivkeys);
    secure_bytes_t                          exportBIP32_unwrapped(std::shared_ptr<Keychain> keychain, bool export_private) const;
    std::vector<KeychainView>               getRootKeychainViews_unwrapped(const std::string& account_name = "", bool get_hidden = false) const;

    // All the keychain unlock methods use the global keys by default

    void                                    unlockKeychain_unwrapped(std::shared_ptr<Keychain> keychain, const secure_bytes_t& lock_key = secure_bytes_t()) const;

    // The following methods return true iff successful
    bool                                    tryUnlockKeychain_unwrapped(std::shared_ptr<Keychain> keychain, const secure_bytes_t& lock_key = secure_bytes_t()) const;

    ////////////////////////
    // Account operations //
    ////////////////////////
    void                                    exportAccount_unwrapped(Account& account, boost::archive::text_oarchive& oa, bool exportprivkeys) const;
    std::shared_ptr<Account>                importAccount_unwrapped(boost::archive::text_iarchive& ia, unsigned int& privkeysimported);

    void                                    refillAccountPool_unwrapped(std::shared_ptr<Account> account);

    bool                                    accountExists_unwrapped(const std::string& account_name) const;
    std::string                             getNextAvailableAccountName_unwrapped(const std::string& desired_account_name) const;
    std::shared_ptr<Account>                getAccount_unwrapped(const std::string& account_name) const; // throws AccountNotFoundException

    std::vector<TxOutView>                  getUnspentTxOutViews_unwrapped(std::shared_ptr<Account> account, uint32_t min_confirmations = 0) const;

    ////////////////////////////
    // ACCOUNT BIN OPERATIONS //
    ////////////////////////////
    std::shared_ptr<AccountBin>             getAccountBin_unwrapped(const std::string& account_name, const std::string& bin_name) const;
    std::shared_ptr<SigningScript>          issueAccountBinSigningScript_unwrapped(std::shared_ptr<AccountBin> account_bin, const std::string& label = "", uint32_t index = 0);
    void                                    refillAccountBinPool_unwrapped(std::shared_ptr<AccountBin> bin, uint32_t index = 0);
    void                                    exportAccountBin_unwrapped(const std::shared_ptr<AccountBin> account_bin, const std::string& export_name, const std::string& filepath) const;
    std::shared_ptr<AccountBin>             importAccountBin_unwrapped(const std::string& filepath); 

    ///////////////////
    // TX OPERATIONS //
    ///////////////////
    std::shared_ptr<Tx>                     getTx_unwrapped(const bytes_t& hash) const; // Tries both signed and unsigned hashes. Throws TxNotFoundException.
    std::shared_ptr<Tx>                     getTx_unwrapped(unsigned long tx_id) const; // Uses database id. Throws TxNotFoundException.
    txs_t                                   getTxs_unwrapped(int tx_status_flags = Tx::ALL, unsigned long start = 0, int count = -1, uint32_t minheight = 0) const;
    std::vector<std::string>                getSerializedUnsignedTxs_unwrapped(const std::string& account_name) const;
    uint32_t                                getTxConfirmations_unwrapped(std::shared_ptr<Tx> tx) const;
    std::shared_ptr<Tx>                     insertTx_unwrapped(std::shared_ptr<Tx> tx, bool replace_labels = false);
    std::shared_ptr<Tx>                     insertNewTx_unwrapped(const Coin::Transaction& cointx, std::shared_ptr<BlockHeader> blockheader = nullptr, bool verifysigs = false, bool isCoinbase = false);
    std::shared_ptr<Tx>                     insertMerkleTx_unwrapped(const ChainMerkleBlock& chainmerkleblock, const Coin::Transaction& cointx, unsigned int txindex, unsigned int txcount, bool verifysigs = false, bool isCoinbase = false);
    std::shared_ptr<Tx>                     confirmMerkleTx_unwrapped(const ChainMerkleBlock& chainmerkleblock, const bytes_t& txhash, unsigned int txindex, unsigned int txcount);
    std::shared_ptr<Tx>                     createTx_unwrapped(const std::string& account_name, uint32_t tx_version, uint32_t tx_locktime, txouts_t txouts, uint64_t fee, unsigned int maxchangeouts = 1);
    std::shared_ptr<Tx>                     createTx_unwrapped(const std::string& username, const std::string& account_name, uint32_t tx_version, uint32_t tx_locktime, txouts_t txouts, uint64_t fee, unsigned int maxchangeouts = 1);
    std::shared_ptr<Tx>                     createTx_unwrapped(const std::string& account_name, uint32_t tx_version, uint32_t tx_locktime, ids_t coin_ids, txouts_t txouts, uint64_t fee, uint32_t min_confirmations);
    std::shared_ptr<Tx>                     createTx_unwrapped(const std::string& username, const std::string& account_name, uint32_t tx_version, uint32_t tx_locktime, ids_t coin_ids, txouts_t txouts, uint64_t fee, uint32_t min_confirmations);
    txs_t                                   consolidateTxOuts_unwrapped(const std::string& account_name, uint32_t max_tx_size /* in bytes */, uint32_t tx_version, uint32_t tx_locktime, ids_t coin_ids, const bytes_t& txoutscript, uint64_t min_fee, uint32_t min_confirmations);
    void                                    deleteTx_unwrapped(std::shared_ptr<Tx> tx);
    void                                    updateTx_unwrapped(std::shared_ptr<Tx> tx);
    SigningRequest                          getSigningRequest_unwrapped(std::shared_ptr<Tx> tx, bool include_raw_tx = false) const;
    SignatureInfo                           getSignatureInfo_unwrapped(std::shared_ptr<Tx> tx) const;
    unsigned int                            signTx_unwrapped(std::shared_ptr<Tx> tx, std::vector<std::string>& keychain_names); // Tries to sign as many as it can with the unlocked keychains.

    std::shared_ptr<TxOut>                  getTxOut_unwrapped(const bytes_t& outhash, uint32_t outindex) const;
    std::shared_ptr<TxOut>                  setSendingLabel_unwrapped(const bytes_t& outhash, uint32_t outindex, const std::string& label);
    std::shared_ptr<TxOut>                  setReceivingLabel_unwrapped(const bytes_t& outhash, uint32_t outindex, const std::string& label);

    unsigned int                            exportTxs_unwrapped(boost::archive::text_oarchive& oa, uint32_t minheight) const;
    unsigned int                            importTxs_unwrapped(boost::archive::text_iarchive& ia);

    //////////////////////////////
    // SIGNINGSCRIPT OPERATIONS //
    //////////////////////////////
    std::shared_ptr<SigningScript>          getSigningScript_unwrapped(const bytes_t& script) const;

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

    /////////////////////
    // USER OPERATIONS //
    /////////////////////
    std::shared_ptr<User>                   addUser_unwrapped(const std::string& username, bool txoutscript_whitelist_enabled = false);
    std::shared_ptr<User>                   getUser_unwrapped(const std::string& username) const;

    /////////////
    // SIGNALS //
    /////////////
    Signals::SignalQueue                    signalQueue;

    KeychainUnlockedSignal                  notifyKeychainUnlocked;
    KeychainLockedSignal                    notifyKeychainLocked;

    TxSignal                                notifyTxInserted;
    TxSignal                                notifyTxUpdated;
    TxSignal                                notifyTxDeleted;
    MerkleBlockSignal                       notifyMerkleBlockInserted;

    TxErrorSignal                           notifyTxInsertionError;
    MerkleBlockErrorSignal                  notifyMerkleBlockInsertionError;

    TxConfirmationErrorSignal               notifyTxConfirmationError;

private:
    mutable boost::mutex mutex;
    std::shared_ptr<odb::core::database> db_;
    std::string name_;

    mutable std::map<std::string, secure_bytes_t> mapPrivateKeyUnlock;
};

}
