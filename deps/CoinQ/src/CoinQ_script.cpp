///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_script.cpp
//
// Copyright (c) 2013-2016 Eric Lombrozo
//
// All Rights Reserved.

#include "CoinQ_script.h"

#include <CoinCore/Base58Check.h>
#include <CoinCore/secp256k1_openssl.h>

using namespace CoinCrypto;

namespace CoinQ {
namespace Script {

uchar_vector opPushData(uint32_t nBytes)
{
    uchar_vector rval;
    if (nBytes <= 0x4b) {
        rval.push_back((unsigned char)nBytes);
    }
    else if (nBytes <= 0xff) {
        rval.push_back(0x4c);
        rval.push_back((unsigned char)nBytes);
    }
    else if (nBytes <= 0xffff) {
        rval.push_back(0x4d);
        rval.push_back((unsigned char)(nBytes & 0xff));
        rval.push_back((unsigned char)(nBytes >> 8));
    }
    else {
        rval.push_back(0x4e);
        rval.push_back((unsigned char)(nBytes & 0xff));
        rval.push_back((unsigned char)((nBytes >> 8) & 0xff));
        rval.push_back((unsigned char)((nBytes >> 16) & 0xff));
        rval.push_back((unsigned char)(nBytes >> 24));
    }

    return rval;
}

uchar_vector pushStackItem(const uchar_vector& data)
{
    uchar_vector rval;
    rval += opPushData(data.size());
    rval += data;
    return rval;
}

uint32_t getDataLength(const uchar_vector& script, uint& pos)
{
    if (pos >= script.size()) {
        throw std::runtime_error("Script pos past end.");
    }

    unsigned char op = script[pos++];
    if (op == OP_0 || op == OP_1NEGATE || (op >= OP_1 && op <= OP_16)) return 0;
    if (op <= 0x4b) {
        return (uint32_t)op;
    }
    else if (op == 0x4c) {
        if (pos >= script.size()) {
            throw std::runtime_error("Script pos past end.");
        }
        return (uint32_t)script[pos++];
    }
    else if (op == 0x4d) {
        if (pos + 1 >= script.size()) {
            throw std::runtime_error("Script pos past end.");
        }
        uint32_t rval = (uint32_t)script[pos++];
        rval         += (uint32_t)script[pos++] << 8;;
        return rval;
    }
    else if (op == 0x4e) {
        if (pos + 3 >= script.size()) {
            throw std::runtime_error("Script pos past end.");
        }
        uint32_t rval = (uint32_t)script[pos++];
        rval         += (uint32_t)script[pos++] << 8;
        rval         += (uint32_t)script[pos++] << 16;
        rval         += (uint32_t)script[pos++] << 24;
        return rval;
    }
    else {
        throw std::runtime_error("Operation is not push data.");
    }
}

uchar_vector getNextOp(const bytes_t& script, uint& pos, bool pushdataonly)
{
    if (script.size() < pos)
        throw std::runtime_error("Script pos past end.");

    if (script.size() == pos)
        return uchar_vector();

    uint start = pos;
    unsigned char op = script[pos++];
    if (op >= OP_1 && op <= OP_16)
    {
        uchar_vector rval;
        rval.push_back(op - OP_1_OFFSET);
        return rval;
    }
    else if (op <= 0x4b)
    {
        pos += op;
        if (pushdataonly) { start += 1; }
    }
    else if (op == 0x4c)
    {
        if (script.size() < pos + 1)
            throw std::runtime_error("Unexpected end of script.");

        uint32_t len = script[pos++];
        pos += len;
        if (pushdataonly) { start += 2; }
    }
    else if (op == 0x4d)
    {
        if (script.size() < pos + 2)
            throw std::runtime_error("Unexpected end of script.");

        uint32_t len =  (uint32_t)script[pos++] |
                        (uint32_t)script[pos++] << 8;

        pos += len;
        if (pushdataonly) { start += 3; }
    }
    else if (op == 0x4e)
    {
        if (script.size() < pos + 4)
            throw std::runtime_error("Unexpected end of script.");

        uint32_t len =  (uint32_t)script[pos++]         |
                        (uint32_t)script[pos++] << 8    |
                        (uint32_t)script[pos++] << 16   |
                        (uint32_t)script[pos++] << 24;

        pos += len;
        if (pushdataonly) { start += 5; }
    }
    else if (pushdataonly)
    {
        return uchar_vector();
    }

    if (script.size() < pos)
        throw std::runtime_error("Script pos past end.");

    return uchar_vector(script.begin() + start, script.begin() + pos);
}

payee_t getScriptPubKeyPayee(const uchar_vector& scriptPubKey)
{
    if (scriptPubKey.size()   == 25 &&
        scriptPubKey[0]       == 0x76 &&
        scriptPubKey[1]       == 0xa9 &&
        scriptPubKey[2]       == 0x14 &&
        scriptPubKey[23]      == 0x88 &&
        scriptPubKey[24]      == 0xac)
    {
        return std::make_pair(SCRIPT_PUBKEY_PAY_TO_PUBKEY_HASH, uchar_vector(scriptPubKey.begin() + 3, scriptPubKey.begin() + 23));
    }

    if (scriptPubKey.size()   == 23 &&
        scriptPubKey[0]       == 0xa9 &&
        scriptPubKey[1]       == 0x14 &&
        scriptPubKey[22]      == 0x87)
    {
        return std::make_pair(SCRIPT_PUBKEY_PAY_TO_SCRIPT_HASH, uchar_vector(scriptPubKey.begin() + 2, scriptPubKey.begin() + 22));
    }

    uint pos = 0;
    uint32_t dataLength = getDataLength(scriptPubKey, pos);
    if (pos + dataLength + 1== scriptPubKey.size() &&
        scriptPubKey[pos + dataLength + 1] == 0xac)
    {
        return std::make_pair(SCRIPT_PUBKEY_PAY_TO_PUBKEY, uchar_vector(scriptPubKey.begin() + pos, scriptPubKey.begin() + pos + dataLength));
    }

    if (scriptPubKey.size() == 0)
    {
        return std::make_pair(SCRIPT_PUBKEY_EMPTY_SCRIPT, uchar_vector());
    }
 
    return std::make_pair(SCRIPT_PUBKEY_UNKNOWN_TYPE, uchar_vector());
}

bool isValidAddress(const std::string& address, const unsigned char addressVersions[])
{
    uchar_vector hash;
    unsigned int version;
    return fromBase58Check(address, hash, version) && (hash.size() == 20) && (version == addressVersions[0] || version == addressVersions[1]);
}

uchar_vector getTxOutScriptForAddress(const std::string& address, const unsigned char addressVersions[])
{
    uchar_vector hash;
    unsigned int version;
    if (!fromBase58Check(address, hash, version)) {
        throw std::runtime_error("Invalid address checksum.");
    }

    uchar_vector script;
    if (version == addressVersions[0]) {
        if (hash.size() != 20) {
            throw std::runtime_error("Invalid pubkey hash length.");
        }
        script.push_back(0x76); // OP_PUSHDATA1
        script.push_back(0xa9); // OP_HASH160
        script.push_back(0x14); // push a 20 byte hash
        script += hash;
        script.push_back(0x88); // OP_EQUALVERIFY
        script.push_back(0xac); // OP_CHECKSIG
    }
    else if (version == addressVersions[1]) {
        if (hash.size() != 20) {
            throw std::runtime_error("Invalid script hash length.");
        }
        script.push_back(0xa9); // OP_HASH160
        script.push_back(0x14); // push a 20 byte hash
        script += hash;
        script.push_back(0x87); // OP_EQUAL
    }
    else {
        throw std::runtime_error("Invalid address version.");
    }

    return script;
}

std::string getAddressForTxOutScript(const bytes_t& txoutscript, const unsigned char addressVersions[])
{
    payee_t payee = getScriptPubKeyPayee(txoutscript);
    switch (payee.first) {
    case SCRIPT_PUBKEY_PAY_TO_PUBKEY_HASH:
        return toBase58Check(payee.second, addressVersions[0]);

    case SCRIPT_PUBKEY_PAY_TO_SCRIPT_HASH:
        return toBase58Check(payee.second, addressVersions[1]);

    default:
        return "N/A";
    }
}


/*
 * class SymmetricHDKeyGroup
*/
SymmetricHDKeyGroup::SymmetricHDKeyGroup(const std::vector<uchar_vector>& extkeys)
{
    for (auto& extkey: extkeys)
    {
        keychains_.push_back(Coin::HDKeychain(extkey));
    }

    index_ = 1; // Reserve 0 for nesting
    update();
}

void SymmetricHDKeyGroup::update()
{
    pubkeys_.clear();
    for (auto& keychain: keychains_)
    {
        pubkeys_.push_back(keychain.getChild(index_).pubkey());
    }
    sort();
}


/*
 * class ScriptTemplate
*/
const uchar_vector& ScriptTemplate::script() const
{
    return reduced_.empty() ? pattern_ : reduced_;
}

uchar_vector ScriptTemplate::script(const uchar_vector& token) const
{
    std::vector<uchar_vector> tokens;
    tokens.push_back(token);
    return script(tokens);
}

uchar_vector ScriptTemplate::script(const std::vector<uchar_vector>& tokens) const
{
    const uchar_vector& pattern = script();

    uchar_vector rval;

    uint pos = 0;
    while (true)
    {   
        uchar_vector fullop = getNextOp(pattern, pos);
        if (fullop.empty()) break;

        if (fullop[0] == OP_TOKEN || fullop[0] == OP_TOKENHASH)
        {
            if (pos >= pattern.size())
                throw std::runtime_error("Invalid pattern.");

            std::size_t i = pattern[pos++];
            if (i >= tokens.size())
            {
                // Reduce indices so we can populate later
                rval << fullop[0] << (i - tokens.size());
                continue;
            }

            switch (fullop[0])
            {
            case OP_TOKEN:
                rval << pushStackItem(tokens[i]);
                break;
            case OP_TOKENHASH:
                rval << pushStackItem(hash160(tokens[i]));
                break;
            default:
                break;
            }
        }
        else
        {
            rval << fullop;
        }
    }

    return rval;
}

ScriptTemplate& ScriptTemplate::apply(const uchar_vector& token)
{
    reduced_ = script(token);
    return *this;
}

ScriptTemplate& ScriptTemplate::apply(const std::vector<uchar_vector>& tokens)
{
    reduced_ = script(tokens);
    return *this;
}

ScriptTemplate& ScriptTemplate::reset()
{
    reduced_.clear();
    return *this;
}
    

/*
 * class WitnessProgram
*/
std::string WitnessProgram::address(const unsigned char addressVersions[]) const
{
    return toBase58Check(hash160(witnessscript_), addressVersions[1]);
}

void WitnessProgram::update()
{
    witnessscript_.clear();
    txinscript_.clear();
    txoutscript_.clear();

    switch (version())
    {
    case 0:
        witnessscript_ << OP_0 << pushStackItem(redeemscript_);
        break;
    case 1:
        witnessscript_ << OP_1 << pushStackItem(sha256(redeemscript_));
        break;
    default:
        break;
    }

    txinscript_ << pushStackItem(witnessscript_);
    txoutscript_ << OP_HASH160 << pushStackItem(hash160(witnessscript_)) << OP_EQUAL;
}

WitnessProgram::version_t WitnessProgram::getWitnessVersion(const uchar_vector& txinscript)
{
    uint pos = 0;
    uchar_vector fullop = getNextOp(txinscript, pos);
    if (fullop.size() < 3)
        return NO_WITNESS;

    if (fullop[0] < 34 && fullop[1] == OP_0 && fullop[2] == fullop.size() - 3)
        return WITNESS_V0;

    if (fullop[0] == 34 && fullop[1] == OP_1 && fullop[2] == fullop.size() - 3)
        return WITNESS_V1;

    return NO_WITNESS;
}

/*
 * class Script
*/
Script::Script(type_t type, unsigned int minsigs, const std::vector<bytes_t>& pubkeys, const std::vector<bytes_t>& sigs)
    : type_(type), minsigs_(minsigs), pubkeys_(pubkeys), sigs_(sigs)
{
    if (type == PAY_TO_PUBKEY_HASH || type == PAY_TO_PUBKEY) {
        if (minsigs != 1 || pubkeys.size() != 1 || sigs.size() > 1) {
            throw std::runtime_error("Invalid parameters.");
        }
        if (sigs_.empty()) { sigs_.push_back(bytes_t()); }
        hash_ = ripemd160(sha256(pubkeys[0]));
    }

    if (type == PAY_TO_MULTISIG_SCRIPT_HASH) {
        if (minsigs == 0 || pubkeys.size() > 16 || minsigs > pubkeys.size() || sigs.size() > pubkeys.size()) {
            throw std::runtime_error("Invalid parameters.");
        }
        if (sigs_.empty()) { sigs_.assign(pubkeys.size(), bytes_t()); }
        uchar_vector redeemscript;
        redeemscript.push_back(minsigs + 0x50);
        for (auto& pubkey: pubkeys_) {
           redeemscript += opPushData(pubkey.size());
           redeemscript += pubkey;
        }
        redeemscript.push_back(pubkeys_.size() + 0x50);
        redeemscript.push_back(0xae); // OP_CHECKMULTISIG
        redeemscript_ = redeemscript;
        hash_ = ripemd160(sha256(redeemscript));
    }
        
}

Script::Script(const bytes_t& txinscript, const bytes_t& signinghash, bool clearinvalidsigs)
{
    std::vector<bytes_t> objects;
    unsigned int pos = 0;
    while (pos < txinscript.size())
    {
        std::size_t size = getDataLength(txinscript, pos);
        if (pos + size > txinscript.size()) throw std::runtime_error("Push operation exceeds data size.");
        objects.push_back(bytes_t(txinscript.begin() + pos, txinscript.begin() + pos + size));
        pos += size;
    }

    if (objects.size() == 2)
    {
        type_ = PAY_TO_PUBKEY_HASH;
        pubkeys_.push_back(objects[1]);
        minsigs_ = 1;
        sigs_.push_back(objects[0]);
        hash_ = ripemd160(sha256(pubkeys_[0]));
    }
    else if (objects.size() >= 3 && objects[0].empty())
    {
        // Parse redeemscript
        bytes_t redeemscript = objects.back();
        if (redeemscript.size() < 3)
        {
            type_ = UNKNOWN;
            return;
            //throw std::runtime_error("Redeem script is too short.");
        }

        // Get minsigs. Size opcode is offset by 0x50.
        unsigned char byte = redeemscript[0];
        if (byte < 0x51 || byte > 0x60)
        {
            type_ = UNKNOWN;
            return;
            //throw std::runtime_error("Invalid signature minimum.");
        }
        minsigs_ = byte - 0x50;

        unsigned char numkeys = 0;
        unsigned int pos = 1;
        while (true)
        {
            byte = redeemscript[pos++];
            if (pos >= redeemscript.size())
            {
                type_ = UNKNOWN;
                return;
                //throw std::runtime_error("Script terminates prematurely.");
            }

            if (byte >= 0x51 && byte <= 0x60)
            {
                // Interpret byte as signature counter.
                if (byte - 0x50 != numkeys)
                {
                    type_ = UNKNOWN;
                    return;
                    //throw std::runtime_error("Invalid public key count.");
                }

                if (numkeys < minsigs_)
                {
                    type_ = UNKNOWN;
                    return;
                    //throw std::runtime_error("The required signature minimum exceeds the number of keys.");
                }

                // Redeemscript must terminate with OP_CHECKMULTISIG
                if (redeemscript[pos++] != 0xae || pos > redeemscript.size())
                {
                    type_ = UNKNOWN;
                    return;
                    //throw std::runtime_error("Invalid script termination.");
                }

                break;
            }
            // Interpret byte as pubkey size
            if (byte > 0x4b || pos + byte > redeemscript.size())
            {
                type_ = UNKNOWN;
                return;
/*
                std::stringstream err;
                err << "Invalid OP at byte " << pos - 1 << ".";
                throw std::runtime_error(err.str());
*/
            }
            numkeys++;
            if (numkeys > 16) throw std::runtime_error("Public key maximum of 16 exceeded.");

            pubkeys_.push_back(bytes_t(redeemscript.begin() + pos, redeemscript.begin() + pos + byte));
            pos += byte;
        }

        type_ = PAY_TO_MULTISIG_SCRIPT_HASH;
        std::vector<bytes_t> sigs;
        sigs.assign(objects.begin() + 1, objects.end() - 1);
        if (sigs.size() > pubkeys_.size()) throw std::runtime_error("Too many signatures.");

        if (signinghash.empty())
        {
            sigs_ = sigs;
        }
        else
        {
            // Validate signatures.
            sigs_.clear();
            unsigned int iSig = 0;
            unsigned int nValidSigs = 0;
            for (auto& pubkey: pubkeys_)
            {
                // If we already have enough valid signatures or there are no more signatures or signature is a placeholder
                if (nValidSigs == minsigs_ || iSig >= sigs.size() || sigs[iSig].empty())
                {
                    // Add or keep placeholder.
                    sigs_.push_back(bytes_t());
                    iSig++;
                }
                else
                {
                    // TODO: add support for other hash types.
                    if (sigs[iSig].back() != SIGHASH_ALL) throw std::runtime_error("Unsupported hash type.");

                    // Remove hash type byte.
                    bytes_t signature(sigs[iSig].begin(), sigs[iSig].end() - 1);

                    // Verify signature.
                    secp256k1_key key;
                    key.setPubKey(pubkey);
                    if (secp256k1_verify(key, signinghash, signature))
                    {
                        // Signature is valid. Keep it.
                        sigs_.push_back(sigs[iSig]);
                        iSig++;
                        nValidSigs++;
                    }
                    else
                    {
                        // Signature is invalid. Add placeholder for this pubkey and test it for next pubkey
                        sigs_.push_back(bytes_t());
                    }
                }
            }
            if (!clearinvalidsigs && iSig < sigs.size()) throw std::runtime_error("Invalid signature.");
        }
        redeemscript_ = redeemscript;
        hash_ = ripemd160(sha256(redeemscript));
    }
    else {
        type_ = UNKNOWN;
    }
}

bytes_t Script::txinscript(sigtype_t sigtype) const
{
    uchar_vector script;

    if (sigtype != SIGN) {
        if (type_ == PAY_TO_MULTISIG_SCRIPT_HASH) {
            script.push_back(0x00);
        }

        for (auto& sig: sigs_) {
            if (!sig.empty() || sigtype == EDIT) {
                script += opPushData(sig.size());
                script += sig;
            }
        }
    }

    if (type_ == PAY_TO_PUBKEY_HASH) {
        if (sigtype != SIGN) {
            script += opPushData(pubkeys_[0].size());
            script += pubkeys_[0];
        }
        else {
            script += txoutscript();
        }
    }
    else if (type_ == PAY_TO_MULTISIG_SCRIPT_HASH) {
        if (sigtype != SIGN) {
            script += opPushData(redeemscript_.size());
        }
        script += redeemscript_;
    }

    return script;
}

bytes_t Script::txoutscript() const
{
    uchar_vector script;

    if (type_ == PAY_TO_PUBKEY_HASH) {
        script.push_back(0x76); // OP_PUSHDATA1
        script.push_back(0xa9); // OP_HASH160
        script.push_back(0x14); // push a 20 byte hash
        script += hash_;
        script.push_back(0x88); // OP_EQUALVERIFY
        script.push_back(0xac); // OP_CHECKSIG
    }
    else if (type_ == PAY_TO_MULTISIG_SCRIPT_HASH) {
        script.push_back(0xa9); // OP_HASH160
        script.push_back(0x14); // push a 20 byte hash
        script += hash_;
        script.push_back(0x87); // OP_EQUAL
    }
    else if (type_ == PAY_TO_PUBKEY) {
        script += opPushData(pubkeys_[0].size());
        script += pubkeys_[0];
        script.push_back(0xac); // OP_CHECKSIG
    }

    return script;
}

unsigned int Script::sigsneeded() const
{
    if (type_ == UNKNOWN) return 0;

    unsigned int sigsneeded = minsigs_;
    for (auto& sig: sigs_) {
        if (!sig.empty()) {
            sigsneeded--;
        }
        if (sigsneeded == 0) break;
    }

    return sigsneeded;
}

std::vector<bytes_t> Script::missingsigs() const
{
    std::vector<bytes_t> missingsigs;
    if (type_ == UNKNOWN) return missingsigs;

    unsigned int i = 0;
    for (auto& sig: sigs_)
    {
        if (sig.empty()) { missingsigs.push_back(pubkeys_[i]); }
        i++;
    }

    return missingsigs;
}

std::vector<bytes_t> Script::presentsigs() const
{
    std::vector<bytes_t> presentsigs;
    if (type_ == UNKNOWN) return presentsigs;

    unsigned int i = 0;
    for (auto& sig: sigs_)
    {
        if (!sig.empty()) { presentsigs.push_back(pubkeys_[i]); }
        i++;
    }

    return presentsigs;
}

bool Script::addSig(const bytes_t& pubkey, const bytes_t& sig)
{
    if (type_ == UNKNOWN) return false;

    unsigned int i = 0;
    unsigned int nsigs = 0;
    for (auto& sig_: sigs_) {
        if (!sig_.empty()) {
            nsigs++;
            if (nsigs > minsigs_) return false;
        }
        else if (pubkeys_[i] == pubkey) {
            sig_ = sig;
            return true;
        }
        i++;
    }

    return false;
}

void Script::clearSigs()
{
    sigs_.clear();
    for (unsigned int i = 0; i < pubkeys_.size(); i++) { sigs_.push_back(bytes_t()); }
}

unsigned int Script::mergesigs(const Script& other)
{
    if (type_ != other.type_) throw std::runtime_error("Script::mergesigs(...) - cannot merge two different script types.");
    if (type_ == UNKNOWN) return 0;

    if (minsigs_ != other.minsigs_) throw std::runtime_error("Script::mergesigs(...) - cannot merge two scripts with different minimum signatures.");
    if (pubkeys_ != other.pubkeys_) throw std::runtime_error("Script::mergesigs(...) - cannot merge two scripts with different public keys.");
    if (sigs_.size() != other.sigs_.size()) throw std::runtime_error("Script::mergesigs(...) - signature counts differ. invalid state.");

    unsigned int sigsadded = 0;
    for (std::size_t i = 0; i < sigs_.size(); i++)
    {
        if (sigs_[i].empty() && !other.sigs_[i].empty())
        {
            sigs_[i] = other.sigs_[i];
            sigsadded++;
        }
    }
    return sigsadded;
}


void SignableTxIn::setTxIn(const Coin::Transaction& tx, std::size_t nIn, uint64_t outpointamount)
{
    redeemscript_.clear();
    pubkeys_.clear();
    sigs_.clear();

    if (nIn >= tx.inputs.size())
        throw std::runtime_error("SignableTxIn::setTxIn() - nIn out of range.");

    const bytes_t& txinscript = tx.inputs[nIn].scriptSig;
    const std::vector<uchar_vector>& stack = tx.inputs[nIn].scriptWitness.stack;
    std::vector<bytes_t> objects;
    unsigned int pos = 0;
    while (pos < txinscript.size())
    {
        std::size_t size = getDataLength(txinscript, pos);
        if (pos + size > txinscript.size())
            throw std::runtime_error("SignableTxIn::setTxIn() - Push operation exceeds data size.");

        objects.push_back(bytes_t(txinscript.begin() + pos, txinscript.begin() + pos + size));
        pos += size;
    }

    WitnessProgram::version_t wpVersion = WitnessProgram::NO_WITNESS;
    std::vector<bytes_t> sigs;
    type_ = UNKNOWN;
    if (objects.size() == 1)
    {
        wpVersion = WitnessProgram::getWitnessVersion(txinscript);
        switch (wpVersion)
        {
        case WitnessProgram::WITNESS_V1:
            if (stack.empty())
            {
                type_ = MISSING_WITNESS;
                return;
            }
            redeemscript_ = stack.back();
            for (std::size_t i = 1; i < stack.size() - 1; i++) { sigs.push_back(stack[i]); }
            break;

        case WitnessProgram::WITNESS_V0:
        default:
            return;
        }
    }
    else if (objects.size() == 2)
    {
        type_ = PAY_TO_PUBKEY_HASH;
        pubkeys_.push_back(objects[1]);
        minsigs_ = 1;
        sigs.push_back(objects[0]);
    }
    else if (objects.size() >= 3)
    {
        redeemscript_ = objects.back();
        for (std::size_t i = 1; i < objects.size() - 1; i++) { sigs.push_back(objects[i]); }
    }

    if (redeemscript_.size() >= 3)
    {
        // Parse redeemscript
        // Get minsigs. Size opcode is offset by 0x50.
        unsigned char byte = redeemscript_[0];
        if (byte < OP_1 || byte > OP_16) return;
        minsigs_ = byte - OP_1_OFFSET;

        unsigned int pos = 1;
        while (true)
        {
            byte = redeemscript_[pos++];
            if (pos >= redeemscript_.size()) return;

            if (byte >= OP_1 && byte <= OP_16)
            {
                // Interpret byte as signature counter.
                if (pubkeys_.size() < minsigs_ || (byte - OP_1_OFFSET) != pubkeys_.size()) return;

                // Redeemscript must terminate with OP_CHECKMULTISIG
                if (redeemscript_[pos++] != 0xae || pos != redeemscript_.size()) return;

                break;
            }
            // Interpret byte as pubkey size
            if (byte > 0x4b || pos + byte > redeemscript_.size()) return;

            if (pubkeys_.size() >= 16) throw std::runtime_error("Public key maximum of 16 exceeded.");

            pubkeys_.push_back(bytes_t(redeemscript_.begin() + pos, redeemscript_.begin() + pos + byte));
            pos += byte;
        }

        type_ = (wpVersion == WitnessProgram::WITNESS_V1 ? PAY_TO_M_OF_N_WITNESS_V1 : PAY_TO_M_OF_N_SCRIPT_HASH);
    }

    if (sigs.size() > pubkeys_.size())
        throw std::runtime_error("Too many signatures.");

    // Validate signatures.
    unsigned int iSig = 0;
    unsigned int nValidSigs = 0;
    for (auto& pubkey: pubkeys_)
    {
        // If we already have enough valid signatures or there are no more signatures or signature is a placeholder
        if (nValidSigs == minsigs_ || iSig >= sigs.size() || sigs[iSig].empty())
        {
            // Add or keep placeholder.
            sigs_.push_back(bytes_t());
            iSig++;
        }
        else
        {
            // TODO: add support for other hash types.
            if (sigs[iSig].back() != SIGHASH_ALL) throw std::runtime_error("Unsupported hash type.");

            // Remove hash type byte.
            bytes_t signature(sigs[iSig].begin(), sigs[iSig].end() - 1);

            // Verify signature.
            bytes_t sighash = tx.getSigHash(SIGHASH_ALL, nIn, redeemscript_, outpointamount);
            secp256k1_key key;
            key.setPubKey(pubkey);
            if (secp256k1_verify(key, sighash, signature))
            {
                // Signature is valid. Keep it.
                sigs_.push_back(sigs[iSig]);
                iSig++;
                nValidSigs++;
            }
            else
            {
                // Signature is invalid. Add placeholder for this pubkey and test it for next pubkey
                sigs_.push_back(bytes_t());
            }
        }
    }
}

bytes_t SignableTxIn::txinscript() const
{
    uchar_vector rval;

    switch (type_)
    {
    case PAY_TO_M_OF_N_SCRIPT_HASH:
        {
            bool needsSigs = sigsneeded() > 0;
            rval << OP_0;
            for (auto& sig: sigs_) { if (!sig.empty() || needsSigs) rval << pushStackItem(sig); }
            rval << pushStackItem(redeemscript_);
        }
        break;

    case PAY_TO_M_OF_N_WITNESS_V1:
        {
            uchar_vector witnessscript;
            witnessscript << OP_1 << pushStackItem(sha256(redeemscript_));
            rval << pushStackItem(witnessscript);
            break;
        } 
    default:
        break;
    }

    return rval; 
}

bytes_t SignableTxIn::txoutscript() const
{
    uchar_vector rval;

    switch (type_)
    {
    case PAY_TO_M_OF_N_SCRIPT_HASH:
        rval << OP_HASH160 << pushStackItem(hash160(redeemscript_)) << OP_EQUAL;
        break;

    case PAY_TO_M_OF_N_WITNESS_V1:
        {
            uchar_vector witnessscript;
            witnessscript << OP_1 << pushStackItem(sha256(redeemscript_));
            rval << OP_HASH160 << pushStackItem(hash160(sha256(witnessscript))) << OP_EQUAL;
            break;
        }
    default:
        break;
    }

    return rval; 
}

Coin::ScriptWitness SignableTxIn::scriptwitness() const
{
    Coin::ScriptWitness scriptwitness;

    switch (type_)
    {
    case PAY_TO_M_OF_N_SCRIPT_HASH:
        break;

    case PAY_TO_M_OF_N_WITNESS_V1:
        {
            bool needsSigs = sigsneeded() > 0;
            scriptwitness.clear();
            scriptwitness.push(bytes_t());
            for (auto& sig: sigs_) { if (!sig.empty() || needsSigs) scriptwitness.push(sig); }
            scriptwitness.push(redeemscript_);
        }
        break;

    default:
        break;
    }

    return scriptwitness; 
}

unsigned int SignableTxIn::sigsneeded() const
{
    if (type_ == UNKNOWN) return 0;

    unsigned int sigsneeded = minsigs_;
    for (auto& sig: sigs_) {
        if (!sig.empty()) {
            sigsneeded--;
        }
        if (sigsneeded == 0) break;
    }

    return sigsneeded;
}

std::vector<bytes_t> SignableTxIn::missingsigs() const
{
    std::vector<bytes_t> missingsigs;
    if (type_ == UNKNOWN) return missingsigs;

    unsigned int i = 0;
    for (auto& sig: sigs_)
    {
        if (sig.empty()) { missingsigs.push_back(pubkeys_[i]); }
        i++;
    }

    return missingsigs;
}

std::vector<bytes_t> SignableTxIn::presentsigs() const
{
    std::vector<bytes_t> presentsigs;
    if (type_ == UNKNOWN) return presentsigs;

    unsigned int i = 0;
    for (auto& sig: sigs_)
    {
        if (!sig.empty()) { presentsigs.push_back(pubkeys_[i]); }
        i++;
    }

    return presentsigs;
}

bool SignableTxIn::addsig(const bytes_t& pubkey, const bytes_t& sig)
{
    if (type_ == UNKNOWN) return false;

    if (sigsneeded() == 0) return false;

    unsigned int i = 0;
    unsigned int nsigs = 0;
    for (auto& sig_: sigs_) {
        if (!sig_.empty()) {
            nsigs++;
            if (nsigs > minsigs_) return false;
        }
        else if (pubkeys_[i] == pubkey) {
            sig_ = sig;
            return true;
        }
        i++;
    }

    return false;
}

void SignableTxIn::clearsigs()
{
    sigs_.clear();
    for (unsigned int i = 0; i < pubkeys_.size(); i++) { sigs_.push_back(bytes_t()); }
}

unsigned int SignableTxIn::mergesigs(const SignableTxIn& other)
{
    if (type_ != other.type_) throw std::runtime_error("SignableTxIn::mergesigs(...) - cannot merge two different script types.");
    if (type_ == UNKNOWN) return 0;

    if (minsigs_ != other.minsigs_) throw std::runtime_error("SignableTxIn::mergesigs(...) - cannot merge two scripts with different minimum signatures.");
    if (pubkeys_ != other.pubkeys_) throw std::runtime_error("SignableTxIn::mergesigs(...) - cannot merge two scripts with different public keys.");
    if (sigs_.size() != other.sigs_.size()) throw std::runtime_error("SignableTxIn::mergesigs(...) - signature counts differ. invalid state.");

    unsigned int sigsadded = 0;
    for (std::size_t i = 0; i < sigs_.size(); i++)
    {
        if (sigs_[i].empty() && !other.sigs_[i].empty())
        {
            sigs_[i] = other.sigs_[i];
            sigsadded++;
        }
    }
    return sigsadded;
}


void Signer::setTx(const Coin::Transaction& tx, const std::vector<uint64_t>& outpointvalues)
{
    tx_ = tx;
    signabletxins_.clear();

    for (std::size_t i = 0; i < tx.inputs.size(); i++)
    {
        uint64_t outpointvalue = (outpointvalues.size() > i ? outpointvalues[i] : 0);
        signabletxins_.push_back(SignableTxIn(tx, i, outpointvalue));
    }
}

unsigned int Signer::sigsneeded(std::size_t nIn) const
{
    if (nIn >= signabletxins_.size())
        throw std::runtime_error("Signer::sigsneeded() - index out of range.");

    return signabletxins_[nIn].sigsneeded();
}

unsigned int Signer::sigsneeded() const
{
    unsigned int total = 0;
    for (auto& signabletxin: signabletxins_) { total += signabletxin.sigsneeded(); }
    return total;
}

std::vector<bytes_t> Signer::missingsigs(std::size_t nIn) const
{
    if (nIn >= signabletxins_.size())
        throw std::runtime_error("Signer::missingsigs() - index out of range.");

    return signabletxins_[nIn].missingsigs();
}

std::vector<bytes_t> Signer::missingsigs() const
{
    std::vector<bytes_t> missingsigs;
    for (auto& signabletxin: signabletxins_)
        for (auto& sig: signabletxin.missingsigs())
            missingsigs.push_back(sig);

    return missingsigs;
}

std::vector<bytes_t> Signer::presentsigs(std::size_t nIn) const
{
    if (nIn >= signabletxins_.size())
        throw std::runtime_error("Signer::presentsigs() - index out of range.");

    return signabletxins_[nIn].presentsigs();
}

std::vector<bytes_t> Signer::presentsigs() const
{
    std::vector<bytes_t> presentsigs;
    for (auto& signabletxin: signabletxins_)
        for (auto& sig: signabletxin.presentsigs())
            presentsigs.push_back(sig);

    return presentsigs;
}

bool Signer::addsig(std::size_t nIn, const bytes_t& pubkey, const bytes_t& sig)
{
    if (nIn >= signabletxins_.size())
        throw std::runtime_error("Signer::addsig() - index out of range.");

    return signabletxins_[nIn].addsig(pubkey, sig);
}

bool Signer::addsig(const bytes_t& pubkey, const bytes_t& sig)
{
    bool bSigned = false;
    for (auto& txin: signabletxins_) { bSigned = bSigned | txin.addsig(pubkey, sig); }
    return bSigned;
}

void Signer::clearsigs(std::size_t nIn)
{
    if (nIn >= signabletxins_.size())
        throw std::runtime_error("Signer::clearsigs() - index out of range.");

    signabletxins_[nIn].clearsigs();
}

void Signer::clearsigs()
{
    for (auto& signabletxin: signabletxins_) { signabletxin.clearsigs(); }
}

unsigned int mergesigs(std::size_t nIn, const Signer& other)
{
    return 0;
}

unsigned int mergesigs(const Signer& other)
{
    return 0;
}


}
}
