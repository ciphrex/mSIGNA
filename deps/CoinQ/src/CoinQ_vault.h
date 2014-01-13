///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_vault.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef _COINQ_VAULT_H
#define _COINQ_VAULT_H

#include "CoinQ_vault_db.hxx"

#include "CoinQ_signals.h"
#include "CoinQ_slots.h"
#include "CoinQ_blocks.h"

#include <BloomFilter.h>

#include <boost/thread.hpp>

class ChainBlock;

namespace CoinQ {
namespace Vault {

class KeychainInfo
{
public:
    KeychainInfo(unsigned long id, const std::string name, Keychain::type_t type, const bytes_t& hash, unsigned long numkeys)
        : id_(id), name_(name), type_(type), hash_(hash), numkeys_(numkeys) { }

    unsigned long id() const { return id_; }
    const std::string& name() const { return name_; }
    Keychain::type_t type() const { return type_; }
    const bytes_t& hash() const { return hash_; }
    unsigned long numkeys() const { return numkeys_; } 

private:
    unsigned long id_;
    std::string name_;
    Keychain::type_t type_;
    bytes_t hash_;
    unsigned long numkeys_;
};

class AccountInfo
{
public:
    AccountInfo(unsigned long id, const std::string name, unsigned int minsigs, const std::vector<std::string> keychain_names, uint64_t balance = 0, uint64_t pending = 0, unsigned long scripts_remaining = 0)
        :id_(id), name_(name), minsigs_(minsigs), keychain_names_(keychain_names), balance_(balance), pending_(pending), scripts_remaining_(scripts_remaining) { }

    unsigned long id() const { return id_; }
    const std::string& name() const { return name_; }
    unsigned int minsigs() const { return minsigs_; }
    const std::vector<std::string>& keychain_names() { return keychain_names_; }
    uint64_t balance() const { return balance_; }
    uint64_t pending() const { return pending_; }
    unsigned long scripts_remaining() const { return scripts_remaining_; }

private:
    unsigned long id_;
    std::string name_;
    unsigned int minsigs_;
    std::vector<std::string> keychain_names_;
    uint64_t balance_;
    uint64_t pending_;
    unsigned long scripts_remaining_;
};

class Vault
{
public:
    Vault(int argc, char** argv, bool create = false);

    enum result_t {
        OBJECT_INSERTED,
        OBJECT_UPDATED,
        OBJECT_UNCHANGED,
        OBJECT_IGNORED
    };

    // Keychain operations
    bool keychainExists(const std::string& keychain_name) const;
    void newKeychain(const std::string& name, unsigned long numkeys);
    void newHDKeychain(const std::string& name, const bytes_t& extkey, unsigned long numkeys = 100);
    void eraseKeychain(const std::string& keychain_name) const;
    void renameKeychain(const std::string& old_name, const std::string& new_name);
    bytes_t getExtendedKeyBytes(const std::string& keychain_name) const;
    std::vector<KeychainInfo> getKeychains() const;
    std::shared_ptr<Keychain> getKeychain(const std::string& keychain_name) const;
    bytes_t exportKeychain(const std::string& keychain_name, const std::string& filepath, bool exportprivkeys = false) const;
    bytes_t importKeychain(const std::string& keychain_name, const std::string& filepath, bool& importprivkeys);
    bool isKeychainPrivate(const std::string& filepath) const;

    // Account operations
    bool accountExists(const std::string& account_name) const;
    void newAccount(const std::string& name, unsigned int minsigs, const std::vector<std::string>& keychain_names);
    void eraseAccount(const std::string& name) const;
    void renameAccount(const std::string& old_name, const std::string& new_name);
    std::vector<AccountInfo> getAccounts() const;
    std::shared_ptr<Account> getAccount(const std::string& name) const;
    bytes_t exportAccount(const std::string& account_name, const std::string& filepath) const;
    bytes_t importAccount(const std::string& account_name, const std::string& filepath);

    std::vector<std::shared_ptr<SigningScriptView>> getSigningScriptViews(const std::string& account_name, int flags = SigningScript::ALL) const;

    unsigned long getScriptCount(const std::string& account_name, SigningScript::status_t status) const;

    unsigned long generateLookaheadScripts(const std::string& account_name, unsigned long lookahead);

    // Tag operations
    bool scriptTagExists(const bytes_t& txoutscript) const;
    void addScriptTag(const bytes_t& txoutscript, const std::string& description);
    void updateScriptTag(const bytes_t& txoutscript, const std::string& description);
    void deleteScriptTag(const bytes_t& txoutscript);

    // Tx operations
    std::vector<std::shared_ptr<TxOut>> getTxOuts(const std::string& account_name) const;
    std::vector<std::shared_ptr<TxOutView>> getTxOutViews(const std::string& account_name, bool unspentonly = true) const;

    std::shared_ptr<TxOut> newTxOut(const std::string& account_name, const std::string& label, uint64_t value = 0);

    // set max_height to 0xffffffff to also get unconfirmed transactions.
    std::vector<std::shared_ptr<AccountTxOutView>> getAccountHistory(const std::string& account_name, int type_flags = TxOut::ALL, int status_flags = Tx::ALL, uint32_t min_height = 0, uint32_t max_height = 0xffffffff) const;

    // returns true iff transaction belongs to one of our accounts.
    // otherwise, transaction is ignored and false is returned.
    bool addTx(std::shared_ptr<Tx> tx, bool delete_conflicting_txs = false);

    // returns true iff transaction was in vault.
    // otherwise, transaction is ignored and false is returned.
    bool deleteTx(const bytes_t& txhash);

    // constructs an unsigned transaction, fills inputs and change outputs automatically.
    // tx_version = 1 and tx_locktime = 0 are standard.
    std::shared_ptr<Tx> newTx(const std::string& account_name, uint32_t tx_version, uint32_t tx_locktime, const Tx::txouts_t& payment_txouts, uint64_t fee, unsigned int maxchangeouts);

    static bool isSigned(std::shared_ptr<Tx> tx);
    std::shared_ptr<Tx> signTx(std::shared_ptr<Tx> tx) const;

    std::shared_ptr<Tx> getTx(const bytes_t& hash) const;

    bool insertBlock(const ChainBlock& block, bool reinsert_tx = false);
    bool deleteBlock(const bytes_t& hash);

    bool insertMerkleBlock(const ChainMerkleBlock& merkleblock);
    bool deleteMerkleBlock(const bytes_t& hash);

    // checks all transactions with null blockheader to see if they match any merkleblock hashes, then updates the blockheader.
    // returns count of transactions updated.
    unsigned int updateUnconfirmed(const bytes_t& txhash = bytes_t());

    uint32_t getFirstAccountTimeCreated() const;
    uint32_t getBestHeight() const;
    std::shared_ptr<BlockHeader> getBestBlockHeader() const;

    std::vector<bytes_t> getLocatorHashes() const;

    Coin::BloomFilter getBloomFilter(double falsePositiveRate, uint32_t nTweak, uint32_t nFlags = 0) const;

    // EVENT SUBSCRIPTIONS
    void subscribeAddTx(tx_slot_t slot) { notifyAddTx.connect(slot); }
    void subscribeInsertBlock(chain_block_slot_t slot) { notifyInsertBlock.connect(slot); }

protected:
    // unwrapped methods must be called from within a session and transaction
    void persistKeychain_unwrapped(Keychain& keychain);
    void updateKeychain_unwrapped(std::shared_ptr<Keychain> keychain);

    unsigned long generateLookaheadScripts_unwrapped(const std::string& account_name, unsigned long lookahead);

    result_t insertTx_unwrapped(std::shared_ptr<Tx>& tx, bool delete_conflicting_txs = false);
    bool deleteTx_unwrapped(const bytes_t& txhash);
    bool deleteBlock_unwrapped(const bytes_t& hash);
    bool deleteMerkleBlock_unwrapped(const bytes_t& hash);
    unsigned int updateUnconfirmed_unwrapped(const bytes_t& txhash = bytes_t());
    uint32_t getFirstAccountTimeCreated_unwrapped() const;

    mutable boost::mutex mutex;

    CoinQSignal<const Coin::Transaction&> notifyAddTx;
    CoinQSignal<const ChainBlock&> notifyInsertBlock;

private:
    std::shared_ptr<odb::core::database> db_;
};

}
}

#endif // _COINQ_VAULT_h
