///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_script.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include "CoinQ_script.h"

#include <CoinClasses/Base58Check.h>

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

uint32_t getDataLength(const uchar_vector& script, uint& pos)
{
    if (pos >= script.size()) {
        throw std::runtime_error("Script pos past end.");
    }

    unsigned char op = script[pos++];
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
        return "";
    }
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

Script::Script(const bytes_t& txinscript)
{
    std::vector<bytes_t> objects;
    unsigned int pos = 0;
    while (pos < txinscript.size()) {
        std::size_t size = getDataLength(txinscript, pos);
        if (pos + size > txinscript.size()) {
            throw std::runtime_error("Push operation exceeds data size.");
        }
        objects.push_back(bytes_t(txinscript.begin() + pos, txinscript.begin() + pos + size));
        pos += size;
    }

    if (objects.size() == 2) {
        type_ = PAY_TO_PUBKEY_HASH;
        pubkeys_.push_back(objects[1]);
        minsigs_ = 1;
        sigs_.push_back(objects[0]);
        hash_ = ripemd160(sha256(pubkeys_[0]));
    }
    else if (objects.size() >= 3 && objects[0].empty()) {
        // Parse redeemscript
        bytes_t redeemscript = objects.back();

        if (redeemscript.size() < 3) {
            throw std::runtime_error("Redeem script is too short.");
        }

        // size opcode is offset by 0x50
        unsigned char minsigs = redeemscript[0];
        if (minsigs < 0x51 || minsigs > 0x60) {
            throw std::runtime_error("Invalid signature minimum.");
        }

        unsigned char numkeys = 0x50;
        unsigned int pos = 1;
        while (true) {
            unsigned char byte = redeemscript[pos++];
            if (pos >= redeemscript.size()) {
                throw std::runtime_error("Script terminates prematurely.");
            }
            if (byte >= 0x51 && byte <= 0x60) {
                // Interpret byte as signature counter.
                if (byte != numkeys) {
                    throw std::runtime_error("Invalid signature count.");
                }
                if (numkeys < minsigs) {
                    throw std::runtime_error("The required signature minimum exceeds the number of keys.");
                }
                // Redeemscript must terminate with OP_CHECKMULTISIG
                if (redeemscript[pos++] != 0xae || pos > redeemscript.size()) {
                    throw std::runtime_error("Invalid script termination.");
                }
                break;
            }
            // Interpret byte as pubkey size
            if (byte > 0x4b || pos + byte > redeemscript.size()) {
                std::stringstream err;
                err << "Invalid OP at byte " << pos - 1 << ".";
                throw std::runtime_error(err.str());
            }
            numkeys++;
            if (numkeys > 0x60) {
                throw std::runtime_error("Public key maximum of 16 exceeded.");
            }
            pubkeys_.push_back(bytes_t(redeemscript.begin() + pos, redeemscript.begin() + pos + byte));
            pos += byte;
        }

        type_ = PAY_TO_MULTISIG_SCRIPT_HASH;
        minsigs_ = minsigs - 0x50;
        sigs_.assign(objects.begin() + 1, objects.end() - 1);
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

    unsigned int i = 0;
    unsigned int nsigs = 0;
    for (auto& sig: sigs_) {
        if (!sig.empty()) {
            nsigs++;
            if (nsigs > minsigs_) break;
        }
        else {
            missingsigs.push_back(pubkeys_[i]);
        }
        i++;
    }

    return missingsigs;
}

bool Script::addSig(const bytes_t& pubkey, const bytes_t& sig)
{
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

}
}
