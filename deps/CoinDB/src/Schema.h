///////////////////////////////////////////////////////////////////////////////
//
// Schema.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

// odb compiler complains about #pragma once
#ifndef COINDB_SCHEMA_H
#define COINDB_SCHEMA_H

#include <CoinCore/CoinNodeData.h>

#include <CoinQ/CoinQ_typedefs.h>
#include <CoinQ/CoinQ_blocks.h>
#include <CoinQ/CoinQ_script.h>

#include <odb/core.hxx>
#include <odb/nullable.hxx>
#include <odb/database.hxx>

#include <memory>

// support for boost serialization
#include <boost/serialization/version.hpp>
#include <boost/serialization/split_member.hpp>

#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/set.hpp>

#include <logger/logger.h>

#pragma db namespace session
namespace CoinDB
{

typedef odb::nullable<unsigned long> null_id_t;

#if defined(DATABASE_MYSQL)
    const std::string DBMS = "MySQL";
    #pragma db value(std::string) type("VARCHAR(255)")
    #pragma db value(bytes_t) type("VARBINARY(255)")
#elif defined(DATABASE_SQLITE)
    const std::string DBMS = "SQLite";
    #pragma db value(bytes_t) type("BLOB")
#else
    #error "No database engine selected."
#endif

////////////////////
// SCHEMA VERSION //
////////////////////

#define SCHEMA_BASE_VERSION 12
#define SCHEMA_VERSION      22

#ifdef ODB_COMPILER
#pragma db model version(SCHEMA_BASE_VERSION, SCHEMA_VERSION, open)
#endif

typedef std::vector<unsigned long> ids_t;

// DO NOT CHANGE THE VERSION CLASS SCHEMA
#pragma db object pointer(std::shared_ptr)
class Version
{
public:
    explicit Version(uint32_t version = SCHEMA_VERSION) : version_(version) { }

    unsigned long id() const { return id_; }

    void version(uint32_t version) { version_ = version; }
    uint32_t version() const { return version_; }

private:
    friend class odb::access;

    #pragma db id auto
    unsigned long id_;

    uint32_t version_;
};


//////////////
// NETWORK  //
//////////////
#pragma db object pointer(std::shared_ptr)
class Network
{
public:
    explicit Network(const std::string& network) : network_(network) { }

    unsigned long id() const { return id_; }

    void network(const std::string& network) { network_ = network; }
    const std::string& network() const { return network_; }

private:
    Network() { }
    friend class odb::access;

    #pragma db id auto
    unsigned long id_;

    std::string network_;
};

///////////
// USERS //
///////////
#pragma db object pointer(std::shared_ptr)
class User : public std::enable_shared_from_this<User>
{
public:
    User(const std::string& username, bool txoutscript_whitelist_enabled = false) : username_(username), txoutscript_whitelist_enabled_(txoutscript_whitelist_enabled) { }

    std::shared_ptr<User> get_shared_ptr() { return shared_from_this(); }

    unsigned int id() const { return id_; }

    const std::string& username() const { return username_; }
    void username(const std::string& username) { username_ = username; }

    const std::set<bytes_t>& txoutscript_whitelist() const { return txoutscript_whitelist_; }
    void txoutscript_whitelist(const std::set<bytes_t>& txoutscripts) { txoutscript_whitelist_ = txoutscripts; }

    void addTxOutScriptToWhitelist(const bytes_t& txoutscript) { txoutscript_whitelist_.insert(txoutscript); }
    std::size_t removeTxOutScriptFromWhitelist(const bytes_t& txoutscript) { return txoutscript_whitelist_.erase(txoutscript); }
    void clearTxOutScriptWhitelist() { txoutscript_whitelist_.clear(); }

    void enableTxOutScriptWhitelist(bool enabled = true) { txoutscript_whitelist_enabled_ = enabled; }
    bool isTxOutScriptWhitelistEnabled() const { return txoutscript_whitelist_enabled_; }

private:
    User() { }

    friend class odb::access;

    #pragma db id auto
    unsigned long id_;

    #pragma db unique
    std::string username_;

    std::set<bytes_t> txoutscript_whitelist_;
    bool txoutscript_whitelist_enabled_;
};

typedef std::vector<std::shared_ptr<User>> UserVector;

//////////////
// CONTACTS //
//////////////
#pragma db object pointer(std::shared_ptr)
class Contact : public std::enable_shared_from_this<Contact>
{
public:
    Contact(const std::string& username) : username_(username) { }

    std::shared_ptr<Contact> get_shared_ptr() { return shared_from_this(); }

    unsigned int id() const { return id_; }

    const std::string& username() const { return username_; }
    void username(const std::string& username) { username_ = username; }

private:
    Contact() { }

    friend class odb::access;

    #pragma db id auto
    unsigned long id_;

    #pragma db unique
    std::string username_;
};

typedef std::vector<std::shared_ptr<Contact>> ContactVector;


////////////////////////////
// KEYCHAINS AND ACCOUNTS //
////////////////////////////

class Account;
class AccountBin;
class SigningScript;

typedef std::vector<std::shared_ptr<SigningScript>> SigningScriptVector;

#pragma db object pointer(std::shared_ptr)
class Keychain : public std::enable_shared_from_this<Keychain>
{
    typedef std::enable_shared_from_this<Keychain> base;

public:
    Keychain(bool hidden = false) : hidden_(hidden) { }
    Keychain(const std::string& name, const secure_bytes_t& entropy = secure_bytes_t(), const secure_bytes_t& lock_key = secure_bytes_t()); // Creates a new root keychain
    Keychain(const Keychain& source)
        : base(source), name_(source.name_), depth_(source.depth_), parent_fp_(source.parent_fp_), child_num_(source.child_num_), pubkey_(source.pubkey_), chain_code_(source.chain_code_), privkey_(source.privkey_), privkey_ciphertext_(source.privkey_ciphertext_), privkey_salt_(source.privkey_salt_), parent_(source.parent_), derivation_path_(source.derivation_path_), hidden_(source.hidden_) { }

    Keychain& operator=(const Keychain& source);

    std::shared_ptr<Keychain> get_shared_ptr() { return shared_from_this(); }

    unsigned int id() const { return id_; }

    std::string name() const { return name_; }
    void name(const std::string& name) { name_ = name; }

    std::shared_ptr<Keychain> root() { return (parent_ ? parent_->root() : shared_from_this()); }
    std::shared_ptr<Keychain> parent() const { return parent_; }
    std::shared_ptr<Keychain> child(uint32_t i, bool get_private = false, const secure_bytes_t& lock_key = secure_bytes_t());

    const std::vector<uint32_t>& derivation_path() const { return derivation_path_; }

    bool isPrivate() const { return (!privkey_ciphertext_.empty()); }
    bool isEncrypted() const { return privkey_salt_ != 0; }

    void lock() const;
    void unlock(const secure_bytes_t& lock_key) const;
    bool isLocked() const;
    void encrypt(const secure_bytes_t& lock_key);
    void decrypt();

    secure_bytes_t getSigningPrivateKey(uint32_t i, const std::vector<uint32_t>& derivation_path = std::vector<uint32_t>()) const;
    bytes_t getSigningPublicKey(uint32_t i, bool get_compressed = true, const std::vector<uint32_t>& derivation_path = std::vector<uint32_t>()) const;

    uint32_t depth() const { return depth_; }
    uint32_t parent_fp() const { return parent_fp_; }
    uint32_t child_num() const { return child_num_; }
    const bytes_t& pubkey() const { return pubkey_; }
    secure_bytes_t privkey() const;
    secure_bytes_t seed() const;
    bool hasSeed() const;
    const bytes_t& chain_code() const { return chain_code_; }

    void importPrivateKey(const Keychain& source);

    // hash = ripemd160(sha256(pubkey + chain_code))
    const bytes_t& hash() const { return hash_; }

    void hidden(bool hidden) { hidden_ = hidden; }
    bool hidden() const { return hidden_; }

    void importBIP32(const secure_bytes_t& extkey, const secure_bytes_t& lock_key = secure_bytes_t());
    secure_bytes_t exportBIP32(bool export_private = false) const;

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

    bytes_t chain_code_;

    #pragma db transient
    mutable secure_bytes_t privkey_;
    bytes_t privkey_ciphertext_;
    uint64_t privkey_salt_;

    #pragma db transient
    mutable secure_bytes_t seed_;
    bytes_t seed_ciphertext_;
    uint64_t seed_salt_;

    #pragma db null
    std::shared_ptr<Keychain> parent_;
    std::vector<uint32_t> derivation_path_;

    #pragma db value_not_null inverse(parent_)
    std::vector<std::weak_ptr<Keychain>> children_;

    bytes_t hash_;

    bool hidden_; // whether to display in UI

    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive& ar, const unsigned int version) const
    {
        ar & name_;
        ar & hash_;
        ar & depth_;
        ar & parent_fp_;
        ar & child_num_;
        ar & pubkey_;
        ar & chain_code_;
        if (version == 1)
        {
            // This field has been removed
            bytes_t chain_code_salt_;
            ar & chain_code_salt_;
        }
        ar & privkey_ciphertext_;
        if (version == 1)
        {
            // This field is now a uint64_t
            bytes_t old_privkey_salt_;
            ar & old_privkey_salt_;
        }
        else
        {
            ar & privkey_salt_;
        }

        // Seed added in version 3
        if (version >= 3)
        {
            ar & seed_ciphertext_;
            ar & seed_salt_;
        }
    }
    template<class Archive>
    void load(Archive& ar, const unsigned int version)
    {
        ar & name_;
        ar & hash_;
        ar & depth_;
        ar & parent_fp_;
        ar & child_num_;
        ar & pubkey_;
        ar & chain_code_;
        if (version == 1)
        {
            // This field has been removed
            bytes_t chain_code_salt_;
            ar & chain_code_salt_;
        }
        ar & privkey_ciphertext_;
        if (version == 1)
        {
            // This field is now a uint64_t
            bytes_t old_privkey_salt_;
            ar & old_privkey_salt_;
            privkey_salt_ = 0;
        }
        else
        {
            ar & privkey_salt_;
        }

        // Seed added in version 3
        if (version >= 3)
        {
            ar & seed_ciphertext_;
            ar & seed_salt_;
        }
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

typedef std::set<std::shared_ptr<Keychain>> KeychainSet;


#pragma db object pointer(std::shared_ptr)
class Key
{
public:
    Key(const std::shared_ptr<Keychain>& keychain, uint32_t index, bool compressed = true);

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



const std::string CHANGE_BIN_NAME = "@change";
const std::string DEFAULT_BIN_NAME = "@default";

typedef std::pair<uint32_t, std::string> IndexedLabel;
#pragma db value(IndexedLabel)

typedef std::map<uint32_t, std::string> IndexedLabelMap;

#pragma db object pointer(std::shared_ptr)
class AccountBin : public std::enable_shared_from_this<AccountBin>
{
    typedef std::enable_shared_from_this<AccountBin> base;

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
    AccountBin(const AccountBin& source) : base(source), account_(source.account_), index_(source.index_), name_(source.name_), script_count_(source.script_count_), next_script_index_(source.next_script_index_), minsigs_(source.minsigs_), keychains_(source.keychains_), keychains__(source.keychains__) { }

    unsigned long id() const { return id_; }

    void account(std::shared_ptr<Account> account) { account_ = account; }
    std::shared_ptr<Account> account() const { return account_.lock(); }

    std::string account_name() const;

    uint32_t index() const { return index_; }

    void name(const std::string& name) { name_ = name; }
    std::string name() const { return name_; }

    SigningScriptVector generateSigningScripts(); // generates them anew. expensive, should only be used when creating the AccountBin or when importing.

    uint32_t script_count() const { return script_count_; }
    uint32_t next_script_index() const { return next_script_index_; }
    uint32_t unused_pool_size() const;

    uint32_t minsigs() const { return minsigs_; }

    std::shared_ptr<SigningScript> newSigningScript(const std::string& label = "");
    void markSigningScriptIssued(uint32_t script_index);

    void keychains(const KeychainSet& keychains) { keychains_ = keychains; keychains__ = keychains; } // only used for imported account bins
    const KeychainSet& keychains() const { loadKeychains(); return keychains__; }

    bool isChange() const { return index_ == CHANGE_INDEX; }
    bool isDefault() const { return index_ == DEFAULT_INDEX; }

    void makeExport(const std::string& name);
    void makeImport();

    void updateHash();
    const bytes_t& hash() const { return hash_; }

private:
    friend class SigningScript;
    void setScriptLabel(uint32_t index, const std::string& label);

    void loadKeychains() const;

    friend class odb::access;

    #pragma db id auto
    unsigned long id_;

    #pragma db null
    std::weak_ptr<Account> account_;
    uint32_t index_;
    std::string name_;

    IndexedLabelMap script_label_map_;
    uint32_t script_count_; // number of scripts already persisted in database
    uint32_t next_script_index_; // index of next script in pool that will be issued


    uint32_t minsigs_;

    // Keychains are only stored in database for nonderived keychains. Otherwise, they are transient and loaded only via loadKeychains().
    #pragma db value_not_null
    KeychainSet keychains_;
    #pragma db transient
    mutable KeychainSet keychains__;

    #pragma db unique
    bytes_t hash_; // ripemd160(sha256(data)) where data = concat(first byte(minsigs), keychain hash 1, keychain hash 2, ...) and keychain hashes are sorted lexically. Byte 0x00 is appended to data before hashing if public keys are uncompressed.

    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive& ar, const unsigned int version) const
    {
        ar & name_;
        ar & index_;
        ar & next_script_index_;
        ar & minsigs_;

        uint32_t n;
        n = keychains_.size();
        ar & n;
        for (auto& keychain: keychains_)    { ar & *keychain; }

        if (version >= 2)
        {
            n = script_label_map_.size();
            ar & n;
            for (auto& script_label: script_label_map_)
            {
                ar & script_label.first;
                ar & script_label.second;
            }    
        }
    }
    template<class Archive>
    void load(Archive& ar, const unsigned int version)
    {
        ar & name_;
        ar & index_;
        ar & next_script_index_;
        ar & minsigs_;

        uint32_t n;
        ar & n;
        keychains_.clear();
        for (uint32_t i = 0; i < n; i++)
        {
            std::shared_ptr<Keychain> keychain(new Keychain(true)); // Load these keychains as hidden keychains.
            ar & *keychain;
            keychains_.insert(keychain);
        }
        keychains__ = keychains_;

        if (version >= 2)
        {
            ar & n;
            script_label_map_.clear();
            for (uint32_t i = 0; i < n; i++)
            {
                uint32_t index;
                ar & index;

                std::string label;
                ar & label;

                setScriptLabel(index, label);
            }
        }

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
        uint32_t issued_script_count,
        uint32_t unused_pool_size,
        uint32_t time_created,
        const std::vector<std::string>& bin_names,
        bool compressed_keys = true,
        bool use_witness = false,
        bool use_witness_p2sh = false
    ) :
        id_(id),
        name_(name),
        minsigs_(minsigs),
        keychain_names_(keychain_names),
        issued_script_count_(issued_script_count),
        unused_pool_size_(unused_pool_size),
        time_created_(time_created),
        bin_names_(bin_names),
        compressed_keys_(compressed_keys),
        use_witness_(use_witness),
        use_witness_p2sh_(use_witness_p2sh)
    {
        std::sort(keychain_names_.begin(), keychain_names_.end());
    }

    AccountInfo(const AccountInfo& source) :
        id_(source.id_),
        name_(source.name_),
        minsigs_(source.minsigs_),
        keychain_names_(source.keychain_names_),
        issued_script_count_(source.issued_script_count_),
        unused_pool_size_(source.unused_pool_size_),
        time_created_(source.time_created_),
        bin_names_(source.bin_names_),
        compressed_keys_(source.compressed_keys_),
        use_witness_(source.use_witness_),
        use_witness_p2sh_(source.use_witness_p2sh_)
    { }

    AccountInfo& operator=(const AccountInfo& source)
    {
        id_ = source.id_;
        name_ = source.name_;
        minsigs_ = source.minsigs_;
        keychain_names_ = source.keychain_names_;
        issued_script_count_ = source.issued_script_count_;
        unused_pool_size_ = source.unused_pool_size_;
        time_created_ = source.time_created_;
        bin_names_ = source.bin_names_;
        compressed_keys_ = source.compressed_keys_;
        use_witness_ = source.use_witness_;
        use_witness_p2sh_ = source.use_witness_p2sh_;
        return *this;
    }

    unsigned int                        id() const { return id_; }
    const std::string&                  name() const { return name_; }
    unsigned int                        minsigs() const { return minsigs_; }
    const std::vector<std::string>&     keychain_names() const { return keychain_names_; }
    uint32_t                            issued_script_count() const { return issued_script_count_; }
    uint32_t                            unused_pool_size() const { return unused_pool_size_; }
    uint32_t                            time_created() const { return time_created_; }
    const std::vector<std::string>&     bin_names() const { return bin_names_; }
    bool                                compressed_keys() const { return compressed_keys_; }
    bool                                use_witness() const { return use_witness_; }
    bool                                use_witness_p2sh() const { return use_witness_p2sh_; }

private:
    unsigned long               id_;
    std::string                 name_;
    unsigned int                minsigs_;
    std::vector<std::string>    keychain_names_;
    uint32_t                    issued_script_count_;
    uint32_t                    unused_pool_size_;
    uint32_t                    time_created_;
    std::vector<std::string>    bin_names_;
    bool                        compressed_keys_;
    bool                        use_witness_;
    bool                        use_witness_p2sh_;
};

const uint32_t DEFAULT_UNUSED_POOL_SIZE = 25;

#pragma db object pointer(std::shared_ptr)
class Account : public std::enable_shared_from_this<Account>
{
public:
    Account();
    Account(const std::string& name, unsigned int minsigs, const KeychainSet& keychains, uint32_t unused_pool_size = DEFAULT_UNUSED_POOL_SIZE, uint32_t time_created = time(NULL), bool compressed_keys = true, bool use_witness = false, bool use_witness_p2sh = false);
    Account(bool use_witness, bool use_witness_p2sh, const std::string& name, unsigned int minsigs, const KeychainSet& keychains, uint32_t unused_pool_size = DEFAULT_UNUSED_POOL_SIZE, uint32_t time_created = time(NULL), bool compressed_keys = true) : Account(name, minsigs, keychains, unused_pool_size, time_created, compressed_keys, use_witness, use_witness_p2sh) { }

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

    void compressed_keys(bool compressed_keys) { compressed_keys_ = compressed_keys; }
    bool compressed_keys() const { return compressed_keys_; }

    void loadScriptTemplates();
    bool use_witness() const { return use_witness_; }
    bool use_witness_p2sh() const { return use_witness_p2sh_; }
    const CoinQ::Script::ScriptTemplate& redeemtemplate() const { return redeemtemplate_; }

    void initScriptPatterns();
    bool redeempattern_set() const { return !redeempattern_.empty(); }

private:
    friend class odb::access;

    #pragma db id auto
    unsigned long id_;

    #pragma db unique
    std::string name_;
    unsigned int minsigs_;

    #pragma db value_not_null \
        id_column("object_id") value_column("value")
    KeychainSet keychains_;

    uint32_t unused_pool_size_; // how many unused scripts we want in our lookahead
    uint32_t time_created_;

    #pragma db unique
    bytes_t hash_; // ripemd160(sha256(data)) where data = concat(first byte(minsigs), keychain hash 1, keychain hash 2, ...) and keychain hashes are sorted lexically

    #pragma db value_not_null inverse(account_)
    AccountBinVector bins_;

    bool compressed_keys_;

    bool use_witness_;
    bytes_t redeempattern_;

    bool use_witness_p2sh_;

    #pragma db transient
    bool scripttemplatesloaded_;
    #pragma db transient
    CoinQ::Script::ScriptTemplate redeemtemplate_;

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

        if (version >= 2)
        {
            ar & compressed_keys_;
        }

        n = bins_.size();
        ar & n;
        for (auto& bin: bins_)              { ar & *bin; }

        if (version >= 3)
        {
            ar & use_witness_;
            ar & redeempattern_;
        }

        if (version >= 4)
        {
            ar & use_witness_p2sh_;
        }
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

        if (version < 2)
        {
            compressed_keys_ = true;
        }
        else
        {
            ar & compressed_keys_;
        }

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

        if (version >= 3)
        {
            ar & use_witness_;
            ar & redeempattern_;
            initScriptPatterns(); // redeempattern should always be initialized
        }
        else
        {
            initScriptPatterns();
        }

        use_witness_p2sh_ = false;
        if (version >= 4)
        {
            ar & use_witness_p2sh_;
        }

        updateHash();
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};


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

    static std::string              getStatusString(int status);
    static std::vector<status_t>    getStatusFlags(int status);

    SigningScript(std::shared_ptr<AccountBin> account_bin, uint32_t index, const std::string& label = "", status_t status = UNUSED);
    SigningScript(std::shared_ptr<AccountBin> account_bin, uint32_t index, const bytes_t& txinscript, const bytes_t& txoutscript, const std::string& label = "", status_t status = UNUSED)
        : account_(account_bin->account()), account_bin_(account_bin), index_(index), label_(label), status_(status), txinscript_(txinscript), txoutscript_(txoutscript) { }

    unsigned long id() const { return id_; }
    void label(const std::string& label);
    const std::string label() const { return label_; }

    void status(status_t status);
    status_t status() const { return status_; }

    void markUsed();

    const bytes_t& redeemscript() const { return redeemscript_; }  
    const bytes_t& txinscript() const { return txinscript_; }
    const bytes_t& txoutscript() const { return txoutscript_; }

    std::shared_ptr<Account> account() const { return account_; }

    // 0 is reserved for subaccounts, 1 is reserved for change addresses, 2 is reserved for the default bin.
    std::shared_ptr<AccountBin> account_bin() const { return account_bin_; }

    uint32_t index() const { return index_; }

    KeyVector& keys() { return keys_; }

    void contact(std::shared_ptr<Contact> contact) { contact_ = contact; }
    std::shared_ptr<Contact> contact() const { return contact_; }

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

    #pragma db type("BLOB")
    bytes_t redeemscript_;
    #pragma db type("BLOB")
    bytes_t txinscript_; // unsigned (0 byte length placeholders are used for signatures)
    #pragma db type("BLOB")
    bytes_t txoutscript_;

    KeyVector keys_;

    std::shared_ptr<Contact> contact_;
};



/////////////////////////////
// BLOCKS AND TRANSACTIONS //
/////////////////////////////

class Tx;
class TxIn;
class TxOut;

#pragma db object pointer(std::shared_ptr)
class BlockHeader
{
public:
    BlockHeader() { }

    BlockHeader(const Coin::CoinBlockHeader& blockheader, uint32_t height = 0xffffffff) { fromCoinCore(blockheader, height); }

    BlockHeader(uint32_t version, const bytes_t& prevhash, const bytes_t& merkleroot, uint32_t timestamp, uint32_t bits, uint32_t nonce, uint32_t height = 0xffffffff)
    : height_(height), version_(version), prevhash_(prevhash), merkleroot_(merkleroot), timestamp_(timestamp), bits_(bits), nonce_(nonce) { updateHash(); }

    void fromCoinCore(const Coin::CoinBlockHeader& blockheader, uint32_t height = 0xffffffff);
    Coin::CoinBlockHeader toCoinCore() const;

    unsigned long id() const { return id_; }
    bytes_t hash() const { return hash_; }

    void height(uint32_t height) { height_ = height; }
    uint32_t height() const { return height_; }

    uint32_t version() const { return version_; }
    bytes_t prevhash() const { return prevhash_; }
    bytes_t merkleroot() const { return merkleroot_; }
    uint32_t timestamp() const { return timestamp_; }
    uint32_t bits() const { return bits_; }
    uint32_t nonce() const { return nonce_; }

    std::string toJson() const;

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

    void updateHash();

    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive& ar, const unsigned int /*version*/) const
    {
        ar & height_;
        ar & version_;
        ar & prevhash_;
        ar & merkleroot_;
        ar & timestamp_;
        ar & bits_;
        ar & nonce_;
    }
    template<class Archive>
    void load(Archive& ar, const unsigned int /*version*/)
    {
        ar & height_;
        ar & version_;
        ar & prevhash_;
        ar & merkleroot_;
        ar & timestamp_;
        ar & bits_;
        ar & nonce_;
        updateHash();
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};


#pragma db object pointer(std::shared_ptr)
class MerkleBlock
{
public:
    MerkleBlock() : txsinserted_(false) { }

    MerkleBlock(const std::shared_ptr<BlockHeader>& blockheader, uint32_t txcount, const std::vector<bytes_t>& hashes, const bytes_t& flags)
        : blockheader_(blockheader), txcount_(txcount), hashes_(hashes), flags_(flags), txsinserted_(false) { }

    MerkleBlock(const ChainMerkleBlock& merkleblock) : txsinserted_(false) { fromCoinCore(merkleblock, merkleblock.height); }

    void fromCoinCore(const Coin::MerkleBlock& merkleblock, uint32_t height = 0xffffffff);
    Coin::MerkleBlock toCoinCore() const;

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

    void txsinserted(bool txsinserted) { txsinserted_ = txsinserted; }
    bool txsinserted() const { return txsinserted_; }

    std::string toJson() const;

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

    bool txsinserted_;

    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive& ar, const unsigned int /*version*/) const
    {
        ar & *blockheader_;
        ar & txcount_;

        uint32_t n = hashes_.size();
        ar & n;
        for (auto& hash: hashes_)    { ar & hash; }

        ar & flags_;
    }
    template<class Archive>
    void load(Archive& ar, const unsigned int /*version*/)
    { 
        blockheader_ = std::shared_ptr<BlockHeader>(new BlockHeader());
        ar & *blockheader_;
        ar & txcount_;

        uint32_t n;
        ar & n;
        hashes_.clear();
        for (uint32_t i = 0; i < n; i++)
        {
            bytes_t hash;
            ar & hash;
            hashes_.push_back(hash);
        }

        ar & flags_;
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};


#pragma db object pointer(std::shared_ptr)
class TxIn
{
public:
    TxIn() { }
    TxIn(const bytes_t& outhash, uint32_t outindex, const bytes_t& script, uint32_t sequence)
        : outhash_(outhash), outindex_(outindex), script_(script), sequence_(sequence) { }

    TxIn(const Coin::TxIn& coin_txin);
    TxIn(const bytes_t& raw);

    void fromCoinCore(const Coin::TxIn& coin_txin);
    Coin::TxIn toCoinCore() const;

    unsigned long id() const { return id_; }
    const bytes_t& outhash() const { return outhash_; }
    uint32_t outindex() const { return outindex_; }

    void script(const bytes_t& script) { script_ = script; }
    const bytes_t& script() const { return script_; }
    bytes_t unsigned_script() const; // throws exception if script type is not recognized

    uint32_t sequence() const { return sequence_; }
    bytes_t raw() const;

    void tx(std::shared_ptr<Tx> tx) { tx_ = tx; }
    const std::shared_ptr<Tx> tx() const { return tx_.lock(); }

    void txindex(unsigned int txindex) { txindex_ = txindex; }
    uint32_t txindex() const { return txindex_; }

    void outpoint(std::shared_ptr<TxOut> outpoint);
    const std::shared_ptr<TxOut> outpoint() const { return outpoint_.lock(); }

    void scriptwitnessstack(const std::vector<bytes_t>& scriptwitnessstack) { scriptwitnessstack_ = scriptwitnessstack; }
    const std::vector<bytes_t>& scriptwitnessstack() const { return scriptwitnessstack_; }

    std::string toJson() const;

private:
    friend class odb::access;

    #pragma db id auto
    unsigned long id_;

    bytes_t outhash_;
    uint32_t outindex_;
    bytes_t script_;
    uint32_t sequence_;

    #pragma db not_null
    std::weak_ptr<Tx> tx_;

    uint32_t txindex_;

    #pragma db null
    std::weak_ptr<TxOut> outpoint_;

    #pragma db value_not_null \
        id_column("object_id") value_column("value")
    std::vector<bytes_t> scriptwitnessstack_;

    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive& ar, const unsigned int version) const
    {
        ar & outhash_;
        ar & outindex_;
        ar & script_;
        ar & sequence_;

        if (version >= 2)
        {
            uint32_t n = scriptwitnessstack_.size();
            ar & n;
            for (auto& scriptwitnessitem: scriptwitnessstack_)    { ar & scriptwitnessitem; }
        }
    }
    template<class Archive>
    void load(Archive& ar, const unsigned int version)
    { 
        ar & outhash_;
        ar & outindex_;
        ar & script_;
        ar & sequence_;

        if (version >= 2)
        {
            uint32_t n;
            ar & n;
            scriptwitnessstack_.clear();
            for (uint32_t i = 0; i < n; i++)
            {
                bytes_t scriptwitnessitem;
                ar & scriptwitnessitem;
                scriptwitnessstack_.push_back(scriptwitnessitem);
            }
        }
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

typedef std::vector<std::shared_ptr<TxIn>> txins_t;


#pragma db object pointer(std::shared_ptr)
class TxOut
{
public:
    enum status_t
    {
        NEITHER     =  0,
        UNSPENT     =  1,
        SPENT       =  1 << 1,
        BOTH        = (1 << 2) - 1
    };

    static std::string              getStatusString(int flags);
    static std::vector<status_t>    getStatusFlags(int flags);


    enum role_t
    {
        ROLE_NONE       =  0,
        ROLE_SENDER     =  1,
        ROLE_RECEIVER   =  1 << 1,
        ROLE_BOTH       = (1 << 2) - 1
    };

    static std::string              getRoleString(int flags);
    static std::vector<role_t>      getRoleFlags(int flags);


    TxOut() : status_(UNSPENT) { }
    TxOut(uint64_t value, const bytes_t& script)
        : value_(value), script_(script), status_(UNSPENT) { }

    // Constructor for change and transfers
    TxOut(uint64_t value, std::shared_ptr<SigningScript> signingscript);

    TxOut(const Coin::TxOut& coin_txout);
    TxOut(const bytes_t& raw);

    Coin::TxOut toCoinCore() const;

    unsigned long id() const { return id_; }

    void value(uint64_t value) { value_ = value; }
    uint64_t value() const { return value_; }

    void script(const bytes_t& script) { script_ = script; }
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

    std::string toJson() const;

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
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
        ar & value_;
        ar & script_;
        ar & sending_label_;
        ar & receiving_label_;
    }
};

typedef std::vector<std::shared_ptr<TxOut>> txouts_t;


#pragma db object pointer(std::shared_ptr)
class Tx : public std::enable_shared_from_this<Tx>
{
public:
    /*
     * If status is UNSIGNED, remove all txinscripts before hashing so hash
     * is unchanged by adding signatures until fully signed. Then compute
     * normal hash and transition to one of the other states.
     *
     * The states are ordered such that transitions are generally from smaller values to larger values.
     * Blockchain reorgs are the exception where it's possible that a CONFIRMED state reverts to an
     * earlier state.
     */
    enum status_t
    {
        NO_STATUS    =  0,
        UNSIGNED     =  1,      // still missing signatures
        UNSENT       =  1 << 1, // signed but not yet broadcast to network
        SENT         =  1 << 2, // sent to at least one peer but possibly not propagated
        PROPAGATED   =  1 << 3, // received from at least one peer
        CANCELED     =  1 << 5, // either will never be broadcast or will never confirm
        CONFIRMED    =  1 << 6, // exists in blockchain
        ALL          = (1 << 7) - 1
    };

    static std::string              getStatusString(int status, bool lowercase = false);
    static std::vector<status_t>    getStatusFlags(int status);

    std::string                     getStatusString() const { return getStatusString(status_); }

    Tx(uint32_t version = 1, uint32_t locktime = 0, uint32_t timestamp = 0xffffffff, status_t status = PROPAGATED, bool conflicting = false)
        : version_(version), locktime_(locktime), timestamp_(timestamp), status_(status), conflicting_(conflicting),  have_all_outpoints_(false), txin_total_(0), txout_total_(0) { }

    void set(uint32_t version, const txins_t& txins, const txouts_t& txouts, uint32_t locktime, uint32_t timestamp = 0xffffffff, status_t status = PROPAGATED, bool conflicting = false, bool checksigs = false);
    void set(Coin::Transaction coin_tx, uint32_t timestamp = 0xffffffff, status_t status = PROPAGATED, bool conflicting = false, bool checksigs = false);
    void set(const bytes_t& raw, uint32_t timestamp = 0xffffffff, status_t status = PROPAGATED, bool conflicting = false, bool checksigs = false);

    Coin::Transaction toCoinCore() const;

    void setBlock(std::shared_ptr<BlockHeader> blockheader, uint32_t blockindex);

    unsigned long id() const { return id_; }
    uint32_t version() const { return version_; }

    void hash(const bytes_t& hash) { hash_ = hash; }
    const bytes_t& hash() const { return status_ == UNSIGNED ? unsigned_hash_ : hash_; }
    const bytes_t& signed_hash() const { return hash_; }
    const bytes_t& unsigned_hash() const { return unsigned_hash_; }
    txins_t txins() const { return txins_; }
    txouts_t txouts() const { return txouts_; }
    uint32_t locktime() const { return locktime_; }
    bytes_t raw(bool withWitness = true) const;

    void timestamp(uint32_t timestamp) { timestamp_ = timestamp; }
    uint32_t timestamp() const { return timestamp_; }

    bool updateStatus(status_t status = NO_STATUS, bool checksigs = false); // Will keep the status it already had if it didn't change and no parameter is passed. Returns true iff status changed.
    void status(status_t status) { status_ = status; }
    status_t status() const { return status_; }

    void conflicting(bool conflicting) { conflicting_ = conflicting; }
    bool conflicting() const { return conflicting_; }

    void updateTotals();
    bool have_all_outpoints() const { return have_all_outpoints_; }
    uint64_t txin_total() const { return txin_total_; }
    uint64_t txout_total() const { return txout_total_; }
    uint64_t fee() const { return have_all_outpoints_ ? txin_total_ - txout_total_ : 0; }

    void block(std::shared_ptr<BlockHeader> header, uint32_t index) { blockheader_ = header; blockindex_ = index; }

    void blockheader(std::shared_ptr<BlockHeader> blockheader);
    std::shared_ptr<BlockHeader> blockheader() const { return blockheader_; }

    void user(std::shared_ptr<User> user) { user_ = user; }
    std::shared_ptr<User> user() const { return user_; }

    void propagation_protocol(const std::string& propagation_protocol) { propagation_protocol_ = propagation_protocol; }
    std::string propagation_protocol() const { return propagation_protocol_; }

    void shuffle_txins();
    void shuffle_txouts();

    unsigned int missingSigCount() const;
    std::set<bytes_t> missingSigPubkeys() const;
    std::set<bytes_t> presentSigPubkeys() const;

    CoinQ::Script::Signer signer() const;

    std::string toJson(bool includeRawHex = false, bool includeSerialized = false) const;

    std::string toSerialized() const;
    void fromSerialized(const std::string& serialized);

private:
    friend class odb::access;

    void fromCoinCore(const Coin::Transaction& coin_tx);

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
    // Once status is PROPAGATED the timestamp is fixed.
    // Timestamp defaults to 0xffffffff
    uint32_t timestamp_;

    status_t status_;

    bool conflicting_;

    // Due to issue with odb::nullable<uint64_t> in mingw-w64, we use a second bool variable.
    bool have_all_outpoints_;
    uint64_t txin_total_;
    uint64_t txout_total_;

    #pragma db null
    std::shared_ptr<BlockHeader> blockheader_;

    #pragma db null
    odb::nullable<uint32_t> blockindex_;

    #pragma db null
    std::shared_ptr<User> user_;

    std::string propagation_protocol_;

    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive& ar, const unsigned int v) const
    {
        ar & version_;

        uint32_t n;
        n = txins_.size();
        ar & n;
        for (auto& txin: txins_)    { ar & *txin; }

        n = txouts_.size();
        ar & n;
        for (auto& txout: txouts_)  { ar & *txout; } 

        ar & locktime_;
        ar & timestamp_; // only used for sorting in UI

        uint32_t statusflag = (uint32_t)status_;
        ar & statusflag;

        if (v >= 2)
        {
            if (user_)
            {
                bool has_user = true;
                ar & has_user;
                ar & user_->username();
            }
            else
            {
                bool has_user = false;
                ar & has_user;
            }
        }

        if (v >= 3)
        {
            ar & propagation_protocol_;
        }
    }
    template<class Archive>
    void load(Archive& ar, const unsigned int v)
    {
        ar & version_;

        uint32_t n;
        ar & n;
        txins_.clear();
        for (uint32_t i = 0; i < n; i++)
        {
            std::shared_ptr<TxIn> txin(new TxIn());
            ar & *txin;
            txin->tx(shared_from_this());
            txin->txindex(i);
            txins_.push_back(txin);
        }

        ar & n;
        txouts_.clear();
        for (uint32_t i = 0; i < n; i++)
        {
            std::shared_ptr<TxOut> txout(new TxOut());
            ar & *txout;
            txout->tx(shared_from_this());
            txout->txindex(i);
            txouts_.push_back(txout);
        }

        ar & locktime_;
        ar & timestamp_; // only used for internal sorting

        uint32_t statusflag;
        ar & statusflag;

        Coin::Transaction coin_tx = toCoinCore();
     
        if (missingSigCount())
        {
            status_ = UNSIGNED;
            hash_.clear();            
        }
        else
        {
            std::vector<status_t> flags = Tx::getStatusFlags(statusflag);
            status_ = flags.empty() ? NO_STATUS : flags[0];
            hash_ = coin_tx.hash();
        }
     
        conflicting_ = false;
     
        coin_tx.clearScriptSigs();
        unsigned_hash_ = coin_tx.hash();
        updateTotals();

        if (v >= 2)
        {
            bool has_user;
            ar & has_user;
            if (has_user)
            {
                std::string username;
                ar & username;
                // TODO: handle user
                //user_ = std::make_shared<User>(username);
            }
        }

        if (v >= 3)
        {
            ar & propagation_protocol_;
        }
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

typedef std::vector<std::shared_ptr<Tx>> txs_t;


// Views
#pragma db view \
    object(Keychain) \
    table("Account_keychains" = "t": "t.value = " + Keychain::id_) \
    object(Account: "t.object_id = " + Account::id_)
struct KeychainView
{
    KeychainView() : is_locked(true) { }

    #pragma db column(Keychain::id_)
    unsigned long id;
    #pragma db column(Keychain::name_)
    std::string name;
    #pragma db column(Keychain::depth_)
    uint32_t depth;
    #pragma db column(Keychain::parent_fp_)
    uint32_t parent_fp;
    #pragma db column(Keychain::child_num_)
    uint32_t child_num;
    #pragma db column(Keychain::pubkey_)
    bytes_t pubkey;
    #pragma db column(Keychain::hash_)
    bytes_t hash;
    #pragma db column("length(" + Keychain::privkey_ciphertext_ + ") != 0")
    bool is_private;
    #pragma db column(Keychain::privkey_salt_ + "!= 0")
    bool is_encrypted;
    #pragma db transient
    bool is_locked;
};

#pragma db view \
    object(Account)
struct AccountView
{
    unsigned long               id;
    std::string                 name;
    unsigned int                minsigs;
    uint32_t                    unused_pool_size;
    uint32_t                    time_created;

    #pragma db transient
    std::vector<KeychainView>    keychain_views;

    #pragma db transient
    std::vector<std::string>    bin_names;
};

#pragma db view \
    object(Account)
struct AccountCountView
{
    #pragma db column("count(" + Account::id_ + ")")
    uint32_t count;
};

#pragma db view \
    object(AccountBin) \
    object(Account: AccountBin::account_)
struct AccountBinView
{
    #pragma db column(Account::id_)
    unsigned long account_id;
    #pragma db column(Account::name_)
    std::string account_name;
    #pragma db column(Account::hash_)
    bytes_t account_hash;

    #pragma db column(AccountBin::id_)
    unsigned long bin_id; 
    #pragma db column(AccountBin::name_)
    std::string bin_name;
    #pragma db column(AccountBin::hash_)
    bytes_t bin_hash;
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

    #pragma db column(SigningScript::redeemscript_)
    bytes_t redeemscript;

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

    #pragma db column("max(" + SigningScript::index_ + ")")
    unsigned long max_index;
};

#pragma db view \
    object(Tx) \
    object(BlockHeader: Tx::blockheader_)
struct TxView
{
    #pragma db column(Tx::id_)
    unsigned long id;
    #pragma db column(Tx::hash_)
    bytes_t hash;
    #pragma db column(Tx::unsigned_hash_)
    bytes_t unsigned_hash;
    #pragma db column(Tx::version_)
    uint32_t version;
    #pragma db column(Tx::locktime_)
    uint32_t locktime;
    #pragma db column(Tx::timestamp_)
    uint32_t timestamp;
    #pragma db column(Tx::status_)
    Tx::status_t status;
    #pragma db column(Tx::have_all_outpoints_)
    bool have_all_outpoints;
    #pragma db column(Tx::txin_total_)
    uint64_t txin_total;
    #pragma db column(Tx::txout_total_)
    uint64_t txout_total;

    uint64_t fee() const
    {
        if (txin_total >= txout_total)
            return txin_total - txout_total;
        else
            return 0;
    }

    #pragma db column(BlockHeader::height_)
    uint32_t height;
};

const std::string EMPTY_STRING = "";

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
    TxOutView() : role_flags(TxOut::ROLE_NONE) { }
    TxOutView(const TxOutView& source, TxOut::role_t role)
    {
        *this = source;
        role_flags = role;
    }

    // must be called when queried - TODO: better style
    void updateRole(int flags = TxOut::ROLE_BOTH)
    {
        role_flags = TxOut::ROLE_NONE;
        if (sending_account_id)          { role_flags |= TxOut::ROLE_SENDER;   }
        if (receiving_account_id)        { role_flags |= TxOut::ROLE_RECEIVER; }
        role_flags &= flags;
    }


    const std::string& role_account() const
    {
        switch (role_flags)
        {
        case TxOut::ROLE_SENDER:
            return sending_account_name;

        case TxOut::ROLE_RECEIVER:
            return receiving_account_name;

        default:
            return EMPTY_STRING;
        }
    }

    const std::string& role_bin() const
    {
        if (role_flags == TxOut::ROLE_RECEIVER)
            return account_bin_name;
        else
            return EMPTY_STRING;
    }

    const std::string& role_label() const
    {
        switch (role_flags)
        {
        case TxOut::ROLE_SENDER:
            return sending_label;

        case TxOut::ROLE_RECEIVER:
            return receiving_label;

        default:
            return EMPTY_STRING;
        }
    }

    std::vector<TxOutView> getSplitRoles(TxOut::role_t first = TxOut::ROLE_RECEIVER, const std::string& account_name = "")
    {
        std::vector<TxOutView> split_views;

        if ((role_flags & TxOut::ROLE_RECEIVER) && (account_name.empty() || account_name == receiving_account_name))
        {
            split_views.push_back(TxOutView(*this, TxOut::ROLE_RECEIVER));
        }

        if ((role_flags & TxOut::ROLE_SENDER) && (account_name.empty() || account_name == sending_account_name))
        {
            split_views.push_back(TxOutView(*this, TxOut::ROLE_SENDER));
        }

        if (first == TxOut::ROLE_SENDER)
        {
            std::reverse(split_views.begin(), split_views.end());
        }

        return split_views;
    }

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

    #pragma db column(SigningScript::redeemscript_)
    bytes_t signingscript_redeemscript;

    #pragma db column(SigningScript::txinscript_)
    bytes_t signingscript_txinscript;

    #pragma db column(TxOut::id_)
    unsigned long id;

    #pragma db column(TxOut::script_)
    bytes_t script;

    #pragma db column(TxOut::value_)
    uint64_t value;

    #pragma db column(TxOut::status_)
    TxOut::status_t status;

    #pragma db transient
    int role_flags;

    #pragma db column(TxOut::sending_label_)
    std::string sending_label;

    #pragma db column(TxOut::receiving_label_)
    std::string receiving_label;

    #pragma db column(Tx::id_)
    unsigned long tx_id;

    #pragma db column(Tx::unsigned_hash_)
    bytes_t tx_unsigned_hash;

    #pragma db column(Tx::hash_)
    bytes_t tx_hash;

    #pragma db column(Tx::timestamp_)
    uint32_t tx_timestamp;

    #pragma db column(Tx::status_)
    Tx::status_t tx_status;

    #pragma db column(Tx::have_all_outpoints_)
    bool tx_has_all_outpoints;

    #pragma db column(Tx::txin_total_)
    uint64_t tx_txin_total;

    #pragma db column(Tx::txout_total_)
    uint64_t tx_txout_total;

    uint64_t tx_fee() const
    {
        if (tx_txin_total >= tx_txout_total)
            return tx_txin_total - tx_txout_total;
        else
            return 0;
    }

    #pragma db column(TxOut::txindex_)
    uint32_t tx_index;

    #pragma db column(BlockHeader::height_)
    uint32_t height;
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
	object(MerkleBlock) \
    object(BlockHeader: MerkleBlock::blockheader_) \
	query(MerkleBlock::txsinserted_ == true)
struct BestHeightView
{
    #pragma db column("max(" + BlockHeader::height_ + ")")
    uint32_t height;
};

#pragma db view \
    object(BlockHeader)
struct HorizonHeightView
{
    #pragma db column("min(" + BlockHeader::height_ + ")")
    uint32_t height;
};

#pragma db view \
    object(BlockHeader)
struct BlockCountView
{
    #pragma db column("count(" + BlockHeader::id_ + ")")
    unsigned long count;
};

#pragma db view \
    object(MerkleBlock)
struct MerkleBlockCountView
{
    #pragma db column("count(" + MerkleBlock::id_ + ")")
    unsigned long count;
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

#pragma db view \
    object(MerkleBlock) query(MerkleBlock::txsinserted_ == false)
struct IncompleteBlockCountView
{
    #pragma db column("count(" + MerkleBlock::id_ + ")")
    unsigned long count;
};

}

BOOST_CLASS_VERSION(CoinDB::BlockHeader, 1)
BOOST_CLASS_VERSION(CoinDB::MerkleBlock, 1)

BOOST_CLASS_VERSION(CoinDB::TxIn, 2)
BOOST_CLASS_VERSION(CoinDB::TxOut, 1)
BOOST_CLASS_VERSION(CoinDB::Tx, 3)

BOOST_CLASS_VERSION(CoinDB::Keychain, 3)
BOOST_CLASS_VERSION(CoinDB::AccountBin, 2)
BOOST_CLASS_VERSION(CoinDB::Account, 4)

#endif // COINDB_SCHEMA_H
