///////////////////////////////////////////////////////////////////////////////
//
// Schema.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "Schema.h"

#include <stdutils/stringutils.h>

#include <CoinCore/hash.h>
#include <CoinCore/CoinNodeData.h>
#include <CoinCore/MerkleTree.h>
#include <CoinCore/hdkeys.h>
#include <CoinCore/aes.h>
#include <CoinCore/random.h>

//#include <logger/logger.h>

// support for boost serialization
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <cstring>

//#define ENABLE_CRYPTO

using namespace CoinDB;

/*
 * class Keychain
 */

Keychain::Keychain(const std::string& name, const secure_bytes_t& entropy, const secure_bytes_t& lock_key)
    : name_(name), hidden_(false)
{
    if (name.empty() || name[0] == '@') throw std::runtime_error("Invalid keychain name.");
    if (entropy.size() < 16) throw std::runtime_error("At least 128 bits of entropy must be supplied.");

    Coin::HDSeed hdSeed(entropy);
    Coin::HDKeychain hdKeychain(hdSeed.getMasterKey(), hdSeed.getMasterChainCode());

    depth_ = (uint32_t)hdKeychain.depth();
    parent_fp_ = hdKeychain.parent_fp();
    child_num_ = hdKeychain.child_num();
    chain_code_ = hdKeychain.chain_code();
    pubkey_ = hdKeychain.pubkey();
    privkey_ = hdKeychain.privkey();
    seed_ = entropy;
    if (lock_key.empty())
    {
        privkey_salt_ = 0;
        privkey_ciphertext_ = privkey_;

        seed_salt_ = 0;
        seed_ciphertext_ = seed_;
    }
    else
    {
        privkey_salt_ = AES::random_salt();
        privkey_ciphertext_ = AES::encrypt(lock_key, privkey_, true, privkey_salt_);

        seed_salt_ = AES::random_salt();
        seed_ciphertext_ = AES::encrypt(lock_key, seed_, true, seed_salt_);
    }
    privkey_.clear();
    seed_.clear();
    hash_ = hdKeychain.full_hash();
}

Keychain& Keychain::operator=(const Keychain& source)
{
    base::operator=(source);

    depth_ = source.depth_;
    parent_fp_ = source.parent_fp_;
    child_num_ = source.child_num_;
    pubkey_ = source.pubkey_;

    chain_code_ = source.chain_code_;

    privkey_ = source.privkey_;
    privkey_ciphertext_ = source.privkey_ciphertext_;
    privkey_salt_ = source.privkey_salt_;

    seed_ = source.seed_;
    seed_ciphertext_ = source.seed_ciphertext_;
    seed_salt_ = source.seed_salt_;

    parent_ = source.parent_;

    derivation_path_ = source.derivation_path_;

    uchar_vector_secure hashdata = pubkey_;
    hashdata += chain_code_;
    hash_ = ripemd160(sha256(hashdata));

    hidden_ = source.hidden_;

    return *this;
}

std::shared_ptr<Keychain> Keychain::child(uint32_t i, bool get_private, const secure_bytes_t& lock_key)
{
    if (get_private && !isPrivate()) throw std::runtime_error("Cannot get private child from nonprivate keychain.");
    if (get_private)
    {
        if (privkey_.empty()) throw std::runtime_error("Private key is locked.");
        Coin::HDKeychain hdkeychain(privkey_, chain_code_, child_num_, parent_fp_, depth_);
        hdkeychain = hdkeychain.getChild(i);
        std::shared_ptr<Keychain> child(new Keychain());
        child->parent_ = get_shared_ptr();
        child->pubkey_ = hdkeychain.pubkey();
        child->chain_code_ = hdkeychain.chain_code();

        child->privkey_ = hdkeychain.privkey();
        if (lock_key.empty())
        {
            child->privkey_salt_ = 0;
            child->privkey_ciphertext_ = privkey_;

            child->seed_salt_ = 0;
            child->seed_ciphertext_ = seed_;
        }
        else
        {
            child->privkey_salt_ = AES::random_salt();
            child->privkey_ciphertext_ = AES::encrypt(lock_key, child->privkey_, true, child->privkey_salt_);

            if (seed_.empty())
            {
                child->seed_salt_ = 0;
            }
            else
            {
                child->seed_salt_ = AES::random_salt();
                child->seed_ciphertext_ = AES::encrypt(lock_key, seed_, true, child->seed_salt_);
            }
        }
        child->privkey_.clear();

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

void Keychain::lock() const
{
    privkey_.clear();
    seed_.clear();
}

void Keychain::unlock(const secure_bytes_t& lock_key) const
{
    if (!isPrivate()) throw std::runtime_error("Cannot unlock a nonprivate keychain.");
    if (privkey_salt_ == 0)
    {
        privkey_ = privkey_ciphertext_;
    }
    else
    {
        privkey_ = AES::decrypt(lock_key, privkey_ciphertext_, true, privkey_salt_); 
    }

    if (seed_salt_ == 0 || seed_ciphertext_.empty())
    {
        seed_ = seed_ciphertext_;
    }
    else
    {
        seed_ = AES::decrypt(lock_key, seed_ciphertext_, true, seed_salt_); 
    }
}

bool Keychain::isLocked() const
{
    return privkey_.empty();
}

void Keychain::encrypt(const secure_bytes_t& lock_key)
{
    if (!isPrivate()) throw std::runtime_error("Cannot encrypt a nonprivate keychain.");
    if (isLocked()) throw std::runtime_error("Keychain is locked.");

    privkey_salt_ = AES::random_salt();
    privkey_ciphertext_ = AES::encrypt(lock_key, privkey_, true, privkey_salt_);

    if (!seed_.empty())
    {
        seed_salt_ = AES::random_salt();
        seed_ciphertext_ = AES::encrypt(lock_key, seed_, true, seed_salt_);
    }
}

void Keychain::decrypt()
{
    if (!isPrivate()) throw std::runtime_error("Cannot unencrypt a nonprivate keychain.");
    if (isLocked()) throw std::runtime_error("Keychain is locked.");

    privkey_salt_ = 0;
    privkey_ciphertext_ = privkey_;

    seed_salt_ = 0;
    seed_ciphertext_ = seed_;
}

secure_bytes_t Keychain::getSigningPrivateKey(uint32_t i, const std::vector<uint32_t>& derivation_path) const
{
    if (!isPrivate()) throw std::runtime_error("Missing private key.");
    if (isLocked()) throw std::runtime_error("Private key is locked.");

    // Remove initial zero from privkey if necessary
    secure_bytes_t stripped_privkey = (privkey_.size() > 32) ? secure_bytes_t(privkey_.begin() + 1, privkey_.end()) : privkey_;
    Coin::HDKeychain hdkeychain(stripped_privkey, chain_code_, child_num_, parent_fp_, depth_);
    for (auto k: derivation_path) { hdkeychain = hdkeychain.getChild(k); }
    return hdkeychain.getPrivateSigningKey(i);
}

bytes_t Keychain::getSigningPublicKey(uint32_t i, bool get_compressed, const std::vector<uint32_t>& derivation_path) const
{
    Coin::HDKeychain hdkeychain(pubkey_, chain_code_, child_num_, parent_fp_, depth_);
    for (auto k: derivation_path) { hdkeychain = hdkeychain.getChild(k); }
    return hdkeychain.getPublicSigningKey(i, get_compressed);
}

secure_bytes_t Keychain::privkey() const
{
    if (!isPrivate()) throw std::runtime_error("Keychain is nonprivate.");
    if (privkey_.empty()) throw std::runtime_error("Keychain is locked.");
    return privkey_;
}

secure_bytes_t Keychain::seed() const
{
    if (!isPrivate()) throw std::runtime_error("Keychain is nonprivate.");
    if (privkey_.empty()) throw std::runtime_error("Keychain is locked.");
    if (seed_ciphertext_.empty()) throw std::runtime_error("Keychain has no seed.");
    return seed_;
}

bool Keychain::hasSeed() const
{
    return !seed_ciphertext_.empty();
}

void Keychain::importPrivateKey(const Keychain& source)
{
    privkey_ciphertext_ = source.privkey_ciphertext_;
    privkey_salt_ = source.privkey_salt_;
}

void Keychain::importBIP32(const secure_bytes_t& extkey, const secure_bytes_t& lock_key)
{
    Coin::HDKeychain hdKeychain(extkey);

    depth_ = (uint32_t)hdKeychain.depth();
    parent_fp_ = hdKeychain.parent_fp();
    child_num_ = hdKeychain.child_num();
    chain_code_ = hdKeychain.chain_code();
    privkey_.clear();
    if (hdKeychain.isPrivate())
    {
        if (!lock_key.empty())
        {
            privkey_salt_ = AES::random_salt();
            privkey_ciphertext_ = AES::encrypt(lock_key, hdKeychain.key(), true, privkey_salt_);
        }
        else
        {
            privkey_salt_ = 0;
            privkey_ciphertext_ = hdKeychain.key();
        }
    }
    else
    {
        privkey_salt_ = 0;
        privkey_ciphertext_.clear();
    }

    pubkey_ = hdKeychain.pubkey();
    hash_ = hdKeychain.full_hash();
}

secure_bytes_t Keychain::exportBIP32(bool export_private) const
{
    secure_bytes_t key;

    if (export_private)
    {
        if (!isPrivate()) throw std::runtime_error("Keychain is nonprivate.");
        if (isLocked()) throw std::runtime_error("Keychain is locked.");

        // Remove initial zero from privkey if necessary
        key = (privkey_.size() > 32) ? secure_bytes_t(privkey_.begin() + 1, privkey_.end()) : privkey_;
    }
    else
    {
        key = pubkey_;
    }

    return Coin::HDKeychain(key, chain_code_, child_num_, parent_fp_, depth_).extkey();
}

void Keychain::clearPrivateKey()
{
    privkey_.clear();
    privkey_ciphertext_.clear();
    privkey_salt_ = 0;

    seed_.clear();
    seed_ciphertext_.clear();
    seed_salt_ = 0;
}


/*
 * class Key
 */

Key::Key(const std::shared_ptr<Keychain>& keychain, uint32_t index, bool compressed)
{
    root_keychain_ = keychain->root();
    derivation_path_ = keychain->derivation_path();
    index_ = index;

    pubkey_ = keychain->getSigningPublicKey(index_, compressed);
    updatePrivate();
}

secure_bytes_t Key::privkey() const
{
    if (!is_private_ || root_keychain_->isLocked()) return secure_bytes_t();
    return root_keychain_->getSigningPrivateKey(index_, derivation_path_);
}

// and a version that throws exceptions
secure_bytes_t Key::try_privkey() const
{
    if (!is_private_) throw std::runtime_error("Key::privkey - cannot get private key from nonprivate key object.");
    if (root_keychain_->isLocked()) throw std::runtime_error("Key::privkey - private key is locked.");

    return root_keychain_->getSigningPrivateKey(index_, derivation_path_);
}


/*
 * class Account
 */

Account::Account()
{
    scripttemplatesloaded_ = false;
}

Account::Account(const std::string& name, unsigned int minsigs, const KeychainSet& keychains, uint32_t unused_pool_size, uint32_t time_created, bool compressed_keys, bool use_witness, bool use_witness_p2sh)
    : name_(name), minsigs_(minsigs), keychains_(keychains), unused_pool_size_(unused_pool_size), time_created_(time_created), compressed_keys_(compressed_keys), use_witness_(use_witness), use_witness_p2sh_(use_witness_p2sh)
{
    if (!use_witness_) { use_witness_p2sh_ = false; }

    scripttemplatesloaded_ = false;

    // TODO: Use exception classes
    if (name_.empty() || name[0] == '@') throw std::runtime_error("Invalid account name.");
    if (keychains_.size() > 15) throw std::runtime_error("Account can use at most 15 keychains.");
    if (minsigs > keychains_.size()) throw std::runtime_error("Account minimum signatures cannot exceed number of keychains.");
    if (minsigs < 1) throw std::runtime_error("Account must require at least one signature.");

    initScriptPatterns();
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

    if (!compressed_keys_) { data.push_back(0x00); }
    if (use_witness_)
    {
        if (use_witness_p2sh_)  { data.push_back(0x03); }
        else                    { data.push_back(0x01); }
    }

    hash_ = ripemd160(sha256(data));
}

void Account::initScriptPatterns()
{
    using namespace CoinQ::Script;

    uchar_vector redeempattern;
    redeempattern << (OP_1_OFFSET + minsigs_);
    for (unsigned int k = 0; k < keychains_.size(); k++)
    {
        redeempattern << OP_PUBKEY << k;
    }
    redeempattern << (OP_1_OFFSET + keychains_.size()) << OP_CHECKMULTISIG;
    redeempattern_ = redeempattern;
}

void Account::loadScriptTemplates()
{
    if (scripttemplatesloaded_) return;
    redeemtemplate_.pattern(redeempattern_);
    scripttemplatesloaded_ = true;
}


AccountInfo Account::accountInfo() const
{
    std::vector<std::string> keychain_names;
    for (auto& keychain: keychains_) { keychain_names.push_back(keychain->name()); }

    std::vector<std::string> bin_names;
    uint32_t issued_script_count = 0;
    for (auto& bin: bins_)
    {
        bin_names.push_back(bin->name());
        if (bin->next_script_index() > 0)
        {
            issued_script_count += (bin->next_script_index() - 1);
        }
    }

    return AccountInfo(id_, name_, minsigs_, keychain_names, issued_script_count, unused_pool_size_, time_created_, bin_names, compressed_keys_, use_witness_, use_witness_p2sh_);
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

SigningScriptVector AccountBin::generateSigningScripts()
{
    SigningScriptVector signingscripts;
    for (uint32_t i = 0; i < next_script_index_; i++)
    {
        std::string label;
        auto it = script_label_map_.find(i);
        if (it != script_label_map_.end())   { label = it->second; }
        SigningScript::status_t status = (index_ == CHANGE_INDEX) ? SigningScript::CHANGE : SigningScript::ISSUED;
        std::shared_ptr<SigningScript> signingscript(new SigningScript(shared_from_this(), i, label, status));
        signingscripts.push_back(signingscript);
    }

    script_count_ = next_script_index_ + unused_pool_size();
    for (uint32_t i = next_script_index_; i < script_count_; i++)
    {
        std::shared_ptr<SigningScript> signingscript(new SigningScript(shared_from_this(), i));
        signingscripts.push_back(signingscript);
    }

    return signingscripts;
}

uint32_t AccountBin::unused_pool_size() const
{
    std::shared_ptr<Account> account = account_.lock();
    return account ? account->unused_pool_size() : DEFAULT_UNUSED_POOL_SIZE;
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

void AccountBin::setScriptLabel(uint32_t index, const std::string& label)
{
    if (!label.empty()) { script_label_map_[index] = label; }
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
    if (!account_) throw std::runtime_error("SigningScript::SigningScript() - account is null.");

    auto& keychains = account_bin_->keychains();
    for (auto& keychain: keychains)
    {
        std::shared_ptr<Key> key(new Key(keychain, index, account_->compressed_keys()));
        keys_.push_back(key);
    }

    // sort keys into canonical order
    std::sort(keys_.begin(), keys_.end(), [](std::shared_ptr<Key> key1, std::shared_ptr<Key> key2) { return key1->pubkey() < key2->pubkey(); });

    std::vector<uchar_vector> pubkeys;
    for (auto& key: keys_) { pubkeys.push_back(key->pubkey()); }

    account_->loadScriptTemplates();
    redeemscript_ = account_->redeemtemplate().script(pubkeys);
    if (redeemscript_.empty()) throw std::runtime_error("SigningScript::SigningScript() - redeemscript is empty.");

    using namespace CoinQ::Script;

    if (account_->use_witness())
    {
        WitnessProgram_P2WSH wp(redeemscript_);
        if (account_->use_witness_p2sh())
        {
            txinscript_ = pushStackItem(wp.script());
            txoutscript_ = wp.p2shscript();
        }
        else
        {
            txoutscript_ = wp.script();
        }
    }
    else
    {
        uchar_vector txinscript, txoutscript;

        txinscript << OP_0;
        for (std::size_t k = 0; k < pubkeys.size(); k++) { txinscript << OP_0; }
        txinscript << pushStackItem(redeemscript_);
        txinscript_ = txinscript;

        txoutscript << OP_HASH160 << pushStackItem(hash160(redeemscript_)) << OP_EQUAL;
        txoutscript_ = txoutscript;
    }

    account_bin_->setScriptLabel(index, label);
}

void SigningScript::label(const std::string& label)
{
    label_ = label;
    account_bin_->setScriptLabel(index_, label);
}

void SigningScript::status(status_t status)
{
    status_ = status;
    if (status > UNUSED) account_bin_->markSigningScriptIssued(index_);
}

void SigningScript::markUsed()
{
	if (account_bin_->index() == AccountBin::CHANGE_INDEX)
	{
		status_ = CHANGE;
	}
	else
	{
		status_ = USED;
	}

	account_bin_->markSigningScriptIssued(index_);
}


/*
 * class BlockHeader
 */

void BlockHeader::fromCoinCore(const Coin::CoinBlockHeader& blockheader, uint32_t height)
{
    hash_ = blockheader.hash();
    height_ = height;
    version_ = blockheader.version();
    prevhash_ = blockheader.prevBlockHash();
    merkleroot_ = blockheader.merkleRoot();
    timestamp_ = blockheader.timestamp();
    bits_ = blockheader.bits();
    nonce_ = blockheader.nonce();
}

Coin::CoinBlockHeader BlockHeader::toCoinCore() const
{
    return Coin::CoinBlockHeader(version_, timestamp_, bits_, nonce_, prevhash_, merkleroot_);
}

std::string BlockHeader::toJson() const
{
    std::stringstream ss;
    ss << "{"
       << "\"hash\":\"" << uchar_vector(hash_).getHex() << "\","
       << "\"height\":" << height_ << ","
       << "\"version\":" << version_ << ","
       << "\"prevhash\":\"" << uchar_vector(prevhash_).getHex() << "\","
       << "\"merkleroot\":\"" << uchar_vector(merkleroot_).getHex() << "\","
       << "\"timestamp\":" << timestamp_ << ","
       << "\"bits\":" << bits_ << ","
       << "\"nonce\":" << nonce_
       << "}";
    return ss.str();
}

void BlockHeader::updateHash()
{
    hash_ = Coin::CoinBlockHeader(version_, timestamp_, bits_, nonce_, prevhash_, merkleroot_).hash();
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

std::string MerkleBlock::toJson() const
{
    std::stringstream ss;
    ss << "{"
       << "\"header\":" << blockheader_->toJson() << ","
       << "\"txcount\":" << txcount_ << ","
       << "\"hashes\":[";
    bool addComma = false;
    for (auto& hash: hashes_)
    {
        if (addComma)   { ss << ","; }
        else            { addComma = true; }
        ss << "\"" << uchar_vector(hash).getHex() << "\""; 
    }
    ss << "],"
       << "\"flags\":\"" << uchar_vector(flags_).getHex() << "\""
       << "}";
    return ss.str();
}


/*
 * class TxIn
 */

TxIn::TxIn(const Coin::TxIn& coin_txin)
{
    fromCoinCore(coin_txin);
}

TxIn::TxIn(const bytes_t& raw)
{
    Coin::TxIn coin_txin(raw);
    fromCoinCore(coin_txin);
}

void TxIn::fromCoinCore(const Coin::TxIn& coin_txin)
{
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

bytes_t TxIn::unsigned_script() const
{
    using namespace CoinQ::Script;

//LOGGER(trace) << "TxIn::unsigned_script: Calling SignableTxIn constructor" << std::endl;
    SignableTxIn signabletxin(tx()->toCoinCore(), txindex_, outpoint() ? outpoint()->value() : 0);
//LOGGER(trace) << "TxIn::unsigned_script: Calling signabletxin.clearsigs()" << std::endl;
    signabletxin.clearsigs();
//LOGGER(trace) << "TxIn::unsigned_script: Returning signabletxin.txinscript()" << std::endl;
    return signabletxin.txinscript();
}

bytes_t TxIn::raw() const
{
    return toCoinCore().getSerialized();
}

void TxIn::outpoint(std::shared_ptr<TxOut> outpoint)
{
    outpoint_ = outpoint;
    if (!outpoint) return;

    std::shared_ptr<Tx> tx = outpoint->tx();
    // TODO: Tx exception class
    if (outindex_ != outpoint->txindex())
        throw std::runtime_error("TxIn::outpoint - incorrect outindex.");
    if (!tx)
        throw std::runtime_error("TxIn::outpoint - tx is null.");
    if (tx->status() == Tx::UNSIGNED)
        throw std::runtime_error("TxIn::outpoint - tx is missing signatures.");
    if (outhash_ != tx->hash())
        throw std::runtime_error("TxIn::outpoint - incorrect outhash.");
}

std::string TxIn::toJson() const
{
    std::stringstream ss;
    ss << "{"
       << "\"outhash\":\"" << uchar_vector(outhash_).getHex() << "\","
       << "\"outindex\":" << outindex_ << ","
       << "\"script\":\"" << uchar_vector(script_).getHex() << "\","
       << "\"sequence\":" << sequence_
       << "}";
    return ss.str();
}

static std::vector<bytes_t> emptyScriptInputs(std::size_t n)
{
    std::vector<bytes_t> scriptinputs;
    for (std::size_t k = 0; k < n; k++) { scriptinputs.push_back(bytes_t()); }
    return scriptinputs;
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
    : value_(value), status_(UNSPENT)
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
    if (receiving_label_.empty()) { receiving_label_ = signingscript->label(); }
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

std::string TxOut::toJson() const
{
    std::stringstream ss;
    ss << "{"
       << "\"value\":" << value_ << ","
       << "\"script\":\"" << uchar_vector(script_).getHex() << "\","
       << "\"sending_label\":\"" << sending_label_ << "\","
       << "\"receiving_label\":\"" << receiving_label_ << "\"";

    if (signingscript_ && signingscript_->contact())
    {
        ss << ",\"sender_username\":\"" << signingscript_->contact()->username() << "\"";
    }

    ss << "}";
    return ss.str(); 
}


/*
 * class Tx
 */

// static
std::string Tx::getStatusString(int status, bool lowercase)
{
    std::vector<std::string> flags;
    if (status & UNSIGNED) flags.push_back(lowercase ? "unsigned" : "UNSIGNED");
    if (status & UNSENT) flags.push_back(lowercase ? "unsent" : "UNSENT");
    if (status & SENT) flags.push_back(lowercase ? "send" : "SENT");
    if (status & PROPAGATED) flags.push_back(lowercase ? "propagated" : "PROPAGATED");
    if (status & CANCELED) flags.push_back(lowercase ? "canceled" : "CANCELED");
    if (status & CONFIRMED) flags.push_back(lowercase ? "confirmed" : "CONFIRMED");
    if (flags.empty()) return lowercase ? "no_status" : "NO_STATUS";
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
    if (status & CANCELED) flags.push_back(CANCELED);
    if (status & CONFIRMED) flags.push_back(CONFIRMED);
    return flags;
}

void Tx::set(uint32_t version, const txins_t& txins, const txouts_t& txouts, uint32_t locktime, uint32_t timestamp, status_t status, bool conflicting, bool checksigs)
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

    if (checksigs && missingSigCount()) { status_ = UNSIGNED; }
    else                                { status_ = status; hash_ = coin_tx.hash(); }

    conflicting_ = conflicting;

    coin_tx.clearScriptSigs();
    unsigned_hash_ = coin_tx.hash();
    updateTotals();
}

void Tx::set(Coin::Transaction coin_tx, uint32_t timestamp, status_t status, bool conflicting, bool checksigs)
{
    //LOGGER(trace) << "Tx::set - fromCoinCore(coin_tx);" << std::endl;
    fromCoinCore(coin_tx);

    timestamp_ = timestamp;

    if (checksigs && missingSigCount()) { status_ = UNSIGNED; }
    else                                { status_ = status; hash_ = coin_tx.hash(); }

    conflicting_ = conflicting;

    coin_tx.clearScriptSigs();
    unsigned_hash_ = coin_tx.hash();
    updateTotals();
}

void Tx::set(const bytes_t& raw, uint32_t timestamp, status_t status, bool conflicting, bool checksigs)
{
    Coin::Transaction coin_tx(raw);
    fromCoinCore(coin_tx);
    timestamp_ = timestamp;

    if (checksigs && missingSigCount()) { status_ = UNSIGNED; }
    else                                { status_ = status; hash_ = coin_tx.hash(); }

    conflicting_ = conflicting;

    coin_tx.clearScriptSigs();
    unsigned_hash_ = coin_tx.hash();
    updateTotals();
}

bool Tx::updateStatus(status_t status /* = NO_STATUS */, bool checksigs)
{
    // Tx is not signed.
    if (checksigs && missingSigCount())
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

        hash_ = toCoinCore().hash();
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
    for (auto& txin: txins_)
    {
        coin_tx.inputs.push_back(txin->toCoinCore());
        Coin::TxIn& input = coin_tx.inputs.back();
        for (auto& item: txin->scriptwitnessstack()) { input.scriptWitness.push(item); }
    }

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

bytes_t Tx::raw(bool withWitness) const
{
    return toCoinCore().getSerialized(withWitness);
}

void Tx::updateTotals()
{
    have_all_outpoints_ = true;
    txin_total_ = 0;
    for (auto& txin: txins_)
    {
        std::shared_ptr<TxOut> outpoint = txin->outpoint();
        if (!outpoint)
        {
            have_all_outpoints_ = false;
        }
        else
        {
            txin_total_ += outpoint->value();
        }
    }

    txout_total_ = 0;
    for (auto& txout: txouts_)
    {
        txout_total_ += txout->value(); 
    }
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
        txin->txindex(i);

        std::vector<bytes_t> stack;
        for (auto& item: coin_txin.scriptWitness.stack) { stack.push_back(item); } 
        txin->scriptwitnessstack(stack);

        txins_.push_back(txin);
        i++;
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
    Coin::Transaction cointx(toCoinCore());
    for (auto& txin: txins_)
    {
        uint64_t outpointvalue = txin->outpoint() ? txin->outpoint()->value() : 0;
        SignableTxIn signabletxin(cointx, txin->txindex(), outpointvalue);
        unsigned int sigsneeded = signabletxin.sigsneeded();
        if (sigsneeded > count) count = sigsneeded;
    }
    return count;
}

std::set<bytes_t> Tx::missingSigPubkeys() const
{
    using namespace CoinQ::Script;
    std::set<bytes_t> pubkeys;
    Coin::Transaction cointx = toCoinCore();
    for (auto& txin: txins_)
    {
        uint64_t outpointvalue = txin->outpoint() ? txin->outpoint()->value() : 0;
        SignableTxIn signabletxin(cointx, txin->txindex(), outpointvalue);
        std::vector<bytes_t> txinpubkeys = signabletxin.missingsigs();
        for (auto& txinpubkey: txinpubkeys) { pubkeys.insert(txinpubkey); }
    } 
    return pubkeys;
}

std::set<bytes_t> Tx::presentSigPubkeys() const
{
    std::set<bytes_t> pubkeys;

    using namespace CoinQ::Script;
    Signer signer_(signer());
    for (auto& signabletxin: signer_.getSignableTxIns())
    {
        std::vector<bytes_t> txinpubkeys = signabletxin.presentsigs();
        for (auto& txinpubkey: txinpubkeys) { pubkeys.insert(txinpubkey); }
    } 
    return pubkeys;
}

CoinQ::Script::Signer Tx::signer() const
{
    using namespace CoinQ::Script;
    std::vector<uint64_t> outpointvalues;
    for (auto& txin: txins_)
    {
        outpointvalues.push_back(txin->outpoint() ? txin->outpoint()->value() : 0);
    }

    return Signer(toCoinCore(), outpointvalues);
}

std::string Tx::toJson(bool includeRawHex, bool includeSerialized) const
{
    std::stringstream ss;
    ss << "{"
       << "\"version\":" << version_ << ","
       << "\"locktime\":" << locktime_ << ","
       << "\"hash\":\"" << uchar_vector(hash()).getHex() << "\","
       << "\"unsignedhash\":\"" << uchar_vector(unsigned_hash()).getHex() << "\","
       << "\"status\":\"" << getStatusString(status_) << "\","
       << "\"height\":";
    if (blockheader_)   { ss << blockheader_->height(); }
    else                { ss << "null"; }
    ss << ","
       << "\"txins\":[";
    bool addComma = false;
    for (auto& txin: txins_)
    {
        if (addComma)   { ss << ","; }
        else            { addComma = true; }
        ss << txin->toJson();
    }
    ss << "],"
       << "\"txouts\":[";
    addComma = false;
    for (auto& txout: txouts_)
    {
        if (addComma)   { ss << ","; }
        else            { addComma = true; }
        ss << txout->toJson();
    }
    ss << "]";

    if (includeRawHex)
    {
        ss << ",\"rawtx\":\"" << uchar_vector(raw()).getHex() << "\"";
    }

    if (includeSerialized)
    {
        ss << ",\"serializedtx\":\"" << toSerialized() << "\"";
    }

    ss << "}";
    return ss.str();
}

std::string Tx::toSerialized() const
{
    std::stringstream ss;
    boost::archive::text_oarchive oa(ss);
    oa << *this;
    return ss.str();
}

void Tx::fromSerialized(const std::string& serialized)
{
    std::stringstream ss;
    ss << serialized;
    boost::archive::text_iarchive ia(ss);
    ia >> *this;
}

