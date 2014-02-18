///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_vault_db.hxx
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

// Compile with:
//      odb -I../deps/CoinClasses/src --std c++11 -d sqlite --generate-query --generate-schema CoinQ_vault_db.hxx
//      g++ -c CoinQ_vault_db-odb.cxx -I../deps/CoinClasses/src -std=c++0x
//

#ifndef _COINQ_VAULT_DB_HXX
#define _COINQ_VAULT_DB_HXX

#include "CoinQ_script.h"

#include "CoinQ_typedefs.h"

#include <uchar_vector.h>
#include <hash.h>
#include <CoinNodeData.h>
#include <hdkeys.h>

#include <odb/core.hxx>
#include <odb/nullable.hxx>
#include <odb/database.hxx>
//#include <odb/lazy-ptr.hxx>

#include <vector>
#include <set>
#include <memory>
#include <stdexcept>

#include <logger.h>

namespace CoinQ {
#pragma db namespace session
namespace Vault {

typedef odb::nullable<unsigned long> null_id_t;

#pragma db value(bytes_t) type("BLOB")

class Tx;
class Account;
class SigningScript;
class ScriptTag;

const uint32_t SCHEMA_VERSION = 1;

// Vault schema version
#pragma db object pointer(std::shared_ptr)
class Version
{
public:
    Version() : version_(0) { }
    Version(uint32_t version) : version_(version) { }

    void version(uint32_t version) { version_ = version; }
    uint32_t version() const { return version_; }

private:
    friend class odb::access;

    #pragma db id auto
    unsigned long id_;

    uint32_t version_;
};

// Blocks, Transactions
#pragma db object pointer(std::shared_ptr)
class BlockHeader
{
public:
    BlockHeader() { }

    BlockHeader(const Coin::CoinBlockHeader& blockheader, uint32_t height = 0xffffffff) { fromCoinClasses(blockheader, height); }

    BlockHeader(const bytes_t& hash, uint32_t height, uint32_t version, const bytes_t& prevhash, const bytes_t& merkleroot, uint32_t timestamp, uint32_t bits, uint32_t nonce)
    : hash_(hash), height_(height), version_(version), prevhash_(prevhash), merkleroot_(merkleroot), timestamp_(timestamp), bits_(bits), nonce_(nonce) { }

    void fromCoinClasses(const Coin::CoinBlockHeader& blockheader, uint32_t height = 0xffffffff);
    Coin::CoinBlockHeader toCoinClasses() const;

    unsigned long id() const { return id_; }
    bytes_t hash() const { return hash_; }
    uint32_t height() const { return height_; }
    uint32_t version() const { return version_; }
    bytes_t prevhash() const { return prevhash_; }
    bytes_t merkleroot() const { return merkleroot_; }
    uint32_t timestamp() const { return timestamp_; }
    uint32_t bits() const { return bits_; }
    uint32_t nonce() const { return nonce_; }

private:
    friend class odb::access;

    #pragma db id auto
    unsigned long id_;

    #pragma db unique
    bytes_t hash_;

    #pragma db unique
    uint32_t height_;

    uint32_t version_;
    bytes_t prevhash_;
    bytes_t merkleroot_;
    uint32_t timestamp_;
    uint32_t bits_;
    uint32_t nonce_;
};

inline void BlockHeader::fromCoinClasses(const Coin::CoinBlockHeader& blockheader, uint32_t height)
{
    hash_ = blockheader.getHashLittleEndian();
    height_ = height;
    version_ = blockheader.version;
    prevhash_ = blockheader.prevBlockHash;
    merkleroot_ = blockheader.merkleRoot;
    timestamp_ = blockheader.timestamp;
    bits_ = blockheader.bits;
    nonce_ = blockheader.nonce;
}

inline Coin::CoinBlockHeader BlockHeader::toCoinClasses() const
{
    return Coin::CoinBlockHeader(version_, timestamp_, bits_, nonce_, prevhash_, merkleroot_);    
}

#pragma db object pointer(std::shared_ptr)
class MerkleBlock
{
public:
    MerkleBlock() { }

    MerkleBlock(const std::shared_ptr<BlockHeader>& blockheader, uint32_t txcount, const std::vector<bytes_t>& hashes, const bytes_t& flags)
        : blockheader_(blockheader), txcount_(txcount), hashes_(hashes), flags_(flags) { }

    void fromCoinClasses(const Coin::MerkleBlock& merkleblock, uint32_t height = 0xffffffff);
    Coin::MerkleBlock toCoinClasses() const;

    unsigned long id() const { return id_; }

    // The logic of blockheader management and persistence is handled by the user of this class.
    void blockheader(const std::shared_ptr<BlockHeader>& blockheader) { blockheader_ = blockheader; }
    std::shared_ptr<BlockHeader> blockheader() const { return blockheader_; }

    void txcount(uint32_t txcount) { txcount_ = txcount; }
    uint32_t txcount() const { return txcount_; }

    void hashes(const std::vector<bytes_t>& hashes) { hashes_ = hashes; }
    const std::vector<bytes_t>& hashes() const { return hashes_; }

    void flags(const bytes_t& flags) { flags_ = flags; }
    const bytes_t& flags() const { return flags_; }

private:
    friend class odb::access;

    #pragma db id auto
    unsigned long id_;

    #pragma db not_null
    std::shared_ptr<BlockHeader> blockheader_;

    uint32_t txcount_;

    #pragma db value_not_null \
        id_column("object_id") value_column("value")
//        id_column("merkleblock_id") value_column("hash")
    std::vector<bytes_t> hashes_;

    bytes_t flags_;
};

inline void MerkleBlock::fromCoinClasses(const Coin::MerkleBlock& merkleblock, uint32_t height)
{
    blockheader_ = std::shared_ptr<BlockHeader>(new BlockHeader(merkleblock.blockHeader, height));
    txcount_ = merkleblock.nTxs;
    hashes_.assign(merkleblock.hashes.begin(), merkleblock.hashes.end());
    flags_ = merkleblock.flags; 
}

inline Coin::MerkleBlock MerkleBlock::toCoinClasses() const
{
    Coin::MerkleBlock merkleblock;
    merkleblock.blockHeader = blockheader_->toCoinClasses();
    merkleblock.nTxs = txcount_;
    merkleblock.hashes.assign(hashes_.begin(), hashes_.end());
    merkleblock.flags = flags_;
    return merkleblock;
}

#pragma db object pointer(std::shared_ptr)
class TxIn
{
public:
    TxIn(const bytes_t& outhash, uint32_t outindex, const bytes_t& script, uint32_t sequence)
        : outhash_(outhash), outindex_(outindex), script_(script), sequence_(sequence) { }

    TxIn(const Coin::TxIn& coin_txin);
    TxIn(const bytes_t& raw);

    Coin::TxIn toCoinClasses() const;

    unsigned long id() const { return id_; }
    const bytes_t& outhash() const { return outhash_; }
    uint32_t outindex() const { return outindex_; }

    void script(const bytes_t& script) { script_ = script; }
    const bytes_t& script() const { return script_; }

    uint32_t sequence() const { return sequence_; }
    bytes_t raw() const;

    void tx(std::weak_ptr<Tx> tx) { tx_ = tx; }
    const std::shared_ptr<Tx> tx() const { return tx_.lock(); }

    void txindex(unsigned int txindex) { txindex_ = txindex; }
    uint32_t txindex() const { return txindex_; }

private:
    friend class odb::access;

    TxIn() { }

    #pragma db id auto
    unsigned long id_;

    bytes_t outhash_;
    uint32_t outindex_;
    bytes_t script_;
    uint32_t sequence_;

    #pragma db null inverse(txins_)
    std::weak_ptr<Tx> tx_;
    uint32_t txindex_; 
};

inline TxIn::TxIn(const Coin::TxIn& coin_txin)
{
    outhash_ = coin_txin.getOutpointHash();
    outindex_ = coin_txin.getOutpointIndex();
    script_ = coin_txin.scriptSig;
    sequence_ = coin_txin.sequence;
}

inline TxIn::TxIn(const bytes_t& raw)
{
    Coin::TxIn coin_txin(raw);
    outhash_ = coin_txin.getOutpointHash();
    outindex_ = coin_txin.getOutpointIndex();
    script_ = coin_txin.scriptSig;
    sequence_ = coin_txin.sequence;
}

inline Coin::TxIn TxIn::toCoinClasses() const
{
    Coin::TxIn coin_txin;
    coin_txin.previousOut = Coin::OutPoint(outhash_, outindex_);
    coin_txin.scriptSig = script_;
    coin_txin.sequence = sequence_;
    return coin_txin;
}

inline bytes_t TxIn::raw() const
{
    return toCoinClasses().getSerialized();
}

#pragma db object pointer(std::shared_ptr)
class TxOut
{
public:
    enum type_t { NONE = 1, CHANGE = 2, DEBIT = 4, CREDIT = 8, ALL = 15 };

    TxOut(uint64_t value, const bytes_t& script, const null_id_t& account_id = null_id_t(), type_t type = NONE)
        : value_(value), script_(script), account_id_(account_id), type_(type) { }

    TxOut(const Coin::TxOut& coin_txout, const null_id_t& account_id = null_id_t(), type_t type = NONE);
    TxOut(const bytes_t& raw, const null_id_t& account_id = null_id_t(), type_t type = NONE);

    Coin::TxOut toCoinClasses() const;

    unsigned long id() const { return id_; }
    uint64_t value() const { return value_; }
    const bytes_t& script() const { return script_; }
    bytes_t raw() const;

    void tx(std::weak_ptr<Tx> tx) { tx_ = tx; }
    const std::shared_ptr<Tx> tx() const { return tx_.lock(); }

    void txindex(uint32_t txindex) { txindex_ = txindex; }
    uint32_t txindex() const { return txindex_; }

    void spent(std::shared_ptr<TxIn> spent) { spent_ = spent; }
    const std::shared_ptr<TxIn> spent() const { return spent_; }

    const std::shared_ptr<SigningScript> signingscript() const { return signingscript_.lock(); }

    void account_id(const null_id_t& account_id) { account_id_ = account_id; }
    const null_id_t& account_id() const { return account_id_; }

    void type(type_t type) { type_ = type; }
    type_t type() const { return type_; }

private:
    friend class odb::access;

    TxOut() { }

    #pragma db id auto
    unsigned long id_;

    uint64_t value_;
    bytes_t script_;

    #pragma db null inverse(txouts_)
    std::weak_ptr<Tx> tx_;
    uint32_t txindex_;

    #pragma db null
    std::shared_ptr<TxIn> spent_;

    friend class SigningScript;

    #pragma db null
    std::weak_ptr<SigningScript> signingscript_;

    null_id_t  account_id_;

    type_t type_; 
};

inline TxOut::TxOut(const Coin::TxOut& coin_txout, const null_id_t& account_id, type_t type)
{
    value_ = coin_txout.value;
    script_ = coin_txout.scriptPubKey;
    account_id_ = account_id;
    type_ = type;
}

inline TxOut::TxOut(const bytes_t& raw, const null_id_t& account_id, type_t type)
{
    Coin::TxOut coin_txout(raw);
    value_ = coin_txout.value;
    script_ = coin_txout.scriptPubKey;
    account_id_ = account_id;
    type_ = type;
}

inline Coin::TxOut TxOut::toCoinClasses() const
{
    Coin::TxOut coin_txout;
    coin_txout.value = value_;
    coin_txout.scriptPubKey = script_;
    return coin_txout;
}

inline bytes_t TxOut::raw() const
{
    return toCoinClasses().getSerialized();
}

#pragma db object pointer(std::shared_ptr)
class Tx : public std::enable_shared_from_this<Tx>
{
public:
    enum status_t { UNSIGNED = 1, UNSENT = 2, SENT = 4, RECEIVED = 8, ALL = 15 };
    /*
     * UNSIGNED - created in the vault, not yet signed.
     * UNSENT   - signed but not yet broadcast to network.
     * SENT     - broadcast to network by us.
     * RECEIVED - received from network.
     *
     * If status is UNSIGNED, remove all txinscripts before hashing so hash
     * is unchanged by adding signatures until fully signed. Then compute
     * normal hash and transition to one of the other states.
     *
     * Note: Status is always set to RECEIVED when we get it from verifier
     *  node even if we sent it.
     */

    Tx(uint32_t version = 1, uint32_t locktime = 0, uint32_t timestamp = 0xffffffff, status_t status = RECEIVED)
        : version_(version), locktime_(locktime), timestamp_(timestamp), status_(status), have_fee_(false), fee_(0) { }

    typedef std::vector<std::shared_ptr<TxIn>> txins_t;
    typedef std::vector<std::shared_ptr<TxOut>> txouts_t;

    void set(uint32_t version, const txins_t& txins, const txouts_t& txouts, uint32_t locktime, uint32_t timestamp = 0xffffffff, status_t status = RECEIVED);
    void set(const Coin::Transaction& coin_tx, uint32_t timestamp = 0xffffffff, status_t status = RECEIVED);
    void set(const bytes_t& raw, uint32_t timestamp = 0xffffffff, status_t status = RECEIVED);

    Coin::Transaction toCoinClasses() const;

    void setBlock(std::shared_ptr<BlockHeader> blockheader, uint32_t blockindex);

    unsigned long id() const { return id_; }
    const bytes_t& hash() const { return hash_; }
    bytes_t unsigned_hash() const { return unsigned_hash_; }
    const txins_t& txins() const { return txins_; }
    const txouts_t& txouts() const { return txouts_; }
    uint32_t locktime() const { return locktime_; }
    bytes_t raw() const;

    void timestamp(uint32_t timestamp) { timestamp_ = timestamp; } 
    uint32_t timestamp() const { return timestamp_; }

    void status(status_t status);
    status_t status() const { return status_; }

    void fee(uint64_t fee) { have_fee_ = true; fee_ = fee; }
    uint64_t fee() const { return fee_; }

    bool have_fee() const { return have_fee_; }

    void block(std::shared_ptr<BlockHeader> header, uint32_t index) { blockheader_ = header, blockindex_ = index; }

    void blockheader(std::shared_ptr<BlockHeader> blockheader) { blockheader_ = blockheader; }
    std::shared_ptr<BlockHeader> blockheader() const { return blockheader_; }

    void shuffle_txins();
    void shuffle_txouts();

    void updateUnsignedHash();
    void updateHash();

private:
    friend class odb::access;

    void fromCoinClasses(const Coin::Transaction& coin_tx);

    #pragma db id auto
    unsigned long id_;

    #pragma db unique
    bytes_t hash_;

    #pragma db unique
    bytes_t unsigned_hash_;

    uint32_t version_;

    #pragma db value_not_null
    txins_t txins_;

    #pragma db value_not_null
    txouts_t txouts_;

    uint32_t locktime_;

    // Timestamp should be set each time we modify the transaction.
    // Once status is RECEIVED the timestamp is fixed.
    // Timestamp defaults to 0xffffffff
    uint32_t timestamp_;

    status_t status_; 

    // Due to issue with odb::nullable<uint64_t> in mingw-w64, we use a second bool variable.
    bool have_fee_;
    uint64_t fee_;

    #pragma db null
    std::shared_ptr<BlockHeader> blockheader_;

    #pragma db null
    odb::nullable<uint32_t> blockindex_; 
};

inline void Tx::updateHash()
{
    if (status_ != UNSIGNED) {
        hash_ = sha256_2(raw());
        std::reverse(hash_.begin(), hash_.end());
    }
    else {
        // TODO: should be nullable unique.
        hash_ = unsigned_hash_;
    }
}

inline void Tx::updateUnsignedHash()
{
    Coin::Transaction coin_tx = toCoinClasses();
    coin_tx.clearScriptSigs();
    unsigned_hash_ = coin_tx.getHashLittleEndian();
}

inline void Tx::set(uint32_t version, const txins_t& txins, const txouts_t& txouts, uint32_t locktime, uint32_t timestamp, status_t status)
{
    version_ = version;

    int i = 0;
    for (auto& txin: txins) { txin->tx(shared_from_this()); txin->txindex(i++); }
    txins_ = txins;

    i = 0;
    for (auto& txout: txouts) { txout->tx(shared_from_this()); txout->txindex(i++); }
    txouts_ = txouts;

    locktime_ = locktime;

    timestamp_ = timestamp;
    status_ = status;

    updateUnsignedHash();
    updateHash();
}

inline void Tx::set(const Coin::Transaction& coin_tx, uint32_t timestamp, status_t status)
{
    LOGGER(trace) << "Tx::set - fromCoinClasses(coin_tx);" << std::endl;
    fromCoinClasses(coin_tx);

    timestamp_ = timestamp;
    status_ = status;

    updateUnsignedHash();
    updateHash();
}

inline void Tx::set(const bytes_t& raw, uint32_t timestamp, status_t status)
{
    Coin::Transaction coin_tx(raw);
    fromCoinClasses(coin_tx);
    timestamp_ = timestamp;
    status_ = status;

    updateUnsignedHash();
    updateHash();
}

inline void Tx::status(status_t status)
{
    if (status_ != status) {
        status_ = status;
        //updateHash();
    }
}

inline Coin::Transaction Tx::toCoinClasses() const
{
    Coin::Transaction coin_tx;
    coin_tx.version = version_;
    for (auto& txin:  txins_ ) { coin_tx.inputs.push_back(txin->toCoinClasses()); }
    for (auto& txout: txouts_) { coin_tx.outputs.push_back(txout->toCoinClasses()); }
    coin_tx.lockTime = locktime_;
    return coin_tx;
}

inline void Tx::setBlock(std::shared_ptr<BlockHeader> blockheader, uint32_t blockindex)
{
    blockheader_ = blockheader;
    blockindex_ = blockindex;
}

inline bytes_t Tx::raw() const
{
    return toCoinClasses().getSerialized(); 
}

inline void Tx::shuffle_txins()
{
    int i = 0;
    std::random_shuffle(txins_.begin(), txins_.end());
    for (auto& txin: txins_) { txin->txindex(i++); }
}

inline void Tx::shuffle_txouts()
{
    int i = 0;
    std::random_shuffle(txouts_.begin(), txouts_.end());
    for (auto& txout: txouts_) { txout->txindex(i++); }
}

inline void Tx::fromCoinClasses(const Coin::Transaction& coin_tx)
{
    version_ = coin_tx.version;

    int i = 0;
    txins_.clear();
    for (auto& coin_txin: coin_tx.inputs) {
        std::shared_ptr<TxIn> txin(new TxIn(coin_txin));
        txin->tx(shared_from_this());
        txin->txindex(i++);
        txins_.push_back(txin);
    }

    i = 0;
    txouts_.clear();
    for (auto& coin_txout: coin_tx.outputs) {
        std::shared_ptr<TxOut> txout(new TxOut(coin_txout));
        txout->tx(shared_from_this());
        txout->txindex(i++);
        txouts_.push_back(txout);
    }

    locktime_ = coin_tx.lockTime;
}

#pragma db object pointer(std::shared_ptr)
class ScriptTag
{
public:
    ScriptTag(const bytes_t& txoutscript, const std::string& description)
        : txoutscript_(txoutscript), description_(description) { }

    unsigned long id() const { return id_; }

    const bytes_t& txoutscript() const { return txoutscript_; }

    void description(const std::string& description) { description_ = description; }
    const std::string& description() const { return description_; }

private:
    friend class odb::access;

    ScriptTag() { }

    #pragma db id auto
    unsigned long id_;
 
    #pragma db unique
    bytes_t txoutscript_;

    std::string description_;
};


/*
#pragma db object
class HDKey
{
public:
    HDKey(const Coin::HDKeychain& keychain);

    unsigned long id() const { return id_; }

    uint32_t version() const { return version_; }
    unsigned char depth() const { return depth_; }
    uint32_t parent_fp() const { return parent_fp_; }
    uint32_t child_num() const { return child_num_; }
    const bytes_t& chain_code() const { return chain_code_; }
    const bytes_t& key() const { return key_; }

    Coin::HDKeychain keychain() const;

private:
    friend class odb::access;

    HDKey() { }

    #pragma db id auto
    unsigned long id_;

    uint32_t version_;
    unsigned char depth_;
    uint32_t parent_fp_;
    uint32_t child_num_;
    bytes_t chain_code_;
    bytes_t key_;
};

inline HDKey::HDKey(const Coin::HDKeychain& keychain)
{
    if (!keychain.isValid()) {
        throw std::runtime_error("HDKeychain is invalid.");
    }

    version_ = keychain.version();
    depth_ = keychain.depth();
    parent_fp_ = keychain.parent_fp();
    child_num_ = keychain.child_num();
    chain_code_ = keychain.chain_code();
    key_ = keychain.key();
}

inline Coin::HDKeychain HDKey::keychain() const
{
    return HDKeychain(key_, chain_code_, child_num_, parent_fp_, depth_);
}
*/

// Keys, Accounts
#pragma db object pointer(std::shared_ptr)
class ExtendedKey
{
public:
    ExtendedKey(const bytes_t& bytes)
        : bytes_(bytes) { }

    const bytes_t& bytes() const { return bytes_; }

    Coin::HDKeychain hdkeychain() const { return Coin::HDKeychain(bytes_); }

private:
    friend class odb::access;

    ExtendedKey() { }

    #pragma db id auto
    unsigned long id_;

    #pragma db unique
    bytes_t bytes_;
};

#pragma db object pointer(std::shared_ptr)
class Key
{
public:
    // For random keys
    Key(const bytes_t& pubkey, const bytes_t& privkey)
        : pubkey_(pubkey), privkey_(privkey), childnum_(0) { }

    Key(const bytes_t& pubkey)
        : pubkey_(pubkey), childnum_(0) { }

    // For deterministic keys
    Key(const std::shared_ptr<ExtendedKey>& extendedkey, uint32_t childnum);

    unsigned long id() const { return id_; }
    bytes_t privkey() const;
    const bytes_t& pubkey() const { return pubkey_; }

private:
    friend class odb::access;

    Key() { }

    #pragma db id auto
    unsigned long id_;

    bytes_t pubkey_;

    // privkey is null for nonprivate keys
    // privkey is nonnull but empty for private deterministic keys. This allows us to query by privkey.is_not_null.
    #pragma db null
    odb::nullable<bytes_t> privkey_;

    // extended key as per BIP0032
    #pragma db value_null
    std::shared_ptr<ExtendedKey> extendedkey_;

    uint32_t childnum_;
};

inline Key::Key(const std::shared_ptr<ExtendedKey>& extendedkey, uint32_t childnum)
    : extendedkey_(extendedkey), childnum_(childnum)
{
    Coin::HDKeychain hdkeychain = extendedkey_->hdkeychain();
    if (hdkeychain.isPrivate()) { privkey_ = bytes_t(); }
    hdkeychain = hdkeychain.getChild(childnum_);
    pubkey_ = hdkeychain.pubkey();
}

inline bytes_t Key::privkey() const
{
    if (!privkey_) {
        throw std::runtime_error("Key::privkey - cannot get private key from nonprivate key object.");
    }

    if (extendedkey_) {
        Coin::HDKeychain hdkeychain = extendedkey_->hdkeychain();
        if (!hdkeychain.isPrivate()) {
            throw std::runtime_error("Key::privkey - cannot get private key from nonprivate key object.");
        }
        return hdkeychain.getChild(childnum_ | 0x80000000).privkey();
    }

    return *privkey_;
}

#pragma db object pointer(std::shared_ptr)
class Keychain
{
public:
    enum type_t { PUBLIC, PRIVATE };

    // constructor for random keychain
    Keychain(const std::string& name, type_t type)
        : name_(name), type_(type), numkeys_(0), nextkeyindex_(0), numsavedkeys_(0) { }

    // constructor for deterministic keychain (BIP0032)
    Keychain(const std::string& name, const std::shared_ptr<ExtendedKey>& extendedkey, unsigned long numkeys = 0);
    //Keychain(const std::string& name, const Coin::HDKeychain& hdkeychain, unsigned long numkeys = 0);

    bool is_deterministic() const { return extendedkey_ != NULL; }
    bool is_private() const { return type_ == PRIVATE; }

    unsigned long id() const { return id_; }

    void name(const std::string& name) { name_ = name; }
    const std::string name() const { return name_; }

    type_t type() const { return type_; }
    std::shared_ptr<ExtendedKey> extendedkey() const { return extendedkey_; }
    const bytes_t& hash() const { return hash_; }
    unsigned long numkeys() const { return numkeys_; }

    unsigned long numsavedkeys() const { return numsavedkeys_; }
    void numsavedkeys(unsigned long numsavedkeys) { numsavedkeys_ = numsavedkeys; }

    typedef std::vector<std::shared_ptr<Key>> keys_t;
    const keys_t& keys() const { return keys_; }

    // only for random keychains
    void add(std::shared_ptr<Key> key);

    // only for deterministic keychains

    // numkeys:
    //   creates all child keys from 0 to numkeys - 1, returns total existing keys.
    //   only creates keys that do not yet exist - if existing keys is larger than numkeys, no new keys are created.
    unsigned long numkeys(unsigned long numkeys);

    std::shared_ptr<Keychain> child(const std::string& name, uint32_t i, unsigned long numkeys = 0);

private:
    friend class odb::access;

    Keychain() { }

    #pragma db id auto
    unsigned long id_;

    #pragma db unique
    std::string name_;

    type_t type_;

    // extended key as per BIP0032
    #pragma db value_null
    std::shared_ptr<ExtendedKey> extendedkey_; 

    bytes_t hash_;
    unsigned long numkeys_;

    // nextkeyindex is the next child index for a deterministic keychain. necessary because some indices might be invalid.
    unsigned long nextkeyindex_;

    // numsavedkeys is the number of keys actually stored in database. used for update.
    unsigned long numsavedkeys_;

    #pragma db value_not_null
    keys_t keys_;
};

inline Keychain::Keychain(const std::string& name, const std::shared_ptr<ExtendedKey>& extendedkey, unsigned long numkeys)
    : name_(name), extendedkey_(extendedkey), numkeys_(0), nextkeyindex_(0), numsavedkeys_(0)
{
    Coin::HDKeychain hdkeychain = extendedkey_->hdkeychain();
    if (hdkeychain.isPrivate()) {
        type_ = PRIVATE;
        hash_ = sha256_2(hdkeychain.getPublic().extkey());
    }
    else {
        type_ = PUBLIC;
        hash_ = sha256_2(hdkeychain.extkey());
    }
    this->numkeys(numkeys);
}

inline void Keychain::add(std::shared_ptr<Key> key)
{
    if (is_deterministic()) {
        throw std::runtime_error("Keychain::add - cannot add new keys to deterministic keychain.");
    }

    hash_ = sha256(uchar_vector(hash_) + key->pubkey());
    keys_.push_back(key);
    numkeys_++;
}

inline unsigned long Keychain::numkeys(unsigned long numkeys)
{
    LOGGER(trace) << "Keychain::numkeys(" << numkeys << ")" << std::endl;
    if (!is_deterministic()) {
        throw std::runtime_error("Keychain::numkeys - cannot set numkeys for random wallet.");
    }

    if (numkeys_ >= numkeys) return numkeys_;

    if (numkeys > 0x7fffffff) {
        throw std::runtime_error("Keychain::numkeys - cannot have more than 0x7fffffff keys.");
    }

    Coin::HDKeychain hdkeychain = extendedkey_->hdkeychain();
    uint32_t privmask = (type_ == PRIVATE) ? 0x80000000 : 0x00000000;

    for (uint32_t i = numkeys_; i < numkeys; i++) {
        while (true) {
            uint32_t childnum = nextkeyindex_++ | privmask;
            Coin::HDKeychain child = hdkeychain.getChild(childnum);
            if (!child) continue;
            std::shared_ptr<Key> key(new Key(extendedkey_, childnum)); 
            keys_.push_back(key);
            break;
        }
    }
    numkeys_ = numkeys;
    return numkeys_;
}

inline std::shared_ptr<Keychain> Keychain::child(const std::string& name, uint32_t i, unsigned long numkeys)
{
    if (!is_deterministic()) {
        throw std::runtime_error("Keychain::child - cannot get child key for random keychain.");
    }

    Coin::HDKeychain parent = extendedkey_->hdkeychain();
    std::shared_ptr<ExtendedKey> child(new ExtendedKey(parent.getChild(i).extkey()));
    std::shared_ptr<Keychain> keychain(new Keychain(name, child, numkeys));
    return keychain;
}

#pragma db object pointer(std::shared_ptr)
class SigningScript : public std::enable_shared_from_this<SigningScript>
{
public:
    enum status_t { UNUSED = 1, CHANGE = 2, REQUEST = 4, RECEIPT = 8, ALL = 15 };
    static std::string getStatusString(int status)
    {
        switch (status) {
        case UNUSED: return "UNUSED";
        case CHANGE: return "CHANGE";
        case REQUEST: return "REQUEST";
        case RECEIPT: return "RECEIPT";
        default: return "UNKNOWN";
        }
    }

    SigningScript(const bytes_t& txinscript, const bytes_t& txoutscript, const std::string& label = "", status_t status = UNUSED)
        : label_(label), status_(status), txinscript_(txinscript), txoutscript_(txoutscript) { }

    unsigned long id() const { return id_; }
    void label(const std::string& label) { label_ = label; }
    const std::string label() const { return label_; }

    void status(status_t status) { status_ = status; }
    status_t status() const { return status_; }

    const bytes_t& txinscript() const { return txinscript_; }
    const bytes_t& txoutscript() const { return txoutscript_; }

    void addTxOut(std::shared_ptr<TxOut> txout);

    void account(std::shared_ptr<Account> account) { account_ = account; }
    std::shared_ptr<Account> account() const { return account_.lock(); }

private:
    friend class odb::access;

    SigningScript() { }

    #pragma db id auto
    unsigned long id_;

    std::string label_;
    status_t status_;

    bytes_t txinscript_; // unsigned (0 byte length placeholders are used for signatures)
    bytes_t txoutscript_; 

    #pragma db value_not_null inverse(signingscript_)
    Tx::txouts_t txouts_;

    friend class Account;
    #pragma db not_null inverse(signingscripts_)
    std::weak_ptr<Account> account_;
};

inline void SigningScript::addTxOut(std::shared_ptr<TxOut> txout)
{
    txouts_.push_back(txout);
    txout->signingscript_ = shared_from_this();
}

#pragma db object pointer(std::shared_ptr)
class Account : public std::enable_shared_from_this<Account>
{
public:
    Account() { }

    typedef std::vector<std::shared_ptr<SigningScript>> signingscripts_t;
    void set(const std::string& name, unsigned int minsigs, const std::set<bytes_t>& keychain_hashes, const signingscripts_t& scripts, uint32_t time_created = time(NULL));

    typedef std::set<std::shared_ptr<Keychain>> keychains_t;
    void set(const std::string& name, unsigned int minsigs, const keychains_t& keychains, uint32_t time_created = time(NULL));

    void extend(const keychains_t& keychains);

    unsigned long id() const { return id_; }

    void name(const std::string& name) { name_ = name; }
    const std::string& name() const { return name_; }

    unsigned int minsigs() const { return minsigs_; }

    uint32_t time_created() const { return time_created_; }

    const std::set<bytes_t>& keychain_hashes() const { return keychain_hashes_; }

    const signingscripts_t& scripts() const { return signingscripts_; }

private:
    friend class odb::access;

    #pragma db id auto
    unsigned long id_;

    #pragma db unique
    std::string name_;

    unsigned int minsigs_;

    #pragma db value_not_null \
        id_column("account_id") value_column("keychain_hash")
    std::set<bytes_t> keychain_hashes_;

    #pragma db value_not_null \
        id_column("account_id") value_column("signingscript_id")
    signingscripts_t signingscripts_;

    uint32_t time_created_;
};


inline void Account::set(const std::string& name, unsigned int minsigs, const std::set<bytes_t>& keychain_hashes, const signingscripts_t& scripts, uint32_t time_created)
{
    name_ = name;
    minsigs_ = minsigs;
    keychain_hashes_ = keychain_hashes;
    signingscripts_ = scripts;
    time_created_ = time_created;
}

inline void Account::set(const std::string& name, unsigned int minsigs, const keychains_t& keychains, uint32_t time_created)
{
    // Find the smallest keychain's length. That will be the number of scripts in the account.
    unsigned long minkeys = 0;
    for (auto& keychain: keychains) {
        if (minkeys == 0) {
            minkeys = keychain->numkeys();
        }
        else if (keychain->numkeys() < minkeys) {
            minkeys = keychain->numkeys();
        }
    }

    // Create pubkey tuples. Sort them to hide which pubkey belongs to which keychain and to ensure only one signing script per combination.
    for (unsigned long i = 0; i < minkeys; i++) {
        std::vector<bytes_t> pubkeys;
        for (auto& keychain: keychains) {
            pubkeys.push_back(keychain->keys()[i]->pubkey());
        }
        // Sort the pubkeys to ensure the same set of pubkeys always generates the same multisig
        std::sort(pubkeys.begin(), pubkeys.end());

        CoinQ::Script::Script script(CoinQ::Script::Script::PAY_TO_MULTISIG_SCRIPT_HASH, minsigs, pubkeys);
        std::shared_ptr<SigningScript> signingscript(new SigningScript(script.txinscript(CoinQ::Script::Script::EDIT), script.txoutscript()));
        signingscripts_.push_back(signingscript);
        signingscript->account_ = shared_from_this();
    }

    name_ = name;
    minsigs_ = minsigs;
    time_created_ = time_created;

    keychain_hashes_.clear();
    for (auto& keychain: keychains) {
        keychain_hashes_.insert(keychain->hash());
    }
}

inline void Account::extend(const keychains_t& keychains)
{
    // TODO: check keychain hashes

    // Find the smallest keychain's length. That will be the number of scripts in the account.
    unsigned long minkeys = 0;
    for (auto& keychain: keychains) {
        if (minkeys == 0) {
            minkeys = keychain->numkeys();
        }
        else if (keychain->numkeys() < minkeys) {
            minkeys = keychain->numkeys();
        }
    }

    unsigned long numscripts = signingscripts_.size();
    if (minkeys <= numscripts) return; // the < should never occur or we won't be able to sign! (TODO: assert)

    for (unsigned long i = numscripts; i < minkeys; i++) {
        std::vector<bytes_t> pubkeys;
        for (auto& keychain: keychains) {
            pubkeys.push_back(keychain->keys()[i]->pubkey());
        }
        // Sort the pubkeys to ensure the same set of pubkeys always generates the same multisig
        std::sort(pubkeys.begin(), pubkeys.end());

        CoinQ::Script::Script script(CoinQ::Script::Script::PAY_TO_MULTISIG_SCRIPT_HASH, minsigs_, pubkeys);
        std::shared_ptr<SigningScript> signingscript(new SigningScript(script.txinscript(CoinQ::Script::Script::EDIT), script.txoutscript()));
        signingscripts_.push_back(signingscript);
        signingscript->account_ = shared_from_this(); 
   } 
}

// Views
#pragma db view object(Account)
struct AccountView
{
    unsigned long id;
    std::string name;
    unsigned int minsigs;
};

#pragma db view \
    object(Account) \
    table("Account_keychain_hashes" = "t": "t.account_id = " + Account::id_) \
    object(Keychain: "t.keychain_hash = " + Keychain::hash_)
struct KeychainView
{
    #pragma db column(Account::name_)
    std::string account_name;

    #pragma db column(Keychain::name_)
    std::string keychain_name;
};

#pragma db view \
    object(TxOut) \
    object(TxIn: TxOut::spent_) \
    object(Tx = parent_tx: TxOut::tx_) \
    object(Tx = child_tx: TxIn::tx_)
struct TxParentChildView
{
    #pragma db column(parent_tx::id_)
    unsigned long parent_id;

    #pragma db column(parent_tx::hash_)
    bytes_t parent_hash;

    #pragma db column(child_tx::id_)
    unsigned long child_id;

    #pragma db column(child_tx::hash_)
    bytes_t child_hash;

    #pragma db column(TxOut::id_)
    unsigned long txout_id;

    #pragma db column(TxIn::id_)
    unsigned long txin_id;
}; 

#pragma db view \
    object(TxOut) \
    object(Tx: TxOut::tx_) \
    object(SigningScript: TxOut::signingscript_) \
    table("Account_signingscripts" = "t": "t.signingscript_id = " + SigningScript::id_) \
    object(Account: "t.account_id = " + Account::id_)
struct TxOutView
{
    #pragma db column(Account::id_)
    unsigned long account_id;

    #pragma db column(Account::name_)
    std::string account_name;

    #pragma db column(SigningScript::id_)
    unsigned long signingscript_id;

    #pragma db column(SigningScript::label_)
    std::string signingscript_label;

    #pragma db column(SigningScript::status_)
    SigningScript::status_t signingscript_status;

    #pragma db column(SigningScript::txinscript_)
    bytes_t signingscript_txinscript;

    #pragma db column(TxOut::script_)
    bytes_t script;

    #pragma db column(TxOut::value_)
    uint64_t value;

    #pragma db column(Tx::hash_)
    bytes_t txhash;

    #pragma db column(Tx::timestamp_)
    uint32_t txtimestamp;

    #pragma db column(Tx::status_)
    Tx::status_t txstatus;

    #pragma db column(TxOut::txindex_)
    uint32_t txindex;    
};

#pragma db view \
    object(Tx) \
    table("MerkleBlock_hashes" = "t": "t.value = " + Tx::hash_) \
    object(MerkleBlock: "t.object_id = " + MerkleBlock::id_) \
    object(BlockHeader: MerkleBlock::blockheader_)
struct ConfirmedTxView
{
    #pragma db column(Tx::id_)
    unsigned long tx_id;

    #pragma db column(Tx::hash_)
    bytes_t txhash;

    #pragma db column(MerkleBlock::id_)
    unsigned long merkleblock_id;

    #pragma db column(BlockHeader::id_)
    unsigned long blockheader_id;

    #pragma db column(BlockHeader::hash_)
    bytes_t blockhash;

    #pragma db column(BlockHeader::height_)
    uint32_t blockheight;    
};
 
#pragma db view \
    object(SigningScript) \
    object(Account: SigningScript::account_)
struct SigningScriptView
{
    #pragma db column(Account::id_)
    unsigned long account_id;

    #pragma db column(Account::name_)
    std::string account_name;

    #pragma db column(SigningScript::id_)
    unsigned long id;

    #pragma db column(SigningScript::label_)
    std::string label;

    #pragma db column(SigningScript::status_)
    SigningScript::status_t status;

    #pragma db column(SigningScript::txinscript_)
    bytes_t txinscript;

    #pragma db column(SigningScript::txoutscript_)
    bytes_t txoutscript;
};

#pragma db view \
    object(SigningScript) \
    object(Account: SigningScript::account_)
struct ScriptCountView
{
    #pragma db column("count(" + SigningScript::id_ + ")")
    std::size_t count;
};

#pragma db view \
    object(TxOut) \
    object(Tx: TxOut::tx_) \
    object(SigningScript: TxOut::signingscript_) \
    table("Account_signingscripts" = "t": "t.signingscript_id = " + SigningScript::id_) \
    object(Account: "t.account_id = " + Account::id_)
struct BalanceView
{
    #pragma db column("sum(" + TxOut::value_ + ")")
    uint64_t balance;
};

#pragma db view \
    object(BlockHeader)
struct BestHeightView
{
    #pragma db column("max(" + BlockHeader::height_ + ")")
    uint32_t best_height;    
};

#pragma db view \
    object(Account)
struct FirstAccountTimeCreatedView
{
    #pragma db column("min(" + Account::time_created_ + ")")
    uint32_t time_created;
};

#pragma db view \
    object(TxOut) \
    object(Account: TxOut::account_id_ == Account::id_) \
    object(Tx: TxOut::tx_) \
    object(ScriptTag: TxOut::script_ == ScriptTag::txoutscript_) \
    object(BlockHeader: Tx::blockheader_)
struct AccountTxOutView
{
    #pragma db column(Account::id_)
    unsigned long account_id;

    #pragma db column(Account::name_)
    std::string account_name;

    #pragma db column(TxOut::script_)
    bytes_t script;

    #pragma db column(TxOut::value_)
    uint64_t value;

    #pragma db column(TxOut::type_)
    TxOut::type_t type;

    #pragma db column(ScriptTag::id_)
    odb::nullable<unsigned long> scripttag_id;

    #pragma db column(ScriptTag::description_)
    odb::nullable<std::string> description;

    #pragma db column(Tx::id_)
    unsigned long txid;

    #pragma db column(Tx::hash_)
    bytes_t txhash;

    #pragma db column(Tx::timestamp_)
    uint32_t txtimestamp;

    #pragma db column(Tx::status_)
    Tx::status_t txstatus;

    #pragma db column(Tx::have_fee_)
    bool have_fee;

    #pragma db column(Tx::fee_)
    uint64_t fee;

    #pragma db column(TxOut::txindex_)
    uint32_t txindex;

    #pragma db column(BlockHeader::height_)
    odb::nullable<uint32_t> height;

    #pragma db column(BlockHeader::timestamp_)
    odb::nullable<uint32_t> block_timestamp;

    #pragma db column(BlockHeader::hash_)
    odb::nullable<bytes_t> block_hash;
};

}
}

#endif // _COIN_VAULT_DB_HXX
