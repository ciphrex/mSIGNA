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

#include <memory>

#include <logger.h>

#pragma db namespace session
namespace CoinDB
{

const unsigned int SCHEMA_VERSION = 3;

class Account;
class AccountBin;
class SigningScript;

#pragma db object pointer(std::shared_ptr)
class Keychain : public std::enable_shared_from_this<Keychain>
{
public:
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

    void lockPrivateKey();
    void lockChainCode();
    void lockAll();

    bool isPrivateKeyLocked() const;
    bool isChainCodeLocked() const;

    void unlockPrivateKey(const secure_bytes_t& lock_key);
    void unlockChainCode(const secure_bytes_t& lock_key);

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

    // hash = ripemd160(sha256(pubkey + chain_code))
    const bytes_t& hash() const { return hash_; }

    secure_bytes_t extkey(bool get_private = false) const;

private:
    friend class odb::access;
    Keychain() { }

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

typedef std::set<std::shared_ptr<Keychain>> KeychainSet;

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

inline std::shared_ptr<Keychain> Keychain::child(uint32_t i, bool get_private)
{
    if (get_private && !isPrivate()) throw std::runtime_error("Cannot get private child from public keychain.");
    if (chain_code_.empty()) throw std::runtime_error("Chain code is locked.");
    if (get_private)
    {
        if (privkey_.empty()) throw std::runtime_error("Private key is locked.");
        Coin::HDKeychain hdkeychain(privkey_, chain_code_, child_num_, parent_fp_, depth_);
        hdkeychain.getChild(i);
        std::shared_ptr<Keychain> child(new Keychain());;
        child->parent_ = get_shared_ptr();
        child->privkey_ = hdkeychain.privkey();
        child->pubkey_ = hdkeychain.pubkey();
        child->chain_code_ = hdkeychain.chain_code();
        child->child_num_ = hdkeychain.child_num();
        child->parent_fp_ = hdkeychain.parent_fp();
        child->depth_ = hdkeychain.depth();
        child->hash_ = hdkeychain.hash();
        child->derivation_path_ = derivation_path_;
        child->derivation_path_.push_back(i);
        return child;
    }
    else
    {
        Coin::HDKeychain hdkeychain(pubkey_, chain_code_, child_num_, parent_fp_, depth_);
        hdkeychain.getChild(i);
        std::shared_ptr<Keychain> child(new Keychain());;
        child->parent_ = get_shared_ptr();
        child->pubkey_ = hdkeychain.pubkey();
        child->chain_code_ = hdkeychain.chain_code();
        child->child_num_ = hdkeychain.child_num();
        child->parent_fp_ = hdkeychain.parent_fp();
        child->depth_ = hdkeychain.depth();
        child->hash_ = hdkeychain.hash();
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
    Key(const std::shared_ptr<Keychain>& keychain, uint32_t index);

    unsigned long id() const { return id_; }
    const bytes_t& pubkey() const { return pubkey_; }
    secure_bytes_t privkey() const;
    bool isPrivate() const { return is_private_; }


    std::shared_ptr<Keychain> root_keychain() const { return root_keychain_; }
    std::vector<uint32_t> derivation_path() const { return derivation_path_; }
    uint32_t index() const { return index_; }

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

    is_private_ = root_keychain_->isPrivate();
    pubkey_ = keychain->getSigningPublicKey(index_);
}

inline secure_bytes_t Key::privkey() const
{
    if (!is_private_) throw std::runtime_error("Key::privkey - cannot get private key from nonprivate key object.");
    if (root_keychain_->isPrivateKeyLocked()) throw std::runtime_error("Key::privkey - private key is locked.");
    if (root_keychain_->isChainCodeLocked()) throw std::runtime_error("Key::privkey - chain code is locked.");

    return root_keychain_->getSigningPrivateKey(index_, derivation_path_);
}

#pragma db object pointer(std::shared_ptr)
class AccountBin : public std::enable_shared_from_this<AccountBin>
{
public:
    AccountBin(std::shared_ptr<Account> account, uint32_t index, const std::string& name);

    std::shared_ptr<Account> account() const { return account_; }
    uint32_t index() const { return index_; }

    void name(const std::string& name) { name_ = name; }
    std::string name() const { return name_; }

    uint32_t script_count() const { return script_count_; }

    std::shared_ptr<SigningScript> newSigningScript(const std::string& label = "");

    bool loadKeychains();
    KeychainSet keychains() const { return keychains_; }

private:
    friend class odb::access;
    AccountBin() { }

    #pragma db id auto
    unsigned long id_;

    #pragma db value_not_null
    std::shared_ptr<Account> account_;
    uint32_t index_;
    std::string name_;

    uint32_t script_count_;

    #pragma db transient
    KeychainSet keychains_;
};


#pragma db object pointer(std::shared_ptr)
class Account : public std::enable_shared_from_this<Account>
{
public:
    Account(const std::string& name, unsigned int minsigs, const KeychainSet& keychains, uint32_t unused_pool_size = 25, uint32_t time_created = time(NULL))
        : name_(name), minsigs_(minsigs), keychains_(keychains), unused_pool_size_(unused_pool_size), time_created_(time_created) { }

    unsigned long id() const { return id_; }

    void name(const std::string& name) { name_ = name; }
    const std::string& name() const { return name_; }
    unsigned int minsigs() const { return minsigs_; }
    KeychainSet& keychains() { return keychains_; }
    uint32_t time_created() const { return time_created_; }
    uint32_t unused_pool_size() const { return unused_pool_size_; }

    std::shared_ptr<AccountBin> addBin(const std::string& name);

    uint32_t bin_count() const { return bins_.size(); }

private:
    friend class odb::access;

    Account() { }

    #pragma db id auto
    unsigned long id_;

    #pragma db unique
    std::string name_;
    unsigned int minsigs_;
    KeychainSet keychains_;
    uint32_t unused_pool_size_; // how many unused scripts we want in our lookahead
    uint32_t time_created_;

    #pragma db value_not_null inverse(account_)
    std::vector<std::weak_ptr<AccountBin>> bins_;
};

inline std::shared_ptr<AccountBin> Account::addBin(const std::string& name)
{
    uint32_t index = bins_.size() + 1;
    std::shared_ptr<AccountBin> bin(new AccountBin(shared_from_this(), index, name));
    bins_.push_back(bin);
    return bin;
}

inline AccountBin::AccountBin(std::shared_ptr<Account> account, uint32_t index, const std::string& name)
    : account_(account), index_(index), name_(name), script_count_(0)
{
}

inline bool AccountBin::loadKeychains()
{
    if (!keychains_.empty()) return false;
    for (auto& keychain: account_->keychains())
        keychains_.insert(keychain->child(index_));
    return true;
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

    SigningScript(std::shared_ptr<AccountBin> account_bin, uint32_t index, const std::string& label = "", status_t status = UNUSED);

    SigningScript(std::shared_ptr<AccountBin> account_bin, uint32_t index, const bytes_t& txinscript, const bytes_t& txoutscript, const std::string& label = "", status_t status = UNUSED)
        : account_(account_bin->account()), account_bin_(account_bin), index_(index), label_(label), status_(status), txinscript_(txinscript), txoutscript_(txoutscript) { }

    unsigned long id() const { return id_; }
    void label(const std::string& label) { label_ = label; }
    const std::string label() const { return label_; }

    void status(status_t status) { status_ = status; }
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

inline std::shared_ptr<SigningScript> AccountBin::newSigningScript(const std::string& label)
{
    std::shared_ptr<SigningScript> signingscript(new SigningScript(shared_from_this(), script_count_++, label));
    return signingscript;
}
/*    
    loadKeychains();

    std::vector<bytes_t> pubkeys;
    for (auto& keychain: keychains_)
        pubkeys.push_back(keychain->getSigningPublicKey(script_count_));

    // sort keys into canonical order
    std::sort(pubkeys.begin(), pubkeys.end());

    CoinQ::Script::Script script(CoinQ::Script::Script::PAY_TO_MULTISIG_SCRIPT_HASH, account_->minsigs(), pubkeys);
    std::shared_ptr<SigningScript> signingscript(new SigningScript(
        shared_from_this(),
        script_count_,
        script.txinscript(CoinQ::Script::Script::EDIT),
        script.txoutscript(),
        label
    ));
    script_count_++;
    return signingscript;
}
*/

}

#endif // COINDB_SCHEMA_HXX
