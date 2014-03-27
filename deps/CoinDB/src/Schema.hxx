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

#include <stdutils/stringutils.h>

#include <odb/core.hxx>
#include <odb/nullable.hxx>
#include <odb/database.hxx>

#include <memory>

#include <logger.h>

// support for boost serialization
#include <boost/serialization/version.hpp>
#include <boost/serialization/split_member.hpp>

#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/set.hpp>

#pragma db namespace session
namespace CoinDB
{

typedef odb::nullable<unsigned long> null_id_t;

#pragma db value(bytes_t) type("BLOB")

////////////////////
// SCHEMA VERSION //
////////////////////

const uint32_t SCHEMA_VERSION = 3;

#pragma db object pointer(std::shared_ptr)
class Version
{
public:
    Version(uint32_t version = SCHEMA_VERSION) : version_(version) { }

    unsigned long id() const { return id_; }

    void version(uint32_t version) { version_ = version; }
    uint32_t version() const { return version_; }

private:
    friend class odb::access;

    #pragma db id auto
    unsigned long id_;

    uint32_t version_;
};


////////////////////////////
// KEYCHAINS AND ACCOUNTS //
////////////////////////////

class Account;
class AccountBin;
class SigningScript;

#pragma db object pointer(std::shared_ptr)
class Keychain : public std::enable_shared_from_this<Keychain>
{
public:
    Keychain() { }
    Keychain(const std::string& name, const secure_bytes_t& entropy, const secure_bytes_t& lock_key = secure_bytes_t(), const bytes_t& salt = bytes_t()); // Creates a new root keychain
    Keychain(const Keychain& source)
        : name_(source.name_), depth_(source.depth_), parent_fp_(source.parent_fp_), child_num_(source.child_num_), pubkey_(source.pubkey_), chain_code_(source.chain_code_), chain_code_ciphertext_(source.chain_code_ciphertext_), chain_code_salt_(source.chain_code_salt_), privkey_(source.privkey_), privkey_ciphertext_(source.privkey_ciphertext_), privkey_salt_(source.privkey_salt_), parent_(source.parent_), derivation_path_(source.derivation_path_) { }

    Keychain& operator=(const Keychain& source);

    std::shared_ptr<Keychain> get_shared_ptr() { return shared_from_this(); }

    unsigned int id() const { return id_; }

    std::string name() const { return name_; }
    void name(const std::string& name) { name_ = name; }

    std::shared_ptr<Keychain> root() { return (parent_ ? parent_->root() : shared_from_this()); }
    std::shared_ptr<Keychain> parent() const { return parent_; }
    std::shared_ptr<Keychain> child(uint32_t i, bool get_private = false);

    const std::vector<uint32_t>& derivation_path() const { return derivation_path_; }

    bool isPrivate() const { return (!privkey_.empty()) || (!privkey_ciphertext_.empty()); }

    // Lock keys must be set before persisting
    void setPrivateKeyLockKey(const secure_bytes_t& lock_key = secure_bytes_t(), const bytes_t& salt = bytes_t());
    void setChainCodeLockKey(const secure_bytes_t& lock_key = secure_bytes_t(), const bytes_t& salt = bytes_t());

    void lockPrivateKey() const;
    void lockChainCode() const;
    void lockAll() const;

    bool isPrivateKeyLocked() const;
    bool isChainCodeLocked() const;

    bool unlockPrivateKey(const secure_bytes_t& lock_key) const;
    bool unlockChainCode(const secure_bytes_t& lock_key) const;

    secure_bytes_t getSigningPrivateKey(uint32_t i, const std::vector<uint32_t>& derivation_path = std::vector<uint32_t>()) const;
    bytes_t getSigningPublicKey(uint32_t i, const std::vector<uint32_t>& derivation_path = std::vector<uint32_t>()) const;

    uint32_t depth() const { return depth_; }
    uint32_t parent_fp() const { return parent_fp_; }
    uint32_t child_num() const { return child_num_; }
    const bytes_t& pubkey() const { return pubkey_; }
    secure_bytes_t privkey() const;
    secure_bytes_t chain_code() const;

    const bytes_t& chain_code_ciphertext() const { return chain_code_ciphertext_; }
    const bytes_t& chain_code_salt() const { return chain_code_salt_; }

    void importPrivateKey(const Keychain& source);

    // hash = ripemd160(sha256(pubkey + chain_code))
    const bytes_t& hash() const { return hash_; }

    secure_bytes_t extkey(bool get_private = false) const;

    void clearPrivateKey();

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
    mutable secure_bytes_t chain_code_;
    bytes_t chain_code_ciphertext_;
    bytes_t chain_code_salt_;

    #pragma db transient
    mutable secure_bytes_t privkey_;
    bytes_t privkey_ciphertext_;
    bytes_t privkey_salt_;

    #pragma db null
    std::shared_ptr<Keychain> parent_;
    std::vector<uint32_t> derivation_path_;

    #pragma db value_not_null inverse(parent_)
    std::vector<std::weak_ptr<Keychain>> children_;

    bytes_t hash_;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & name_;
        ar & hash_;
        ar & depth_;
        ar & parent_fp_;
        ar & child_num_;
        ar & pubkey_;
        ar & chain_code_ciphertext_;
        ar & chain_code_salt_;
        ar & privkey_ciphertext_;
        ar & privkey_salt_;
    }
};

typedef std::set<std::shared_ptr<Keychain>> KeychainSet;

inline Keychain::Keychain(const std::string& name, const secure_bytes_t& entropy, const secure_bytes_t& lock_key, const bytes_t& salt)
    : name_(name)
{
    if (name.empty() || name[0] == '@') throw std::runtime_error("Invalid keychain name.");

    Coin::HDSeed hdSeed(entropy);
    Coin::HDKeychain hdKeychain(hdSeed.getMasterKey(), hdSeed.getMasterChainCode());

    depth_ = (uint32_t)hdKeychain.depth();
    parent_fp_ = hdKeychain.parent_fp();
    child_num_ = hdKeychain.child_num();
    chain_code_ = hdKeychain.chain_code();
    privkey_ = hdKeychain.key();
    pubkey_ = hdKeychain.pubkey();
    hash_ = hdKeychain.full_hash();

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

inline std::shared_ptr<Keychain> Keychain::child(uint32_t i, bool get_private)
{
    if (get_private && !isPrivate()) throw std::runtime_error("Cannot get private child from public keychain.");
    if (chain_code_.empty()) throw std::runtime_error("Chain code is locked.");
    if (get_private)
    {
        if (privkey_.empty()) throw std::runtime_error("Private key is locked.");
        Coin::HDKeychain hdkeychain(privkey_, chain_code_, child_num_, parent_fp_, depth_);
        hdkeychain = hdkeychain.getChild(i);
        std::shared_ptr<Keychain> child(new Keychain());;
        child->parent_ = get_shared_ptr();
        child->privkey_ = hdkeychain.privkey();
        child->pubkey_ = hdkeychain.pubkey();
        child->chain_code_ = hdkeychain.chain_code();
        child->child_num_ = hdkeychain.child_num();
        child->parent_fp_ = hdkeychain.parent_fp();
        child->depth_ = hdkeychain.depth();
        child->hash_ = hdkeychain.full_hash();
        child->derivation_path_ = derivation_path_;
        child->derivation_path_.push_back(i);
        return child;
    }
    else
    {
        Coin::HDKeychain hdkeychain(pubkey_, chain_code_, child_num_, parent_fp_, depth_);
        hdkeychain = hdkeychain.getChild(i);
        std::shared_ptr<Keychain> child(new Keychain());;
        child->parent_ = get_shared_ptr();
        child->pubkey_ = hdkeychain.pubkey();
        child->chain_code_ = hdkeychain.chain_code();
        child->child_num_ = hdkeychain.child_num();
        child->parent_fp_ = hdkeychain.parent_fp();
        child->depth_ = hdkeychain.depth();
        child->hash_ = hdkeychain.full_hash();
        child->derivation_path_ = derivation_path_;
        child->derivation_path_.push_back(i);
        return child;
    }
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

inline void Keychain::lockPrivateKey() const
{
    privkey_.clear();
}

inline void Keychain::lockChainCode() const 
{
    chain_code_.clear();
}

inline void Keychain::lockAll() const
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

inline bool Keychain::unlockPrivateKey(const secure_bytes_t& lock_key) const
{
    if (!isPrivate()) throw std::runtime_error("Cannot unlock the private key of a public keychain.");
    if (!privkey_.empty()) return true; // Already unlocked

    // TODO: decrypt
    privkey_ = privkey_ciphertext_;
    return true;
}

inline bool Keychain::unlockChainCode(const secure_bytes_t& lock_key) const
{
    if (!chain_code_.empty()) return true; // Already unlocked

    // TODO: decrypt
    chain_code_ = chain_code_ciphertext_;
    return true;
}

inline secure_bytes_t Keychain::getSigningPrivateKey(uint32_t i, const std::vector<uint32_t>& derivation_path) const
{
    if (!isPrivate()) throw std::runtime_error("Cannot get a private signing key from public keychain.");
    if (privkey_.empty()) throw std::runtime_error("Private key is locked.");
    if (chain_code_.empty()) throw std::runtime_error("Chain code is locked.");

    // Remove initial zero from privkey.
    // TODO: Don't store the zero in the first place.
    secure_bytes_t stripped_privkey(privkey_.begin() + 1, privkey_.end());
    Coin::HDKeychain hdkeychain(stripped_privkey, chain_code_, child_num_, parent_fp_, depth_);
    for (auto k: derivation_path) { hdkeychain = hdkeychain.getChild(k); }
    return hdkeychain.getPrivateSigningKey(i);
}

inline bytes_t Keychain::getSigningPublicKey(uint32_t i, const std::vector<uint32_t>& derivation_path) const
{
    if (chain_code_.empty()) throw std::runtime_error("Chain code is locked.");

    Coin::HDKeychain hdkeychain(pubkey_, chain_code_, child_num_, parent_fp_, depth_);
    for (auto k: derivation_path) { hdkeychain = hdkeychain.getChild(k); }
    return hdkeychain.getPublicSigningKey(i);
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

inline void Keychain::importPrivateKey(const Keychain& source)
{
    privkey_ciphertext_ = source.privkey_ciphertext_;
    privkey_salt_ = source.privkey_salt_;
}

inline secure_bytes_t Keychain::extkey(bool get_private) const
{
    if (get_private && !isPrivate()) throw std::runtime_error("Cannot get private extkey of a public keychain.");
    if (get_private && privkey_.empty()) throw std::runtime_error("Keychain private key is locked.");
    if (chain_code_.empty()) throw std::runtime_error("Keychain chain code is locked.");

    secure_bytes_t key = get_private ? privkey_ : pubkey_;
    return Coin::HDKeychain(key, chain_code_, child_num_, parent_fp_, depth_).extkey();
}

inline void Keychain::clearPrivateKey()
{
    privkey_.clear();
    privkey_ciphertext_.clear();
    privkey_salt_.clear();
}


#pragma db object pointer(std::shared_ptr)
class Key
{
public:
    Key(const std::shared_ptr<Keychain>& keychain, uint32_t index);

    unsigned long id() const { return id_; }
    const bytes_t& pubkey() const { return pubkey_; }
    secure_bytes_t privkey() const;
    secure_bytes_t try_privkey() const;
    bool isPrivate() const { return is_private_; }


    std::shared_ptr<Keychain> root_keychain() const { return root_keychain_; }
    std::vector<uint32_t> derivation_path() const { return derivation_path_; }
    uint32_t index() const { return index_; }

    void updatePrivate() { is_private_ = root_keychain_->isPrivate(); }

private:
    friend class odb::access;

    Key() { }

    #pragma db id auto
    unsigned long id_;

    #pragma db value_not_null
    std::shared_ptr<Keychain> root_keychain_;
    std::vector<uint32_t> derivation_path_;
    uint32_t index_;

    bytes_t pubkey_;
    bool is_private_;
};

typedef std::vector<std::shared_ptr<Key>> KeyVector;

inline Key::Key(const std::shared_ptr<Keychain>& keychain, uint32_t index)
{
    root_keychain_ = keychain->root();
    derivation_path_ = keychain->derivation_path();
    index_ = index;

    pubkey_ = keychain->getSigningPublicKey(index_);
    updatePrivate();
}

inline secure_bytes_t Key::privkey() const
{
    if (!is_private_ || root_keychain_->isPrivateKeyLocked() || root_keychain_->isChainCodeLocked()) return secure_bytes_t();
    return root_keychain_->getSigningPrivateKey(index_, derivation_path_);
}

// and a version that throws exceptions
inline secure_bytes_t Key::try_privkey() const
{
    if (!is_private_) throw std::runtime_error("Key::privkey - cannot get private key from nonprivate key object.");
    if (root_keychain_->isPrivateKeyLocked()) throw std::runtime_error("Key::privkey - private key is locked.");
    if (root_keychain_->isChainCodeLocked()) throw std::runtime_error("Key::privkey - chain code is locked.");

    return root_keychain_->getSigningPrivateKey(index_, derivation_path_);
}


const std::string CHANGE_BIN_NAME = "@change";
const std::string DEFAULT_BIN_NAME = "@default";

#pragma db object pointer(std::shared_ptr)
class AccountBin : public std::enable_shared_from_this<AccountBin>
{
public:
    // Note: index 0 is reserved for subaccounts but vector indices are 0-indexed so an offset of 1 is required.
    enum
    {
        CHANGE_INDEX = 1,
        DEFAULT_INDEX,
        FIRST_CUSTOM_INDEX
    };

    AccountBin() { }
    AccountBin(std::shared_ptr<Account> account, uint32_t index, const std::string& name);
    AccountBin(const AccountBin& source) : account_(source.account_), index_(source.index_), name_(source.name_), script_count_(source.script_count_), next_script_index_(source.next_script_index_), keychains_(source.keychains_) { }

    unsigned long id() const { return id_; }

    void account(std::shared_ptr<Account> account) { account_ = account; }
    std::shared_ptr<Account> account() const { return account_.lock(); }

    uint32_t index() const { return index_; }

    void name(const std::string& name) { name_ = name; }
    std::string name() const { return name_; }

    uint32_t script_count() const { return script_count_; }
    uint32_t next_script_index() const { return next_script_index_; }

    std::shared_ptr<SigningScript> newSigningScript(const std::string& label = "");
    void markSigningScriptIssued(uint32_t script_index);

    bool loadKeychains(bool get_private = false);
    KeychainSet keychains() const { return keychains_; }

    bool isChange() const { return index_ == CHANGE_INDEX; }
    bool isDefault() const { return index_ == DEFAULT_INDEX; }

private:
    friend class odb::access;

    #pragma db id auto
    unsigned long id_;

    #pragma db value_not_null
    std::weak_ptr<Account> account_;
    uint32_t index_;
    std::string name_;

    uint32_t script_count_;
    uint32_t next_script_index_; // index of next script in pool that will be issued

    #pragma db transient
    KeychainSet keychains_;

    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive& ar, const unsigned int version) const
    {
        ar & name_;
        ar & index_;
        ar & next_script_index_;
//        ar & keychains_; // useful for exporting the account bin independently from account
    }
    template<class Archive>
    void load(Archive& ar, const unsigned int version)
    {
        ar & name_;
        ar & index_;
        ar & next_script_index_;
 //       ar & keychains_; // useful for importing the account bin independently from account
        script_count_ = 0;
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

typedef std::vector<std::shared_ptr<AccountBin>> AccountBinVector;

// Immutable object containng keychain and bin names as strings
class AccountInfo
{
public:
    AccountInfo(
        unsigned long id,
        const std::string& name,
        unsigned int minsigs,
        const std::vector<std::string>& keychain_names,
        uint32_t unused_pool_size,
        uint32_t time_created,
        const std::vector<std::string>& bin_names
    ) :
        id_(id),
        name_(name),
        minsigs_(minsigs),
        keychain_names_(keychain_names),
        unused_pool_size_(unused_pool_size),
        time_created_(time_created),
        bin_names_(bin_names)
    {
        std::sort(keychain_names_.begin(), keychain_names_.end());
    }

    AccountInfo(const AccountInfo& source) :
        id_(source.id_),
        name_(source.name_),
        minsigs_(source.minsigs_),
        keychain_names_(source.keychain_names_),
        unused_pool_size_(source.unused_pool_size_),
        time_created_(source.time_created_),
        bin_names_(source.bin_names_)
    { }

    AccountInfo& operator=(const AccountInfo& source)
    {
        id_ = source.id_;
        name_ = source.name_;
        minsigs_ = source.minsigs_;
        keychain_names_ = source.keychain_names_;
        unused_pool_size_ = source.unused_pool_size_;
        time_created_ = source.time_created_;
        bin_names_ = source.bin_names_;
        return *this;
    }

    unsigned int                        id() const { return id_; }
    const std::string&                  name() const { return name_; }
    unsigned int                        minsigs() const { return minsigs_; }
    const std::vector<std::string>&     keychain_names() const { return keychain_names_; }
    uint32_t                            unused_pool_size() const { return unused_pool_size_; }
    uint32_t                            time_created() const { return time_created_; }
    const std::vector<std::string>&     bin_names() const { return bin_names_; }

private:
    unsigned long               id_;
    std::string                 name_;
    unsigned int                minsigs_;
    std::vector<std::string>    keychain_names_;
    uint32_t                    unused_pool_size_;
    uint32_t                    time_created_;
    std::vector<std::string>    bin_names_;
};

#pragma db object pointer(std::shared_ptr)
class Account : public std::enable_shared_from_this<Account>
{
public:
    Account() { }
    Account(const std::string& name, unsigned int minsigs, const KeychainSet& keychains, uint32_t unused_pool_size = 25, uint32_t time_created = time(NULL))
        : name_(name), minsigs_(minsigs), keychains_(keychains), unused_pool_size_(unused_pool_size), time_created_(time_created)
    {
        // TODO: Use exception classes
        if (name_.empty() || name[0] == '@') throw std::runtime_error("Invalid account name.");
        if (keychains_.size() > 15) throw std::runtime_error("Account can use at most 15 keychains.");
        if (minsigs > keychains_.size()) throw std::runtime_error("Account minimum signatures cannot exceed number of keychains.");
        if (minsigs < 1) throw std::runtime_error("Account must require at least one signature.");

        updateHash();
    }

    void updateHash();

    AccountInfo accountInfo() const;

    unsigned long id() const { return id_; }

    void name(const std::string& name) { name_ = name; }
    const std::string& name() const { return name_; }
    unsigned int minsigs() const { return minsigs_; }


    void keychains(const KeychainSet& keychains) { keychains_ = keychains; }
    KeychainSet keychains() const { return keychains_; }

    uint32_t unused_pool_size() const { return unused_pool_size_; }
    uint32_t time_created() const { return time_created_; }
    const bytes_t& hash() const { return hash_; }
    AccountBinVector bins() const { return bins_; }

    std::shared_ptr<AccountBin> addBin(const std::string& name);

    uint32_t bin_count() const { return bins_.size(); }

private:
    friend class odb::access;

    #pragma db id auto
    unsigned long id_;

    #pragma db unique
    std::string name_;
    unsigned int minsigs_;
    KeychainSet keychains_;
    uint32_t unused_pool_size_; // how many unused scripts we want in our lookahead
    uint32_t time_created_;
    bytes_t hash_; // ripemd160(sha256(data)) where data = concat(first byte(minsigs), keychain hash 1, keychain hash 2, ...) and keychain hashes are sorted lexically

    #pragma db value_not_null inverse(account_)
    AccountBinVector bins_;

    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive& ar, const unsigned int version) const
    {
        ar & name_;
        ar & minsigs_;

        uint32_t n = keychains_.size();
        ar & n;
        for (auto& keychain: keychains_)    { ar & *keychain; }

        ar & unused_pool_size_;
        ar & time_created_;

        n = bins_.size();
        ar & n;
        for (auto& bin: bins_)              { ar & *bin; }
    }
    template<class Archive>
    void load(Archive& ar, const unsigned int version)
    {
        ar & name_;
        ar & minsigs_;

        uint32_t n;
        ar & n;
        keychains_.clear();
        for (uint32_t i = 0; i < n; i++)
        {
            std::shared_ptr<Keychain> keychain(new Keychain());
            ar & *keychain;
            keychains_.insert(keychain);
        }

        ar & unused_pool_size_;
        ar & time_created_;

        // TODO: validate bins
        ar & n;
        bins_.clear();
        for (uint32_t i = 0; i < n; i++)
        {
            std::shared_ptr<AccountBin> bin(new AccountBin());
            ar & *bin;
            bin->account(shared_from_this());
            bins_.push_back(bin);
        }

        updateHash();
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

inline void Account::updateHash()
{
    std::vector<bytes_t> keychain_hashes;
    for (auto& keychain: keychains_) { keychain_hashes.push_back(keychain->hash()); }
    std::sort(keychain_hashes.begin(), keychain_hashes.end());

    uchar_vector data;
    data.push_back((unsigned char)minsigs_);
    for (auto& keychain_hash: keychain_hashes) { data += keychain_hash; }

    hash_ = ripemd160(sha256(data));
}

inline AccountInfo Account::accountInfo() const
{
    std::vector<std::string> keychain_names;
    for (auto& keychain: keychains_) { keychain_names.push_back(keychain->name()); }

    std::vector<std::string> bin_names;
    for (auto& bin: bins_) { bin_names.push_back(bin->name()); }

    return AccountInfo(id_, name_, minsigs_, keychain_names, unused_pool_size_, time_created_, bin_names);
}

inline std::shared_ptr<AccountBin> Account::addBin(const std::string& name)
{
    uint32_t index = bins_.size() + 1;
    std::shared_ptr<AccountBin> bin(new AccountBin(shared_from_this(), index, name));
    bins_.push_back(bin);
    return bin;
}

inline AccountBin::AccountBin(std::shared_ptr<Account> account, uint32_t index, const std::string& name)
    : account_(account), index_(index), name_(name), script_count_(0), next_script_index_(0)
{
    if (index == 0) throw std::runtime_error("Account bin index cannot be zero.");
    if (index == CHANGE_INDEX && name != CHANGE_BIN_NAME) throw std::runtime_error("Account bin index reserved for change.");
    if (index == DEFAULT_INDEX && name != DEFAULT_BIN_NAME) throw std::runtime_error("Account bin index reserved for default."); 
}

inline bool AccountBin::loadKeychains(bool get_private)
{
    if (!keychains_.empty()) return false;
    for (auto& keychain: account()->keychains())
    {
        std::shared_ptr<Keychain> child(keychain->child(index_, get_private));
        keychains_.insert(child);
    }
    return true;
}


#pragma db object pointer(std::shared_ptr)
class SigningScript : public std::enable_shared_from_this<SigningScript>
{
public:
    enum status_t
    {
        NONE        =  0,
        UNUSED      =  1,
        CHANGE      =  1 << 1,
        ISSUED      =  1 << 2,
        USED        =  1 << 3,
        ALL         = (1 << 4) - 1
    };
    static std::string getStatusString(int status)
    {
        std::vector<std::string> flags;
        if (status & UNUSED) flags.push_back("UNUSED");
        if (status & CHANGE) flags.push_back("CHANGE");
        if (status & ISSUED) flags.push_back("ISSUED");
        if (status & USED) flags.push_back("USED");
        if (flags.empty()) return "NONE";

        return stdutils::delimited_list(flags, " | ");
    }

    static std::vector<status_t> getStatusFlags(int status)
    {
        std::vector<status_t> flags;
        if (status & UNUSED) flags.push_back(UNUSED);
        if (status & CHANGE) flags.push_back(CHANGE);
        if (status & ISSUED) flags.push_back(ISSUED);
        if (status & USED) flags.push_back(USED);
        return flags;
    }

    SigningScript(std::shared_ptr<AccountBin> account_bin, uint32_t index, const std::string& label = "", status_t status = UNUSED);
    SigningScript(std::shared_ptr<AccountBin> account_bin, uint32_t index, const bytes_t& txinscript, const bytes_t& txoutscript, const std::string& label = "", status_t status = UNUSED)
        : account_(account_bin->account()), account_bin_(account_bin), index_(index), label_(label), status_(status), txinscript_(txinscript), txoutscript_(txoutscript) { }

    unsigned long id() const { return id_; }
    void label(const std::string& label) { label_ = label; }
    const std::string label() const { return label_; }

    void status(status_t status);
    status_t status() const { return status_; }

    const bytes_t& txinscript() const { return txinscript_; }
    const bytes_t& txoutscript() const { return txoutscript_; }

    std::shared_ptr<Account> account() const { return account_; }

    // 0 is reserved for subaccounts, 1 is reserved for change addresses, 2 is reserved for the default bin.
    std::shared_ptr<AccountBin> account_bin() const { return account_bin_; }

    uint32_t index() const { return index_; }

    KeyVector& keys() { return keys_; }

private:
    friend class odb::access;
    SigningScript() { }

    #pragma db id auto
    unsigned long id_;

    #pragma db value_not_null
    std::shared_ptr<Account> account_;

    #pragma db value_not_null
    std::shared_ptr<AccountBin> account_bin_;
    uint32_t index_;

    std::string label_;
    status_t status_;

    bytes_t txinscript_; // unsigned (0 byte length placeholders are used for signatures)
    bytes_t txoutscript_;

    KeyVector keys_;
};

inline SigningScript::SigningScript(std::shared_ptr<AccountBin> account_bin, uint32_t index, const std::string& label, status_t status)
    : account_(account_bin->account()), account_bin_(account_bin), index_(index), label_(label), status_(status)
{
    account_bin_->loadKeychains();
    for (auto& keychain: account_bin_->keychains())
    {
        std::shared_ptr<Key> key(new Key(keychain, index));
        keys_.push_back(key);
    }

    // sort keys into canonical order
    std::sort(keys_.begin(), keys_.end(), [](std::shared_ptr<Key> key1, std::shared_ptr<Key> key2) { return key1->pubkey() < key2->pubkey(); });

    std::vector<bytes_t> pubkeys;
    for (auto& key: keys_) { pubkeys.push_back(key->pubkey()); }
    CoinQ::Script::Script script(CoinQ::Script::Script::PAY_TO_MULTISIG_SCRIPT_HASH, account_->minsigs(), pubkeys);
    txinscript_ = script.txinscript(CoinQ::Script::Script::EDIT);
    txoutscript_ = script.txoutscript();
}

inline void SigningScript::status(status_t status)
{
    status_ = status;
    if (status > UNUSED) account_bin_->markSigningScriptIssued(index_);
}

inline std::shared_ptr<SigningScript> AccountBin::newSigningScript(const std::string& label)
{
    std::shared_ptr<SigningScript> signingscript(new SigningScript(shared_from_this(), script_count_++, label));
    return signingscript;
}

inline void AccountBin::markSigningScriptIssued(uint32_t script_index)
{
    if (script_index >= next_script_index_)
        next_script_index_ = script_index + 1;
}

/////////////////////////////
// BLOCKS AND TRANSACTIONS //
/////////////////////////////

class Tx;

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
    const std::shared_ptr<BlockHeader>& blockheader() const { return blockheader_; }

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
    std::vector<bytes_t> hashes_;

    bytes_t flags_;
};

inline void MerkleBlock::fromCoinClasses(const Coin::MerkleBlock& merkleblock, uint32_t height)
{
    blockheader_ = std::shared_ptr<BlockHeader>(new BlockHeader(merkleblock.blockHeader, height));
    txcount_ = merkleblock.nTxs;
    hashes_.assign(merkleblock.hashes.begin(), merkleblock.hashes.end()); 
    for (auto& hash: hashes_) { std::reverse(hash.begin(), hash.end()); }
    flags_ = merkleblock.flags;
}

inline Coin::MerkleBlock MerkleBlock::toCoinClasses() const
{
    Coin::MerkleBlock merkleblock;
    merkleblock.blockHeader = blockheader_->toCoinClasses();
    merkleblock.nTxs = txcount_;
    merkleblock.hashes.assign(hashes_.begin(), hashes_.end());
    for (auto& hash: merkleblock.hashes) { std::reverse(hash.begin(), hash.end()); }
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

    void tx(std::shared_ptr<Tx> tx) { tx_ = tx; }
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

    #pragma db not_null
    std::weak_ptr<Tx> tx_;

    uint32_t txindex_;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & outhash_;
        ar & outindex_;
        ar & script_;
        ar & sequence_;
    }
};

typedef std::vector<std::shared_ptr<TxIn>> txins_t;

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
    enum status_t { NEITHER = 0, UNSPENT = 1, SPENT = 2, BOTH = 3 };
    static std::string getStatusString(int flags)
    {
        std::vector<std::string> str_flags;
        if (flags & UNSPENT) str_flags.push_back("UNSPENT");
        if (flags & SPENT) str_flags.push_back("SPENT");
        if (str_flags.empty()) return "NEITHER";
        return stdutils::delimited_list(str_flags, " | ");
    }

    static std::vector<status_t> getStatusFlags(int flags)
    {
        std::vector<status_t> vflags;
        if (flags & UNSPENT) vflags.push_back(UNSPENT);
        if (flags & SPENT) vflags.push_back(SPENT);
        return vflags;
    }

    TxOut() : status_(UNSPENT) { }
    TxOut(uint64_t value, const bytes_t& script)
        : value_(value), script_(script), status_(UNSPENT) { }

    // Constructor for change and transfers
    TxOut(uint64_t value, std::shared_ptr<SigningScript> signingscript);

    TxOut(const Coin::TxOut& coin_txout);
    TxOut(const bytes_t& raw);

    Coin::TxOut toCoinClasses() const;

    unsigned long id() const { return id_; }
    uint64_t value() const { return value_; }
    const bytes_t& script() const { return script_; }
    bytes_t raw() const;

    void tx(std::shared_ptr<Tx> tx) { tx_ = tx; }
    const std::shared_ptr<Tx> tx() const { return tx_.lock(); }

    void txindex(uint32_t txindex) { txindex_ = txindex; }
    uint32_t txindex() const { return txindex_; }

    void spent(std::shared_ptr<TxIn> spent);
    const std::shared_ptr<TxIn> spent() const { return spent_; }

    void sending_account(std::shared_ptr<Account> sending_account) { sending_account_ = sending_account; }
    const std::shared_ptr<Account> sending_account() const { return sending_account_; }

    void sending_label(const std::string& label) { sending_label_ = label; }
    const std::string& sending_label() const { return sending_label_; }

    const std::shared_ptr<Account> receiving_account() const { return receiving_account_; }

    void receiving_label(const std::string& label) { receiving_label_ = label; }
    const std::string& receiving_label() const { return receiving_label_; }

    const std::shared_ptr<AccountBin> account_bin() const { return account_bin_; }

    void signingscript(std::shared_ptr<SigningScript> signingscript);
    const std::shared_ptr<SigningScript> signingscript() const { return signingscript_; }

    status_t status() const { return status_; }

private:
    friend class odb::access;

    #pragma db id auto
    unsigned long id_;

    uint64_t value_;
    bytes_t script_;

    #pragma db not_null
    std::weak_ptr<Tx> tx_;
    uint32_t txindex_;

    #pragma db null
    std::shared_ptr<TxIn> spent_;

    #pragma db null
    std::shared_ptr<Account> sending_account_;
    std::string sending_label_;

    #pragma db null
    std::shared_ptr<Account> receiving_account_;
    std::string receiving_label_;

    // account_bin and signingscript are only for receiving account
    #pragma db null
    std::shared_ptr<AccountBin> account_bin_;

    #pragma db null
    std::shared_ptr<SigningScript> signingscript_;

    // status == SPENT if spent_ is not null. Otherwise UNSPENT.
    // Redundant but convenient for view queries.
    status_t status_;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & value_;
        ar & script_;
        ar & sending_label_;
        ar & receiving_label_;
    }
};

typedef std::vector<std::shared_ptr<TxOut>> txouts_t;

inline TxOut::TxOut(uint64_t value, std::shared_ptr<SigningScript> signingscript)
    : value_(value)
{
    this->signingscript(signingscript);
}

inline TxOut::TxOut(const Coin::TxOut& coin_txout)
    : value_(coin_txout.value), script_(coin_txout.scriptPubKey), status_(UNSPENT)
{
}

inline TxOut::TxOut(const bytes_t& raw)
{
    Coin::TxOut coin_txout(raw);
    value_ = coin_txout.value;
    script_ = coin_txout.scriptPubKey;
    status_ = UNSPENT;
}

inline void TxOut::spent(std::shared_ptr<TxIn> spent)
{
    spent_ = spent;
    status_ = spent ? SPENT : UNSPENT;
}

inline void TxOut::signingscript(std::shared_ptr<SigningScript> signingscript)
{
    if (!signingscript) throw std::runtime_error("TxOut::signingscript - null signingscript.");

    script_ = signingscript->txoutscript();
    receiving_account_ = signingscript->account();
    account_bin_ = signingscript->account_bin();
    signingscript_ = signingscript;
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
    enum status_t
    {
        NO_STATUS    =  0,
        UNSIGNED     =  1,      // still missing signatures
        UNSENT       =  1 << 1, // signed but not yet broadcast to network
        SENT         =  1 << 2, // sent to at least one peer but possibly not propagated
        RECEIVED     =  1 << 3, // received from at least one peer
        CONFLICTING  =  1 << 4, // unconfirmed and spends the same output as another transaction
        CANCELED     =  1 << 5, // either will never be broadcast or will never confirm
        CONFIRMED    =  1 << 6, // exists in blockchain
        ALL          = (1 << 7) - 1
    };
    /*
     * If status is UNSIGNED, remove all txinscripts before hashing so hash
     * is unchanged by adding signatures until fully signed. Then compute
     * normal hash and transition to one of the other states.
     *
     * The states are ordered such that transitions are generally from smaller values to larger values.
     * Blockchain reorgs are the exception where it's possible that a CONFIRMED state reverts to an
     * earlier state.
     */
    static std::string getStatusString(int status)
    {
        std::vector<std::string> flags;
        if (status & UNSIGNED) flags.push_back("UNSIGNED");
        if (status & UNSENT) flags.push_back("UNSENT");
        if (status & SENT) flags.push_back("SENT");
        if (status & RECEIVED) flags.push_back("RECEIVED");
        if (status & CONFLICTING) flags.push_back("CONFLICTING");
        if (status & CANCELED) flags.push_back("CANCELED");
        if (status & CONFIRMED) flags.push_back("CONFIRMED");
        if (flags.empty()) return "NO_STATUS";
        return stdutils::delimited_list(flags, " | ");
    }

    static std::vector<status_t> getStatusFlags(int status)
    {
        std::vector<status_t> flags;
        if (status & UNSIGNED) flags.push_back(UNSIGNED);
        if (status & UNSENT) flags.push_back(UNSENT);
        if (status & SENT) flags.push_back(SENT);
        if (status & RECEIVED) flags.push_back(RECEIVED);
        if (status & CONFLICTING) flags.push_back(CONFLICTING);
        if (status & CANCELED) flags.push_back(CANCELED);
        if (status & CONFIRMED) flags.push_back(CONFIRMED);
        return flags;
    }

    Tx(uint32_t version = 1, uint32_t locktime = 0, uint32_t timestamp = 0xffffffff, status_t status = RECEIVED)
        : version_(version), locktime_(locktime), timestamp_(timestamp), status_(status), have_fee_(false), fee_(0) { }

    void set(uint32_t version, const txins_t& txins, const txouts_t& txouts, uint32_t locktime, uint32_t timestamp = 0xffffffff, status_t status = RECEIVED);
    void set(Coin::Transaction coin_tx, uint32_t timestamp = 0xffffffff, status_t status = RECEIVED);
    void set(const bytes_t& raw, uint32_t timestamp = 0xffffffff, status_t status = RECEIVED);

    Coin::Transaction toCoinClasses() const;

    void setBlock(std::shared_ptr<BlockHeader> blockheader, uint32_t blockindex);

    unsigned long id() const { return id_; }
    uint32_t version() const { return version_; }
    const bytes_t& hash() const { return hash_; }
    bytes_t unsigned_hash() const { return unsigned_hash_; }
    txins_t txins() const { return txins_; }
    txouts_t txouts() const { return txouts_; }
    uint32_t locktime() const { return locktime_; }
    bytes_t raw() const;

    void timestamp(uint32_t timestamp) { timestamp_ = timestamp; }
    uint32_t timestamp() const { return timestamp_; }

    bool updateStatus(status_t status = NO_STATUS); // Will keep the status it already had if it didn't change and no parameter is passed. Returns true iff status changed.
    status_t status() const { return status_; }

    void fee(uint64_t fee) { have_fee_ = true; fee_ = fee; }
    uint64_t fee() const { return fee_; }

    bool have_fee() const { return have_fee_; }

    void block(std::shared_ptr<BlockHeader> header, uint32_t index) { blockheader_ = header, blockindex_ = index; }

    void blockheader(std::shared_ptr<BlockHeader> blockheader) { blockheader_ = blockheader; }
    std::shared_ptr<BlockHeader> blockheader() const { return blockheader_; }

    void shuffle_txins();
    void shuffle_txouts();

    unsigned int missingSigCount() const;
    std::set<bytes_t> missingSigPubkeys() const;

private:
    friend class odb::access;

    void fromCoinClasses(const Coin::Transaction& coin_tx);

    #pragma db id auto
    unsigned long id_;

    // hash stays empty until transaction is fully signed.
    bytes_t hash_;

    // We'll use the unsigned hash as a unique identifier to avoid malleability issues.
    #pragma db unique
    bytes_t unsigned_hash_;

    uint32_t version_;

    #pragma db value_not_null inverse(tx_)
    txins_t txins_;

    #pragma db value_not_null inverse(tx_)
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

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & version_;
        ar & txins_;
        ar & txouts_;
        ar & locktime_;
        ar & timestamp_; // only used for sorting in UI
    }
};

inline void Tx::set(uint32_t version, const txins_t& txins, const txouts_t& txouts, uint32_t locktime, uint32_t timestamp, status_t status)
{
    version_ = version;

    int i = 0;
    txins_.clear();
    for (auto& txin: txins)
    {
        txin->tx(shared_from_this());
        txin->txindex(i++);
        txins_.push_back(txin);
    }

    i = 0;
    txouts_.clear();
    for (auto& txout: txouts)
    {
        txout->tx(shared_from_this());
        txout->txindex(i++);
        txouts_.push_back(txout);
    }

    locktime_ = locktime;
    timestamp_ = timestamp;

    Coin::Transaction coin_tx = toCoinClasses();

    if (missingSigCount())  { status_ = UNSIGNED; }
    else                    { status_ = status; hash_ = coin_tx.getHashLittleEndian(); }

    coin_tx.clearScriptSigs();
    unsigned_hash_ = coin_tx.getHashLittleEndian();
}

inline void Tx::set(Coin::Transaction coin_tx, uint32_t timestamp, status_t status)
{
    LOGGER(trace) << "Tx::set - fromCoinClasses(coin_tx);" << std::endl;
    fromCoinClasses(coin_tx);

    timestamp_ = timestamp;

    if (missingSigCount())  { status_ = UNSIGNED; }
    else                    { status_ = status; hash_ = coin_tx.getHashLittleEndian(); }

    coin_tx.clearScriptSigs();
    unsigned_hash_ = coin_tx.getHashLittleEndian();
}

inline void Tx::set(const bytes_t& raw, uint32_t timestamp, status_t status)
{
    Coin::Transaction coin_tx(raw);
    fromCoinClasses(coin_tx);
    timestamp_ = timestamp;

    if (missingSigCount())  { status_ = UNSIGNED; }
    else                    { status_ = status; hash_ = coin_tx.getHashLittleEndian(); }

    coin_tx.clearScriptSigs();
    unsigned_hash_ = coin_tx.getHashLittleEndian();
}

inline bool Tx::updateStatus(status_t status /* = NO_STATUS */)
{
    // Tx is not signed.
    if (missingSigCount())
    {
        status_ = UNSIGNED;
        hash_ = bytes_t();
        return true;
    }

    // Tx status changed from unsigned to signed.
    if (status_ == UNSIGNED)
    {
        switch (status)
        {
        case NO_STATUS:
            status_ = UNSENT;
            break;

        case UNSIGNED:
            status_ = RECEIVED;
            break;

        default:
            status_ = status;
        }

        hash_ = toCoinClasses().getHashLittleEndian();
        return true;
    }

    // Only update the status if the new status is valid.
    if (status && status != UNSIGNED && status != status_)
    {
        status_ = status;
        return true;
    }

    return false;
}

inline Coin::Transaction Tx::toCoinClasses() const
{
    Coin::Transaction coin_tx;
    coin_tx.version = version_;
    for (auto& txin:  txins_) { coin_tx.inputs.push_back(txin->toCoinClasses()); }
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
    for (auto& coin_txin: coin_tx.inputs)
    {
        std::shared_ptr<TxIn> txin(new TxIn(coin_txin));
        txin->tx(shared_from_this());
        txin->txindex(i++);
        txins_.push_back(txin);
    }

    i = 0;
    txouts_.clear();
    for (auto& coin_txout: coin_tx.outputs)
    {
        std::shared_ptr<TxOut> txout(new TxOut(coin_txout));
        txout->tx(shared_from_this());
        txout->txindex(i++);
        txouts_.push_back(txout);
    }

    locktime_ = coin_tx.lockTime;
}

inline unsigned int Tx::missingSigCount() const
{
    // Assume for now all inputs belong to the same account.
    using namespace CoinQ::Script;
    unsigned int count = 0;
    for (auto& txin: txins_)
    {
        Script script(txin->script());
        unsigned int sigsneeded = script.sigsneeded();
        if (sigsneeded > count) count = sigsneeded;
    }
    return count;
}

inline std::set<bytes_t> Tx::missingSigPubkeys() const
{
    using namespace CoinQ::Script;
    std::set<bytes_t> pubkeys;
    for (auto& txin: txins_)
    {
        Script script(txin->script());
        std::vector<bytes_t> txinpubkeys = script.missingsigs();
        for (auto& txinpubkey: txinpubkeys) { pubkeys.insert(txinpubkey); }
    } 
    return pubkeys;
}

// Views
#pragma db view \
    object(AccountBin) \
    object(Account: AccountBin::account_)
struct AccountBinView
{
    #pragma db column(Account::id_)
    unsigned long account_id;
    #pragma db column(Account::name_)
    std::string account_name;

    #pragma db column(AccountBin::id_)
    unsigned long bin_id; 
    #pragma db column(AccountBin::name_)
    std::string bin_name;
};

#pragma db view \
    object(SigningScript) \
    object(Account: SigningScript::account_) \
    object(AccountBin: SigningScript::account_bin_)
struct SigningScriptView
{
    #pragma db column(Account::id_)
    unsigned long account_id;

    #pragma db column(Account::name_)
    std::string account_name;

    #pragma db column(AccountBin::id_)
    unsigned long account_bin_id;

    #pragma db column(AccountBin::name_)
    std::string account_bin_name;

    #pragma db column(SigningScript::id_)
    unsigned long id;

    #pragma db column(SigningScript::index_)
    uint32_t index;

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
    object(Account: SigningScript::account_) \
    object(AccountBin: SigningScript::account_bin_)
struct ScriptCountView
{
    #pragma db column("count(" + SigningScript::id_ + ")")
    uint32_t count;
};

#pragma db view \
    object(TxOut) \
    object(Tx: TxOut::tx_) \
    object(BlockHeader: Tx::blockheader_) \
    object(Account = sending_account: TxOut::sending_account_) \
    object(Account = receiving_account: TxOut::receiving_account_) \
    object(AccountBin: TxOut::account_bin_) \
    object(SigningScript: TxOut::signingscript_)
struct TxOutView
{
    #pragma db column(sending_account::id_)
    unsigned long sending_account_id;

    #pragma db column(sending_account::name_)
    std::string sending_account_name;

    #pragma db column(receiving_account::id_)
    unsigned long receiving_account_id;

    #pragma db column(receiving_account::name_)
    std::string receiving_account_name;

    #pragma db column(AccountBin::id_)
    unsigned long account_bin_id;

    #pragma db column(AccountBin::name_)
    std::string account_bin_name;

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

    #pragma db column(TxOut::status_)
    TxOut::status_t status;

    #pragma db column(Tx::unsigned_hash_)
    bytes_t tx_unsigned_hash;

    #pragma db column(Tx::hash_)
    bytes_t tx_hash;

    #pragma db column(Tx::timestamp_)
    uint32_t tx_timestamp;

    #pragma db column(Tx::status_)
    Tx::status_t tx_status;

    #pragma db column(TxOut::txindex_)
    uint32_t tx_index;

    #pragma db column(BlockHeader::height_)
    uint32_t block_height;
};

#pragma db view \
    object(TxOut) \
    object(Tx: TxOut::tx_) \
    object(BlockHeader: Tx::blockheader_) \
    object(Account: TxOut::receiving_account_) \
    object(AccountBin: TxOut::account_bin_) \
    object(SigningScript: TxOut::signingscript_)
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
struct HorizonTimestampView
{
    #pragma db column("min(" + Account::time_created_ + ")")
    uint32_t timestamp;
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
    bytes_t tx_hash;

    #pragma db column(MerkleBlock::id_)
    unsigned long merkleblock_id;

    #pragma db column(BlockHeader::id_)
    unsigned long blockheader_id;

    #pragma db column(BlockHeader::hash_)
    bytes_t block_hash;

    #pragma db column(BlockHeader::height_)
    uint32_t block_height;
};

}

#endif // COINDB_SCHEMA_HXX
