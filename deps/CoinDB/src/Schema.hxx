///////////////////////////////////////////////////////////////////////////////
//
// Schema.hxx
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.
//

// odb compiler complains about #pragma once
#ifndef COINDB_SCHEMA_HXX
#define COINDB_SCHEMA_HXX

#include <CoinQ_script.h>
#include <CoinQ_typedefs.h>

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

#pragma db namespace session
namespace CoinDB
{

typedef odb::nullable<unsigned long> null_id_t;

#pragma db value(bytes_t) type("BLOB")

class Tx;
class Account;
class SigningScript;
class ScriptTag;

const uint32_t SCHEMA_VERSION = 3;

// Vault schema version
#pragma db object pointer(std::shared_ptr)
class Version
{
public:
    Version() : version_(0) { }
    Version(uint32_t version) : version_(version) { }

    unsigned long id() const { return id_; }

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

// Keys, Accounts
#pragma db object pointer(std::shared_ptr)
class Keychain : public std::enable_shared_from_this<Keychain>
{
public:
    Keychain(std::shared_ptr<Keychain> parent = nullptr) : parent_(parent) { }
    Keychain(const std::string& name, const secure_bytes_t& entropy, const secure_bytes_t& lock_key = secure_bytes_t(), const bytes_t& salt = bytes_t()); // Creates a new root keychain
    Keychain(const std::string& name, std::shared_ptr<Keychain> parent = nullptr) : name_(name), parent_(parent) { }
    Keychain(const Keychain& source)
        : name_(source.name_), depth_(source.depth_), parent_fp_(source.parent_fp_), child_num_(source.child_num_), pubkey_(source.pubkey_), chain_code_(source.chain_code_), chain_code_ciphertext_(source.chain_code_ciphertext_), chain_code_salt_(source.chain_code_salt_), privkey_(source.privkey_), privkey_ciphertext_(source.privkey_ciphertext_), privkey_salt_(source.privkey_salt_), parent_(source.parent_), derivation_path_(source.derivation_path_) { }

    Keychain& operator=(const Keychain& source);

    unsigned int id() const { return id_; }

    std::string name() const { return name_; }
    void name(const std::string& name) { name_ = name; }

    std::shared_ptr<Keychain> parent() { return parent_; }
    std::shared_ptr<Keychain> root() { return (parent_ ? parent_->root() : shared_from_this()); }

    const std::vector<uint32_t>& getDerivationPath() const { return derivation_path_; }

    void createPrivate(const secure_bytes_t& privkey, const secure_bytes_t& chain_code, uint32_t child_num = 0, uint32_t parent_fp = 0, uint32_t depth = 0);
    void createPublic(const bytes_t& pubkey, const secure_bytes_t& chain_code, uint32_t child_num = 0, uint32_t parent_fp = 0, uint32_t depth = 0);

    bool isPrivate() const;

    // Lock keys must be set before persisting
    void setPrivateKeyLockKey(const secure_bytes_t& lock_key = secure_bytes_t(), const bytes_t& salt = bytes_t());
    void setChainCodeLockKey(const secure_bytes_t& lock_key = secure_bytes_t(), const bytes_t& salt = bytes_t());

    void lockPrivateKey();
    void lockChainCode();
    void lockAll();

    bool isPrivateKeyLocked() const;
    bool isChainCodeLocked() const;

    void unlockPrivateKey(const secure_bytes_t& lock_key);
    void unlockChainCode(const secure_bytes_t& lock_key);

    secure_bytes_t getSigningPrivateKey(uint32_t i, const std::vector<uint32_t>& derivation_path = std::vector<uint32_t>()) const;
    bytes_t getSigningPublicKey(uint32_t i) const;

    // TODO: getChildKeychain should be const
    Keychain getChildKeychain(uint32_t i, bool get_private = false);
    Keychain getChildKeychain(const std::vector<uint32_t>& v, bool get_private = false);

    uint32_t depth() const { return depth_; }
    uint32_t parent_fp() const { return parent_fp_; }
    uint32_t child_num() const { return child_num_; }
    const bytes_t& pubkey() const { return pubkey_; }
    secure_bytes_t privkey() const;
    secure_bytes_t chain_code() const;

    const bytes_t& chain_code_ciphertext() const { return chain_code_ciphertext_; }
    const bytes_t& chain_code_salt() const { return chain_code_salt_; }

    // hash = ripemd160(sha256(pubkey + chain_code))
    const bytes_t& hash() const { return hash_; }

    secure_bytes_t extkey(bool get_private = false) const;

private:
    friend class odb::access;

    #pragma db id auto
    unsigned long id_;

    #pragma db unique
    std::string name_;

    uint32_t depth_;
    uint32_t parent_fp_;
    uint32_t child_num_;
    bytes_t pubkey_;

    #pragma db transient
    secure_bytes_t chain_code_;
    bytes_t chain_code_ciphertext_;
    bytes_t chain_code_salt_;

    #pragma db transient
    secure_bytes_t privkey_; 
    bytes_t privkey_ciphertext_;
    bytes_t privkey_salt_;

    #pragma db null
    std::shared_ptr<Keychain> parent_;
    std::vector<uint32_t> derivation_path_;

    #pragma db value_not_null inverse(parent_)
    std::vector<std::weak_ptr<Keychain>> children_;

    bytes_t hash_;
};

inline Keychain::Keychain(const std::string& name, const secure_bytes_t& entropy, const secure_bytes_t& lock_key, const bytes_t& salt)
    : name_(name)
{
    Coin::HDSeed hdSeed(entropy);
    Coin::HDKeychain hdKeychain(hdSeed.getMasterKey(), hdSeed.getMasterChainCode());

    depth_ = (uint32_t)hdKeychain.depth();
    parent_fp_ = hdKeychain.parent_fp();
    child_num_ = hdKeychain.child_num();
    chain_code_ = hdKeychain.chain_code();
    privkey_ = hdKeychain.key();
    pubkey_ = hdKeychain.pubkey();
    hash_ = hdKeychain.hash();

    setPrivateKeyLockKey(lock_key, salt);
    setChainCodeLockKey(lock_key, salt);
}

inline Keychain& Keychain::operator=(const Keychain& source)
{
    depth_ = source.depth_;
    parent_fp_ = source.parent_fp_;
    child_num_ = source.child_num_;
    pubkey_ = source.pubkey_;

    chain_code_ = source.chain_code_;
    chain_code_ciphertext_ = source.chain_code_ciphertext_;
    chain_code_salt_ = source.chain_code_salt_;

    privkey_ = source.privkey_;
    privkey_ciphertext_ = source.privkey_ciphertext_;
    privkey_salt_ = source.privkey_salt_;

    parent_ = source.parent_;

    derivation_path_ = source.derivation_path_;

    uchar_vector_secure hashdata = pubkey_;
    hashdata += chain_code_;
    hash_ = ripemd160(sha256(hashdata));

    return *this;
}

#define CHECK_HDKEYCHAIN

inline void Keychain::createPrivate(const secure_bytes_t& privkey, const secure_bytes_t& chain_code, uint32_t child_num, uint32_t parent_fp, uint32_t depth)
{
    depth_ = depth;
    parent_fp_ = parent_fp;
    child_num_ = child_num;
    privkey_ = privkey;
    Coin::HDKeychain hdkeychain(privkey_, chain_code_, child_num_, parent_fp_, depth_);
    if (!hdkeychain.isPrivate()) throw std::runtime_error("Invalid private key.");

    pubkey_ = hdkeychain.pubkey();

    uchar_vector_secure hashdata = pubkey_;
    hashdata += chain_code_;
    hash_ = ripemd160(sha256(hashdata));
}

inline void Keychain::createPublic(const bytes_t& pubkey, const secure_bytes_t& chain_code, uint32_t child_num, uint32_t parent_fp, uint32_t depth)
{
    depth_ = depth;
    parent_fp_ = parent_fp;
    child_num_ = child_num;
    pubkey_ = pubkey;
#ifdef CHECK_HDKEYCHAIN
    Coin::HDKeychain hdkeychain(pubkey_, chain_code_, child_num_, parent_fp_, depth_);
    if (hdkeychain.isPrivate()) throw std::runtime_error("Invalid public key.");
#endif

    uchar_vector_secure hashdata = pubkey_;
    hashdata += chain_code_;
    hash_ = ripemd160(sha256(hashdata));
}

inline bool Keychain::isPrivate() const
{
    return (!privkey_.empty()) || (!privkey_ciphertext_.empty());
}

inline void Keychain::setPrivateKeyLockKey(const secure_bytes_t& /*lock_key*/, const bytes_t& salt)
{
    if (!isPrivate()) throw std::runtime_error("Cannot lock the private key of a public keychain.");
    if (privkey_.empty()) throw std::runtime_error("Key is locked.");

    // TODO: encrypt
    privkey_ciphertext_ = privkey_;
    privkey_salt_ = salt;
}

inline void Keychain::setChainCodeLockKey(const secure_bytes_t& /*lock_key*/, const bytes_t& salt)
{
    if (chain_code_.empty()) throw std::runtime_error("Chain code is locked.");

    // TODO: encrypt
    chain_code_ciphertext_ = chain_code_;
    chain_code_salt_ = salt;
}

inline void Keychain::lockPrivateKey()
{
    privkey_.clear();
}

inline void Keychain::lockChainCode()
{
    chain_code_.clear();
}

inline void Keychain::lockAll()
{
    lockPrivateKey();
    lockChainCode();
}

inline bool Keychain::isPrivateKeyLocked() const
{
    if (!isPrivate()) throw std::runtime_error("Keychain is not private.");
    return privkey_.empty();
}

inline bool Keychain::isChainCodeLocked() const
{
    return chain_code_.empty();
}

inline void Keychain::unlockPrivateKey(const secure_bytes_t& lock_key)
{
    if (!isPrivate()) throw std::runtime_error("Cannot unlock the private key of a public keychain.");
    if (!privkey_.empty()) return; // Already unlocked

    // TODO: decrypt
    privkey_ = privkey_ciphertext_;    
}

inline void Keychain::unlockChainCode(const secure_bytes_t& lock_key)
{
    if (chain_code_.empty()) return; // Already unlocked

    // TODO: decrypt
    chain_code_ = chain_code_ciphertext_;
}

inline secure_bytes_t Keychain::getSigningPrivateKey(uint32_t i, const std::vector<uint32_t>& derivation_path) const
{
    if (!isPrivate()) throw std::runtime_error("Cannot get a private signing key from public keychain.");
    if (privkey_.empty()) throw std::runtime_error("Private key is locked.");
    if (chain_code_.empty()) throw std::runtime_error("Chain code is locked.");

    Coin::HDKeychain hdkeychain(privkey_, chain_code_, child_num_, parent_fp_, depth_);
    return hdkeychain.getPrivateSigningKey(i);
}

inline bytes_t Keychain::getSigningPublicKey(uint32_t i) const
{
    if (chain_code_.empty()) throw std::runtime_error("Chain code is locked.");

    Coin::HDKeychain hdkeychain(pubkey_, chain_code_, child_num_, parent_fp_, depth_);
    return hdkeychain.getPublicSigningKey(i);
}

inline Keychain Keychain::getChildKeychain(uint32_t i, bool get_private)
{
    std::vector<uint32_t> v(1, i);
    return getChildKeychain(v, get_private);
}

inline Keychain Keychain::getChildKeychain(const std::vector<uint32_t>& derivation_path, bool get_private)
{
    if (get_private && !isPrivate()) throw std::runtime_error("Cannot get private extkey of a public keychain.");
    if (get_private && privkey_.empty()) throw std::runtime_error("Keychain private key is locked.");
    if (chain_code_.empty()) throw std::runtime_error("Keychain chain code is locked.");

    Keychain keychainNode(shared_from_this());;

    secure_bytes_t key = get_private ? privkey_ : pubkey_;
    Coin::HDKeychain hdkeychain(key, chain_code_, child_num_, parent_fp_, depth_);
    for (auto i: derivation_path) {
        keychainNode.derivation_path_.push_back(i);
        hdkeychain = hdkeychain.getChild(i);
    }
            
    if (get_private) {
        keychainNode.createPrivate(hdkeychain.privkey(), hdkeychain.chain_code(), hdkeychain.child_num(), hdkeychain.parent_fp(), hdkeychain.depth());
    }
    else {
        keychainNode.createPublic(hdkeychain.pubkey(), hdkeychain.chain_code(), hdkeychain.child_num(), hdkeychain.parent_fp(), hdkeychain.depth());
    }

    return keychainNode;        
}

inline secure_bytes_t Keychain::privkey() const
{
    if (!isPrivate()) throw std::runtime_error("Keychain is public.");
    if (privkey_.empty()) throw std::runtime_error("Keychain private key is locked.");
    return privkey_;
}

inline secure_bytes_t Keychain::chain_code() const
{
    if (chain_code_.empty()) throw std::runtime_error("Keychain chain code is locked.");
    return chain_code_;
}

inline secure_bytes_t Keychain::extkey(bool get_private) const
{
    if (get_private && !isPrivate()) throw std::runtime_error("Cannot get private extkey of a public keychain.");
    if (get_private && privkey_.empty()) throw std::runtime_error("Keychain private key is locked.");
    if (chain_code_.empty()) throw std::runtime_error("Keychain chain code is locked.");

    secure_bytes_t key = get_private ? privkey_ : pubkey_;
    return Coin::HDKeychain(key, chain_code_, child_num_, parent_fp_, depth_).extkey();
}

#pragma db object pointer(std::shared_ptr)
class Key
{
public:
    Key(const std::shared_ptr<Keychain>& keychain, uint32_t signing_key_index);

    unsigned long id() const { return id_; }
    bool is_private() const { return is_private_; }
    const bytes_t& pubkey() const { return pubkey_; }
    secure_bytes_t privkey() const;

    std::shared_ptr<Keychain> keychain() const { return keychain_; }

    bool isPrivateKeyLocked() const { return keychain_->isPrivateKeyLocked(); }
    bool isChainCodeLocked() const { return keychain_->isChainCodeLocked(); }

private:
    friend class odb::access;

    Key() { }

    #pragma db id auto
    unsigned long id_;

    #pragma db value_not_null
    std::shared_ptr<Keychain> keychain_;
    std::vector<uint32_t> derivation_path_;
    uint32_t signing_key_index_;

    bool is_private_;
    bytes_t pubkey_;

};

inline Key::Key(const std::shared_ptr<Keychain>& keychain, uint32_t signing_key_index)
{
    keychain_ = keychain;
    derivation_path_ = keychain_->getDerivationPath();
    signing_key_index_ = signing_key_index;

    is_private_ = keychain_->isPrivate();
    pubkey_ = keychain_->getSigningPublicKey(signing_key_index_);
}

inline secure_bytes_t Key::privkey() const
{
    if (!is_private_) throw std::runtime_error("Key::privkey - cannot get private key from nonprivate key object.");
    if (isPrivateKeyLocked()) throw std::runtime_error("Key::privkey - private key is locked.");
    if (isChainCodeLocked()) throw std::runtime_error("Key::privkey - chain code is locked.");

    return keychain_->getSigningPrivateKey(signing_key_index_, derivation_path_);
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
    std::shared_ptr<Account> account() const { return account_; }

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

    std::shared_ptr<Account> account_;
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
    typedef std::set<std::shared_ptr<Keychain>> keychains_t;
    Account(const std::string& name, unsigned int minsigs, const keychains_t& keychains, uint32_t unused_pool_size = 100, uint32_t time_created = time(NULL))
        : name_(name), minsigs_(minsigs), keychains_(keychains), time_created_(time_created), script_count_(0), unused_pool_size_(unused_pool_size) { }

    unsigned long id() const { return id_; }

    void name(const std::string& name) { name_ = name; }
    const std::string& name() const { return name_; }
    unsigned int minsigs() const { return minsigs_; }
    keychains_t& keychains() { return keychains_; }
    uint32_t time_created() const { return time_created_; }
    uint32_t script_count() const { return script_count_; }
    uint32_t unused_pool_size() const { return unused_pool_size_; }

    std::shared_ptr<SigningScript> newSigningScript();

private:
    friend class odb::access;

    Account() { }

    #pragma db id auto
    unsigned long id_;

    #pragma db unique
    std::string name_;
    unsigned int minsigs_;
    keychains_t keychains_;
    uint32_t time_created_;
    uint32_t script_count_;
    uint32_t unused_pool_size_; // how many unused scripts we want in our lookahead
};

inline std::shared_ptr<SigningScript> Account::newSigningScript()
{
    script_count_++;
    std::vector<bytes_t> pubkeys;
    for (auto& keychain: keychains_)
        pubkeys.push_back(keychain->getSigningPublicKey(script_count_));

    // sort keys into canonical order
    std::sort(pubkeys.begin(), pubkeys.end());

    CoinQ::Script::Script script(CoinQ::Script::Script::PAY_TO_MULTISIG_SCRIPT_HASH, minsigs_, pubkeys);
    std::shared_ptr<SigningScript> signingscript(new SigningScript(script.txinscript(CoinQ::Script::Script::EDIT), script.txoutscript()));
    signingscript->account(shared_from_this());
    return signingscript;
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
    table("Account_keychain_roots" = "t": "t.account_id = " + Account::id_) \
    object(Keychain: "t.keychain_root = " + Keychain::hash_)
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

#endif // COINDB_SCHEMA_HXX
