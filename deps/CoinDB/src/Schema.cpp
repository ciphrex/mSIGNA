///////////////////////////////////////////////////////////////////////////////
//
// Schema.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.
//

#include "Schema.h"

#include <stdutils/stringutils.h>

#include <CoinCore/hash.h>
#include <CoinCore/CoinNodeData.h>
#include <CoinCore/hdkeys.h>
#include <CoinCore/aes.h>

#include <CoinQ/CoinQ_script.h>

#include <logger/logger.h>

//#define ENABLE_CRYPTO

using namespace CoinDB;

/*
 * class Keychain
 */

Keychain::Keychain(const std::string& name, const secure_bytes_t& entropy, const secure_bytes_t& lock_key, const bytes_t& salt)
    : name_(name), hidden_(false)
{
    if (name.empty() || name[0] == '@') throw std::runtime_error("Invalid keychain name.");

    if (!entropy.empty())
    {
        Coin::HDSeed hdSeed(entropy);
        Coin::HDKeychain hdKeychain(hdSeed.getMasterKey(), hdSeed.getMasterChainCode());

        depth_ = (uint32_t)hdKeychain.depth();
        parent_fp_ = hdKeychain.parent_fp();
        child_num_ = hdKeychain.child_num();
        chain_code_ = hdKeychain.chain_code();
        privkey_ = hdKeychain.key();
        pubkey_ = hdKeychain.pubkey();
        hash_ = hdKeychain.full_hash();

        setPrivateKeyUnlockKey(lock_key, salt);
        setChainCodeUnlockKey(lock_key, salt);
    }
}

Keychain& Keychain::operator=(const Keychain& source)
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

    hidden_ = source.hidden_;

    return *this;
}

std::shared_ptr<Keychain> Keychain::child(uint32_t i, bool get_private)
{
    if (get_private && !isPrivate()) throw std::runtime_error("Cannot get private child from public keychain.");
    if (chain_code_.empty()) throw std::runtime_error("Chain code is locked.");
    if (get_private)
    {
        if (privkey_.empty()) throw std::runtime_error("Private key is locked.");
        Coin::HDKeychain hdkeychain(privkey_, chain_code_, child_num_, parent_fp_, depth_);
        hdkeychain = hdkeychain.getChild(i);
        std::shared_ptr<Keychain> child(new Keychain());
        child->parent_ = get_shared_ptr();
        secure_bytes_t privkey = hdkeychain.privkey();
        // Strip leading zero byte if necessary
        if (privkey.size() > 32)
            child->privkey_.assign(privkey.begin() + 1, privkey.end());
        else
            child->privkey_ = privkey;
        child->privkey_ = privkey;
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

bool Keychain::setPrivateKeyUnlockKey(const secure_bytes_t& lock_key, const bytes_t& /*salt*/)
{
    if (!isPrivate()) throw std::runtime_error("Cannot lock the private key of a public keychain.");
    if (privkey_.empty()) throw std::runtime_error("Key is locked.");

#ifdef ENABLE_CRYPTO
    if (lock_key.empty())
    {
        privkey_ciphertext_ = privkey_;
        privkey_salt_.clear(); // empty salt means no encryption
    }
    else
    {
        // TODO: add real salt, better crypto function
        privkey_ciphertext_ = aes_encrypt(lock_key, privkey_, 0);
        privkey_salt_.clear();
        privkey_salt_.push_back(1);
    }
#else
    privkey_ciphertext_ = privkey_;
    privkey_salt_.clear();
#endif

    return true;
}

bool Keychain::setChainCodeUnlockKey(const secure_bytes_t& lock_key, const bytes_t& /*salt*/)
{
    if (chain_code_.empty()) throw std::runtime_error("Chain code is locked.");

#ifdef ENABLE_CRYPTO
    if (lock_key.empty())
    {
        chain_code_ciphertext_ = chain_code_;
        chain_code_salt_.clear();
    }
    else
    {
        // TODO: add real salt, better crypto function
        chain_code_ciphertext_ = aes_encrypt(lock_key, chain_code_, 0);
        chain_code_salt_.clear();
        chain_code_salt_.push_back(1);
    }
#else
    chain_code_ciphertext_ = chain_code_;
    chain_code_salt_.clear();
#endif

    return true;
}

void Keychain::lockPrivateKey() const
{
    privkey_.clear();
}

void Keychain::lockChainCode() const 
{
    chain_code_.clear();
}

void Keychain::lockAll() const
{
    lockPrivateKey();
    lockChainCode();
}

bool Keychain::isPrivateKeyLocked() const
{
    if (!isPrivate()) throw std::runtime_error("Keychain is not private.");
    return privkey_.empty();
}

bool Keychain::isChainCodeLocked() const
{
    return chain_code_.empty();
}

bool Keychain::unlockPrivateKey(const secure_bytes_t& lock_key) const
{
    if (!isPrivate()) throw std::runtime_error("Missing private key.");
    if (!privkey_.empty()) return true; // Already unlocked

#ifdef ENABLE_CRYPTO
    if (privkey_salt_.empty())
    {
        privkey_ = privkey_ciphertext_;
    }
    else
    {
        // TODO: add real salt, better crypto function
        privkey_ = aes_decrypt(lock_key, privkey_ciphertext_, 0);
    }
#else
    privkey_ = privkey_ciphertext_;
#endif

    return true;
}

bool Keychain::unlockChainCode(const secure_bytes_t& lock_key) const
{
    if (!chain_code_.empty()) return true; // Already unlocked

#ifdef ENABLE_CRYPTO
    if (chain_code_salt_.empty())
    {
        chain_code_ = chain_code_ciphertext_;
    }
    else
    {
        // TODO: add real salt, better crypto function
        chain_code_ = aes_decrypt(lock_key, chain_code_ciphertext_, 0);
    }
#else
    chain_code_ = chain_code_ciphertext_;
#endif

    return true;
}

secure_bytes_t Keychain::getSigningPrivateKey(uint32_t i, const std::vector<uint32_t>& derivation_path) const
{
    if (!isPrivate()) throw std::runtime_error("Missing private key.");
    if (privkey_.empty()) throw std::runtime_error("Private key is locked.");
    if (chain_code_.empty()) throw std::runtime_error("Chain code is locked.");

    // Remove initial zero from privkey if necessary
    secure_bytes_t stripped_privkey = (privkey_.size() > 32) ? secure_bytes_t(privkey_.begin() + 1, privkey_.end()) : privkey_;
    Coin::HDKeychain hdkeychain(stripped_privkey, chain_code_, child_num_, parent_fp_, depth_);
    for (auto k: derivation_path) { hdkeychain = hdkeychain.getChild(k); }
    return hdkeychain.getPrivateSigningKey(i);
}

bytes_t Keychain::getSigningPublicKey(uint32_t i, const std::vector<uint32_t>& derivation_path) const
{
    if (chain_code_.empty()) throw std::runtime_error("Chain code is locked.");

    Coin::HDKeychain hdkeychain(pubkey_, chain_code_, child_num_, parent_fp_, depth_);
    for (auto k: derivation_path) { hdkeychain = hdkeychain.getChild(k); }
    return hdkeychain.getPublicSigningKey(i);
}

secure_bytes_t Keychain::privkey() const
{
    if (!isPrivate()) throw std::runtime_error("Keychain is public.");
    if (privkey_.empty()) throw std::runtime_error("Keychain private key is locked.");
    return privkey_;
}

secure_bytes_t Keychain::chain_code() const
{
    if (chain_code_.empty()) throw std::runtime_error("Keychain chain code is locked.");
    return chain_code_;
}

void Keychain::importPrivateKey(const Keychain& source)
{
    privkey_ciphertext_ = source.privkey_ciphertext_;
    privkey_salt_ = source.privkey_salt_;
}

void Keychain::extkey(const secure_bytes_t& extkey, bool try_private, const secure_bytes_t& lock_key, const bytes_t& salt)
{
    Coin::HDKeychain hdKeychain(extkey);

    depth_ = (uint32_t)hdKeychain.depth();
    parent_fp_ = hdKeychain.parent_fp();
    child_num_ = hdKeychain.child_num();
    chain_code_ = hdKeychain.chain_code();
    if (hdKeychain.isPrivate() && try_private)
    {
        privkey_ = hdKeychain.key();
        setPrivateKeyUnlockKey(lock_key, salt);
    }
    else
    {
        privkey_.clear();
    }
    pubkey_ = hdKeychain.pubkey();
    hash_ = hdKeychain.full_hash();
}

secure_bytes_t Keychain::extkey(bool get_private) const
{
    if (get_private && !isPrivate()) throw std::runtime_error("Missing private key.");
    if (get_private && privkey_.empty()) throw std::runtime_error("Keychain private key is locked.");
    if (chain_code_.empty()) throw std::runtime_error("Keychain chain code is locked.");

    secure_bytes_t key;
    if (get_private)
    {
        key = (privkey_.size() > 32) ? secure_bytes_t(privkey_.begin() + 1, privkey_.end()) : privkey_;
    }
    else
    {
        key = pubkey_;
    }
    // Remove initial zero from privkey if necessary
    return Coin::HDKeychain(key, chain_code_, child_num_, parent_fp_, depth_).extkey();
}

void Keychain::clearPrivateKey()
{
    privkey_.clear();
    privkey_ciphertext_.clear();
    privkey_salt_.clear();
}


/*
 * class Key
 */

Key::Key(const std::shared_ptr<Keychain>& keychain, uint32_t index)
{
    root_keychain_ = keychain->root();
    derivation_path_ = keychain->derivation_path();
    index_ = index;

    pubkey_ = keychain->getSigningPublicKey(index_);
    updatePrivate();
}

secure_bytes_t Key::privkey() const
{
    if (!is_private_ || root_keychain_->isPrivateKeyLocked() || root_keychain_->isChainCodeLocked()) return secure_bytes_t();
    return root_keychain_->getSigningPrivateKey(index_, derivation_path_);
}

// and a version that throws exceptions
secure_bytes_t Key::try_privkey() const
{
    if (!is_private_) throw std::runtime_error("Key::privkey - cannot get private key from nonprivate key object.");
    if (root_keychain_->isPrivateKeyLocked()) throw std::runtime_error("Key::privkey - private key is locked.");
    if (root_keychain_->isChainCodeLocked()) throw std::runtime_error("Key::privkey - chain code is locked.");

    return root_keychain_->getSigningPrivateKey(index_, derivation_path_);
}


/*
 * class Account
 */

Account::Account(const std::string& name, unsigned int minsigs, const KeychainSet& keychains, uint32_t unused_pool_size, uint32_t time_created)
    : name_(name), minsigs_(minsigs), keychains_(keychains), unused_pool_size_(unused_pool_size), time_created_(time_created)
{
    // TODO: Use exception classes
    if (name_.empty() || name[0] == '@') throw std::runtime_error("Invalid account name.");
    if (keychains_.size() > 15) throw std::runtime_error("Account can use at most 15 keychains.");
    if (minsigs > keychains_.size()) throw std::runtime_error("Account minimum signatures cannot exceed number of keychains.");
    if (minsigs < 1) throw std::runtime_error("Account must require at least one signature.");

    updateHash();
}

void Account::updateHash()
{
    std::vector<bytes_t> keychain_hashes;
    for (auto& keychain: keychains_) { keychain_hashes.push_back(keychain->hash()); }
    std::sort(keychain_hashes.begin(), keychain_hashes.end());

    uchar_vector data;
    data.push_back((unsigned char)minsigs_);
    for (auto& keychain_hash: keychain_hashes) { data += keychain_hash; }

    hash_ = ripemd160(sha256(data));
}

AccountInfo Account::accountInfo() const
{
    std::vector<std::string> keychain_names;
    for (auto& keychain: keychains_) { keychain_names.push_back(keychain->name()); }

    std::vector<std::string> bin_names;
    for (auto& bin: bins_) { bin_names.push_back(bin->name()); }

    return AccountInfo(id_, name_, minsigs_, keychain_names, unused_pool_size_, time_created_, bin_names);
}

std::shared_ptr<AccountBin> Account::addBin(const std::string& name)
{
    uint32_t index = bins_.size() + 1;
    std::shared_ptr<AccountBin> bin(new AccountBin(shared_from_this(), index, name));
    bins_.push_back(bin);
    return bin;
}


/*
 * class AccountBin
 */

AccountBin::AccountBin(std::shared_ptr<Account> account, uint32_t index, const std::string& name)
    : account_(account), index_(index), name_(name), script_count_(0), next_script_index_(0), minsigs_(account->minsigs())
{
    if (index == 0) throw std::runtime_error("Account bin index cannot be zero.");
    if (index == CHANGE_INDEX && name != CHANGE_BIN_NAME) throw std::runtime_error("Account bin index reserved for change.");
    if (index == DEFAULT_INDEX && name != DEFAULT_BIN_NAME) throw std::runtime_error("Account bin index reserved for default."); 

    updateHash();
}

std::string AccountBin::account_name() const
{
    return account() ? account()->name() : std::string("@null");
}

void AccountBin::loadKeychains() const
{
    if (!keychains__.empty()) return;
    if (!account())
    {
        // If we do not have an account for this bin we cannot derive the keychains, so use the stored values.
        keychains__ = keychains_;
    }
    else
    {
        for (auto& keychain: account()->keychains())
        {
            std::shared_ptr<Keychain> child(keychain->child(index_));
            keychains__.insert(child);
        }
    }
}

void AccountBin::updateHash()
{
    loadKeychains();

    std::vector<bytes_t> keychain_hashes;
    for (auto& keychain: keychains__) { keychain_hashes.push_back(keychain->hash()); }
    std::sort(keychain_hashes.begin(), keychain_hashes.end());

    uchar_vector data;
    data.push_back((unsigned char)minsigs_);
    for (auto& keychain_hash: keychain_hashes) { data += keychain_hash; }

    hash_ = ripemd160(sha256(data));
}

std::shared_ptr<SigningScript> AccountBin::newSigningScript(const std::string& label)
{
    std::shared_ptr<SigningScript> signingscript(new SigningScript(shared_from_this(), script_count_++, label));
    return signingscript;
}

void AccountBin::markSigningScriptIssued(uint32_t script_index)
{
    if (script_index >= next_script_index_)
        next_script_index_ = script_index + 1;
}

void AccountBin::makeExport(const std::string& name)
{
    name_ = name;
    loadKeychains();
    keychains_ = keychains__;
    for (auto& keychain: keychains_) { keychain->name(""); }
    index_ = 0;
    account_.reset();
}

void AccountBin::makeImport()
{
    updateHash();
    keychains_.clear();
}


/*
 * class SigningScript
 */

// static
std::string SigningScript::getStatusString(int status)
{
    std::vector<std::string> flags;
    if (status & UNUSED) flags.push_back("UNUSED");
    if (status & CHANGE) flags.push_back("CHANGE");
    if (status & ISSUED) flags.push_back("ISSUED");
    if (status & USED) flags.push_back("USED");
    if (flags.empty()) return "NONE";

    return stdutils::delimited_list(flags, " | ");
}

// static
std::vector<SigningScript::status_t> SigningScript::getStatusFlags(int status)
{
    std::vector<status_t> flags;
    if (status & UNUSED) flags.push_back(UNUSED);
    if (status & CHANGE) flags.push_back(CHANGE);
    if (status & ISSUED) flags.push_back(ISSUED);
    if (status & USED) flags.push_back(USED);
    return flags;
}

SigningScript::SigningScript(std::shared_ptr<AccountBin> account_bin, uint32_t index, const std::string& label, status_t status)
    : account_(account_bin->account()), account_bin_(account_bin), index_(index), label_(label), status_(status)
{
    auto& keychains = account_bin_->keychains();
    for (auto& keychain: keychains)
    {
        std::shared_ptr<Key> key(new Key(keychain, index));
        keys_.push_back(key);
    }

    // sort keys into canonical order
    std::sort(keys_.begin(), keys_.end(), [](std::shared_ptr<Key> key1, std::shared_ptr<Key> key2) { return key1->pubkey() < key2->pubkey(); });

    std::vector<bytes_t> pubkeys;
    for (auto& key: keys_) { pubkeys.push_back(key->pubkey()); }
    CoinQ::Script::Script script(CoinQ::Script::Script::PAY_TO_MULTISIG_SCRIPT_HASH, account_bin->minsigs(), pubkeys);
    txinscript_ = script.txinscript(CoinQ::Script::Script::EDIT);
    txoutscript_ = script.txoutscript();
}

void SigningScript::status(status_t status)
{
    status_ = status;
    if (status > UNUSED) account_bin_->markSigningScriptIssued(index_);
}


/*
 * class BlockHeader
 */

void BlockHeader::fromCoinCore(const Coin::CoinBlockHeader& blockheader, uint32_t height)
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

Coin::CoinBlockHeader BlockHeader::toCoinCore() const
{
    return Coin::CoinBlockHeader(version_, timestamp_, bits_, nonce_, prevhash_, merkleroot_);
}


/*
 * class MerkleBlock
 */

void MerkleBlock::fromCoinCore(const Coin::MerkleBlock& merkleblock, uint32_t height)
{
    blockheader_ = std::shared_ptr<BlockHeader>(new BlockHeader(merkleblock.blockHeader, height));
    txcount_ = merkleblock.nTxs;
    hashes_.assign(merkleblock.hashes.begin(), merkleblock.hashes.end()); 
    for (auto& hash: hashes_) { std::reverse(hash.begin(), hash.end()); }
    flags_ = merkleblock.flags;
}

Coin::MerkleBlock MerkleBlock::toCoinCore() const
{
    Coin::MerkleBlock merkleblock;
    merkleblock.blockHeader = blockheader_->toCoinCore();
    merkleblock.nTxs = txcount_;
    merkleblock.hashes.assign(hashes_.begin(), hashes_.end());
    for (auto& hash: merkleblock.hashes) { std::reverse(hash.begin(), hash.end()); }
    merkleblock.flags = flags_;
    return merkleblock;
}


/*
 * class TxIn
 */

TxIn::TxIn(const Coin::TxIn& coin_txin)
{
    outhash_ = coin_txin.getOutpointHash();
    outindex_ = coin_txin.getOutpointIndex();
    script_ = coin_txin.scriptSig;
    sequence_ = coin_txin.sequence;
}

TxIn::TxIn(const bytes_t& raw)
{
    Coin::TxIn coin_txin(raw);
    outhash_ = coin_txin.getOutpointHash();
    outindex_ = coin_txin.getOutpointIndex();
    script_ = coin_txin.scriptSig;
    sequence_ = coin_txin.sequence;
}

Coin::TxIn TxIn::toCoinCore() const
{
    Coin::TxIn coin_txin;
    coin_txin.previousOut = Coin::OutPoint(outhash_, outindex_);
    coin_txin.scriptSig = script_;
    coin_txin.sequence = sequence_;
    return coin_txin;
}

bytes_t TxIn::raw() const
{
    return toCoinCore().getSerialized();
}


/*
 * class TxOut
 */

// static
std::string TxOut::getStatusString(int flags)
{
    std::vector<std::string> str_flags;
    if (flags & UNSPENT)    str_flags.push_back("UNSPENT");
    if (flags & SPENT)      str_flags.push_back("SPENT");
    if (str_flags.empty())  return "NEITHER";
    return stdutils::delimited_list(str_flags, " | ");
}

// static
std::vector<TxOut::status_t> TxOut::getStatusFlags(int flags)
{
    std::vector<status_t> vflags;
    if (flags & UNSPENT)    vflags.push_back(UNSPENT);
    if (flags & SPENT)      vflags.push_back(SPENT);
    return vflags;
}

// static
std::string TxOut::getRoleString(int flags)
{
    std::vector<std::string> str_flags;
    if (flags & ROLE_SENDER)        str_flags.push_back("SEND");
    if (flags & ROLE_RECEIVER)      str_flags.push_back("RECEIVE");
    if (str_flags.empty())          return "NONE";
    return stdutils::delimited_list(str_flags, " | ");
}

// static
std::vector<TxOut::role_t> TxOut::getRoleFlags(int flags)
{
    std::vector<role_t> vflags;
    if (flags & ROLE_SENDER)        vflags.push_back(ROLE_SENDER);
    if (flags & ROLE_RECEIVER)      vflags.push_back(ROLE_RECEIVER);
    return vflags;
}

TxOut::TxOut(uint64_t value, std::shared_ptr<SigningScript> signingscript)
    : value_(value)
{
    this->signingscript(signingscript);
}

TxOut::TxOut(const Coin::TxOut& coin_txout)
    : value_(coin_txout.value), script_(coin_txout.scriptPubKey), status_(UNSPENT)
{
}

TxOut::TxOut(const bytes_t& raw)
{
    Coin::TxOut coin_txout(raw);
    value_ = coin_txout.value;
    script_ = coin_txout.scriptPubKey;
    status_ = UNSPENT;
}

void TxOut::spent(std::shared_ptr<TxIn> spent)
{
    spent_ = spent;
    status_ = spent ? SPENT : UNSPENT;
}

void TxOut::signingscript(std::shared_ptr<SigningScript> signingscript)
{
    if (!signingscript) throw std::runtime_error("TxOut::signingscript - null signingscript.");

    script_ = signingscript->txoutscript();
    receiving_account_ = signingscript->account();
    receiving_label_ = signingscript->label();
    account_bin_ = signingscript->account_bin();
    signingscript_ = signingscript;
}

Coin::TxOut TxOut::toCoinCore() const
{
    Coin::TxOut coin_txout;
    coin_txout.value = value_;
    coin_txout.scriptPubKey = script_;
    return coin_txout;
}

bytes_t TxOut::raw() const
{
    return toCoinCore().getSerialized();
}


/*
 * class Tx
 */

// static
std::string Tx::getStatusString(int status)
{
    std::vector<std::string> flags;
    if (status & UNSIGNED) flags.push_back("UNSIGNED");
    if (status & UNSENT) flags.push_back("UNSENT");
    if (status & SENT) flags.push_back("SENT");
    if (status & PROPAGATED) flags.push_back("PROPAGATED");
    if (status & CONFLICTING) flags.push_back("CONFLICTING");
    if (status & CANCELED) flags.push_back("CANCELED");
    if (status & CONFIRMED) flags.push_back("CONFIRMED");
    if (flags.empty()) return "NO_STATUS";
    return stdutils::delimited_list(flags, " | ");
}

// static
std::vector<Tx::status_t> Tx::getStatusFlags(int status)
{
    std::vector<status_t> flags;
    if (status & UNSIGNED) flags.push_back(UNSIGNED);
    if (status & UNSENT) flags.push_back(UNSENT);
    if (status & SENT) flags.push_back(SENT);
    if (status & PROPAGATED) flags.push_back(PROPAGATED);
    if (status & CONFLICTING) flags.push_back(CONFLICTING);
    if (status & CANCELED) flags.push_back(CANCELED);
    if (status & CONFIRMED) flags.push_back(CONFIRMED);
    return flags;
}

void Tx::set(uint32_t version, const txins_t& txins, const txouts_t& txouts, uint32_t locktime, uint32_t timestamp, status_t status)
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

    Coin::Transaction coin_tx = toCoinCore();

    if (missingSigCount())  { status_ = UNSIGNED; }
    else                    { status_ = status; hash_ = coin_tx.getHashLittleEndian(); }

    coin_tx.clearScriptSigs();
    unsigned_hash_ = coin_tx.getHashLittleEndian();
}

void Tx::set(Coin::Transaction coin_tx, uint32_t timestamp, status_t status)
{
    LOGGER(trace) << "Tx::set - fromCoinCore(coin_tx);" << std::endl;
    fromCoinCore(coin_tx);

    timestamp_ = timestamp;

    if (missingSigCount())  { status_ = UNSIGNED; }
    else                    { status_ = status; hash_ = coin_tx.getHashLittleEndian(); }

    coin_tx.clearScriptSigs();
    unsigned_hash_ = coin_tx.getHashLittleEndian();
}

void Tx::set(const bytes_t& raw, uint32_t timestamp, status_t status)
{
    Coin::Transaction coin_tx(raw);
    fromCoinCore(coin_tx);
    timestamp_ = timestamp;

    if (missingSigCount())  { status_ = UNSIGNED; }
    else                    { status_ = status; hash_ = coin_tx.getHashLittleEndian(); }

    coin_tx.clearScriptSigs();
    unsigned_hash_ = coin_tx.getHashLittleEndian();
}

bool Tx::updateStatus(status_t status /* = NO_STATUS */)
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
            status_ = PROPAGATED;
            break;

        default:
            status_ = status;
        }

        hash_ = toCoinCore().getHashLittleEndian();
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

Coin::Transaction Tx::toCoinCore() const
{
    Coin::Transaction coin_tx;
    coin_tx.version = version_;
    for (auto& txin:  txins_) { coin_tx.inputs.push_back(txin->toCoinCore()); }
    for (auto& txout: txouts_) { coin_tx.outputs.push_back(txout->toCoinCore()); }
    coin_tx.lockTime = locktime_;
    return coin_tx;
}

void Tx::setBlock(std::shared_ptr<BlockHeader> blockheader, uint32_t blockindex)
{
    blockheader_ = blockheader;
    blockindex_ = blockindex;
}

void Tx::blockheader(std::shared_ptr<BlockHeader> blockheader)
{
    blockheader_ = blockheader;
    if (blockheader)                { status_ = CONFIRMED;  }
    else if (status_ == CONFIRMED)  { status_ = PROPAGATED; }   
}

bytes_t Tx::raw() const
{
    return toCoinCore().getSerialized();
}

void Tx::shuffle_txins()
{
    int i = 0;
    std::random_shuffle(txins_.begin(), txins_.end());
    for (auto& txin: txins_) { txin->txindex(i++); }
}

void Tx::shuffle_txouts()
{
    int i = 0;
    std::random_shuffle(txouts_.begin(), txouts_.end());
    for (auto& txout: txouts_) { txout->txindex(i++); }
}

void Tx::fromCoinCore(const Coin::Transaction& coin_tx)
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

unsigned int Tx::missingSigCount() const
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

std::set<bytes_t> Tx::missingSigPubkeys() const
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

