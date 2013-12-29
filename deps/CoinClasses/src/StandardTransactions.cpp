////////////////////////////////////////////////////////////////////////////////
//
// StandardTransactions.cpp
//
// Copyright (c) 2011-2012 Eric Lombrozo
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

// Note: bUpdated = false for mutable fields iff internal state is inconsistent.
//       bUpdated = true if internal state has been updated.

#include "StandardTransactions.h"
#include "CoinNodeData.h"
#include "Base58Check.h"
#include "CoinKey.h"
#include "hash.h"

#include <map>
#include <set>
#include <sstream>

using namespace Coin;

// TODO: Move opPushData and bytesPushData to a script manipulation module and use them
// wherever scripts are accessed.
uchar_vector Coin::opPushData(uint32_t nBytes)
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
        rval.push_back((unsigned char)(nBytes >> 8));
        rval.push_back((unsigned char)(nBytes & 0xff));
    }
    else {
        rval.push_back(0x4e);
        rval.push_back((unsigned char)(nBytes >> 24));
        rval.push_back((unsigned char)((nBytes >> 16) & 0xff));
        rval.push_back((unsigned char)((nBytes >> 8) & 0xff));
        rval.push_back((unsigned char)(nBytes & 0xff));
    }

    return rval;
}

uint32_t Coin::bytesPushData(const uchar_vector& script, uint& pos)
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
        uint32_t rval = (uint32_t)script[pos++] << 8;
        rval         += (uint32_t)script[pos++];
        return rval;
    }
    else if (op == 0x4e) {
        if (pos + 3 >= script.size()) {
            throw std::runtime_error("Script pos past end.");
        }
        uint32_t rval = (uint32_t)script[pos++] << 24;
        rval         += (uint32_t)script[pos++] << 16;
        rval         += (uint32_t)script[pos++] << 8;
        rval         += (uint32_t)script[pos++];
        return rval;
    }
    else {
        throw std::runtime_error("Operation is not push data.");
    }
}




StandardTxOut::StandardTxOut(const TxOut& txOut) : TxOut(txOut)
{
    if (scriptPubKey.size() == 25 &&
            scriptPubKey[0] == 0x76 &&
            scriptPubKey[1] == 0xa9 &&
            scriptPubKey[2] == 0x14 &&
            scriptPubKey[23] == 0x88 &&
            scriptPubKey[24] == 0xac) {
        txOutType = TXOUT_P2A;
        pubKeyHash.assign(scriptPubKey.begin() + 3, scriptPubKey.begin() + 23);
        return;
    }

    if (scriptPubKey.size() == 23 &&
            scriptPubKey[0] == 0xa9 &&
            scriptPubKey[1] == 0x14 &&
            scriptPubKey[22] == 0x87) {
        txOutType = TXOUT_P2SH;
        pubKeyHash.assign(scriptPubKey.begin() + 2, scriptPubKey.begin() + 22);
        return;
    }

    txOutType = TXOUT_UNKNOWN;
}

void StandardTxOut::set(const std::string& address, uint64_t value, const unsigned char addressVersions[])
{
    uchar_vector pubKeyHash;
    uint version;
    if (!fromBase58Check(address, pubKeyHash, version)) {
        std::stringstream ss;
        ss << "Invalid address checksum for address " << address << ".";
        throw std::runtime_error(ss.str());
    }

    if (version == addressVersions[0]) {
        // pay-to-address
        txOutType = TXOUT_P2A;
        scriptPubKey = uchar_vector("76a914") + pubKeyHash + uchar_vector("88ac");
    }
    else if (version == addressVersions[1]) {
        // pay-to-script-hash
        txOutType = TXOUT_P2SH;
        scriptPubKey = uchar_vector("a914") + pubKeyHash + uchar_vector("87");
    }
    else {
        throw std::runtime_error("Invalid address version.");
    }

    if (pubKeyHash.size() != 20) {
        throw std::runtime_error("Invalid hash length.");
    }

    this->value = value;
    this->pubKeyHash = pubKeyHash;
}



void P2AddressTxIn::addPubKey(const uchar_vector& _pubKey)
{
    if (pubKey.size() > 0) {
        throw std::runtime_error("PubKey already added.");
    }
    pubKey = _pubKey;
}

void P2AddressTxIn::addSig(const uchar_vector& _pubKey, const uchar_vector& _sig, SigHashType sigHashType)
{
    if (pubKey.size() == 0) {
        throw std::runtime_error("No PubKey added yet.");
    }

    if (_pubKey != pubKey) {
        throw std::runtime_error("PubKey not part of input.");
    }

    sig = _sig;
    if (sigHashType != SIGHASH_ALREADYADDED) {
        sig.push_back(sigHashType);
    }
}

void P2AddressTxIn::setScriptSig(ScriptSigType scriptSigType)
{
    scriptSig.clear();

    if (scriptSigType == SCRIPT_SIG_SIGN) {
        scriptSig.push_back(0x76);
        scriptSig.push_back(0xa9);
        scriptSig.push_back(0x14);
        scriptSig += ripemd160(sha256(pubKey));
        scriptSig.push_back(0x88);
        scriptSig.push_back(0xac);
    }
    else {
        scriptSig += opPushData(sig.size()); // includes SigHashType
        scriptSig += sig;

        scriptSig += opPushData(pubKey.size());
        scriptSig += pubKey;
    }
}



void MultiSigRedeemScript::setMinSigs(uint minSigs)
{
    if (minSigs < 1) {
        throw std::runtime_error("At least one signature is required.");
    }

    if (minSigs > 16) {
        throw std::runtime_error("At most 16 signatures are allowed.");
    }

    this->minSigs = minSigs;
    this->bUpdated = false;
}

void MultiSigRedeemScript::addPubKey(const uchar_vector& pubKey)
{
    if (pubKeys.size() >= 16) {
        throw std::runtime_error("Public key maximum of 16 already reached.");
    }

    if (pubKey.size() > 75) {
        throw std::runtime_error("Public keys can be a maximum of 75 bytes.");
    }

    pubKeys.push_back(pubKey);
    bUpdated = false;
}

void MultiSigRedeemScript::parseRedeemScript(const uchar_vector& redeemScript)
{
    if (redeemScript.size() < 3) {
        throw std::runtime_error("Redeem script is too short.");
    }

    // OP_1 is 0x51, OP_16 is 0x60
    unsigned char mSigs = redeemScript[0];
    if (mSigs < 0x51 || mSigs > 0x60) {
        throw std::runtime_error("Invalid signature minimum.");
    }

    unsigned char nKeys = 0x50;
    uint i = 1;
    std::vector<uchar_vector> _pubKeys;
    while (true) {
        unsigned char byte = redeemScript[i++];
        if (i >= redeemScript.size()) {
            throw std::runtime_error("Script terminates prematurely.");
        }
        if ((byte >= 0x51) && (byte <= 0x60)) {
            // interpret byte as the signature counter.
            if (byte != nKeys) {
                throw std::runtime_error("Invalid signature count.");
            }
            if (nKeys < mSigs) {
                throw std::runtime_error("The required signature minimum exceeds the number of keys.");
            }
            if (redeemScript[i++] != 0xae || i > redeemScript.size()) {
                throw std::runtime_error("Invalid script termination.");
            }
            break;
        }
        // interpret byte as the pub key size
        if ((byte > 0x4b) || (i + byte > redeemScript.size())) {
            std::stringstream ss;
            ss << "Invalid OP at byte " << i - 1 << ".";
            throw std::runtime_error(ss.str());
        }
        nKeys++;
        if (nKeys > 0x60) {
            throw std::runtime_error("Public key maximum of 16 exceeded.");
        }
        _pubKeys.push_back(uchar_vector(redeemScript.begin() + i, redeemScript.begin() + i + byte));
        i += byte;
    }

    minSigs = mSigs - 0x50;
    pubKeys = _pubKeys;
}

uchar_vector MultiSigRedeemScript::getRedeemScript() const
{
    if (!bUpdated) {
        uint nKeys = pubKeys.size();

        if (minSigs > nKeys) {
            throw std::runtime_error("Insufficient public keys.");
        }

        redeemScript.clear();
        redeemScript.push_back((unsigned char)(minSigs + 0x50));
        for (uint i = 0; i < nKeys; i++) {
            redeemScript.push_back(pubKeys[i].size());
            redeemScript += pubKeys[i];
        }
        redeemScript.push_back((unsigned char)(nKeys + 0x50));
        redeemScript.push_back(0xae); // OP_CHECKMULTISIG
        bUpdated = true;
    }

    return redeemScript;
}

std::string MultiSigRedeemScript::getAddress() const
{
    uchar_vector scriptHash = ripemd160(sha256(getRedeemScript()));
    return toBase58Check(scriptHash, addressVersions[1], base58chars);
}

std::string MultiSigRedeemScript::toJson(bool bShowPubKeys) const
{
    uint nKeys = pubKeys.size();
    std::stringstream ss;
    ss <<   "{\n    \"m\" : " << minSigs
       <<   ",\n    \"n\" : " << nKeys
       <<   ",\n    \"address\" : \"" << getAddress()
       << "\",\n    \"redeemScript\" : \"" << getRedeemScript().getHex() << "\"";
    if (bShowPubKeys) {
        ss << ",\n    \"pubKeys\" :\n    [";
        for (uint i = 0; i < nKeys; i++) {
            uchar_vector pubKeyHash = ripemd160(sha256(pubKeys[i]));
            std::string address = toBase58Check(pubKeyHash, addressVersions[0], base58chars);
            if (i > 0) ss << ",";
            ss <<    "\n        {"
               <<    "\n            \"address\" : \"" << address
               << "\",\n            \"pubKey\" : \"" << pubKeys[i].getHex()
               <<  "\"\n        }";
        }
        ss << "\n    ]";
    }
    ss << "\n}";
    return ss.str();
}



void MofNTxIn::setRedeemScript(const MultiSigRedeemScript& redeemScript)
{
    minSigs = redeemScript.getMinSigs();
    pubKeys = redeemScript.getPubKeys();

    mapPubKeyToSig.clear();
    for (uint i = 0; i < pubKeys.size(); i++) {
        mapPubKeyToSig[pubKeys[i]] = uchar_vector();
    }
}

void MofNTxIn::addPubKey(const uchar_vector& pubKey)
{
    std::map<uchar_vector, uchar_vector>::iterator it;
    it = mapPubKeyToSig.find(pubKey);
    if (it != mapPubKeyToSig.end()) {
        throw std::runtime_error("PubKey already added.");
    }

    mapPubKeyToSig[pubKey] = uchar_vector();
}

// TODO: verify it->second can be set
void MofNTxIn::clearSigs()
{
    std::map<uchar_vector, uchar_vector>::iterator it = mapPubKeyToSig.begin();
    for(; it != mapPubKeyToSig.end(); ++it) {
        it->second = uchar_vector();
    }
}

void MofNTxIn::addSig(const uchar_vector& pubKey, const uchar_vector& sig, SigHashType sigHashType)
{
    std::map<uchar_vector, uchar_vector>::iterator it;
    it = mapPubKeyToSig.find(pubKey);
    if (it == mapPubKeyToSig.end()) {
        std::stringstream ss;
        ss << "PubKey " << pubKey.getHex() << " not yet added.";
        throw std::runtime_error(ss.str());
    }

    mapPubKeyToSig[pubKey] = sig;
    if (sigHashType != SIGHASH_ALREADYADDED) {
        mapPubKeyToSig[pubKey].push_back(sigHashType);
    }
}

void MofNTxIn::getPubKeysMissingSig(std::vector<uchar_vector>& pubKeys, uint& minSigsStillNeeded) const
{
    pubKeys.clear();

    uint nSigs = 0;
    for (uint i = 0; i < this->pubKeys.size(); i++) {
        // at() might throw an out_of_range exception if pubKey is not found,
        // but this condition should never occur.
        if (mapPubKeyToSig.at(this->pubKeys[i]).size() > 0) {
            nSigs++;
        }
        else {
            pubKeys.push_back(this->pubKeys[i]);
        }
    }

    minSigsStillNeeded = (nSigs >= minSigs) ? 0 : (minSigs - nSigs);
}

void MofNTxIn::setScriptSig(ScriptSigType scriptSigType)
{
    scriptSig.clear();

    if (scriptSigType == SCRIPT_SIG_BROADCAST || scriptSigType == SCRIPT_SIG_EDIT) {
        scriptSig.push_back(0x00); // OP_FALSE

        for (uint i = 0; i < pubKeys.size(); i++) {
            uchar_vector sig = mapPubKeyToSig[pubKeys[i]];
            if (sig.size() > 0 || scriptSigType == SCRIPT_SIG_EDIT) {
                scriptSig += opPushData(sig.size());
                scriptSig += sig;
            }
        }
    }

    MultiSigRedeemScript redeemScript(minSigs);
    for (uint i = 0; i < pubKeys.size(); i++) {
        redeemScript.addPubKey(pubKeys[i]);
    }

    if (scriptSigType == SCRIPT_SIG_BROADCAST || scriptSigType == SCRIPT_SIG_EDIT) {    
        scriptSig += opPushData(redeemScript.getRedeemScript().size());
    }
    scriptSig += redeemScript.getRedeemScript();    
}



void P2SHTxIn::setScriptSig(ScriptSigType scriptSigType)
{
    scriptSig.clear();

    if (scriptSigType == SCRIPT_SIG_BROADCAST || scriptSigType == SCRIPT_SIG_EDIT) {
        scriptSig.push_back(0x00); // OP_FALSE

        for (uint i = 0; i < sigs.size(); i++) {
            if (sigs[i].size() > 0 || scriptSigType == SCRIPT_SIG_EDIT) {
                scriptSig += opPushData(sigs[i].size());
                scriptSig += sigs[i];
            }
        }

        scriptSig += opPushData(redeemScript.size());
    }

    scriptSig += redeemScript;
}



void TransactionBuilder::setSerialized(const uchar_vector& bytes)
{
    Transaction tx(bytes);
    setTx(tx);

    uchar_vector remaining(bytes.begin() + tx.getSize(), bytes.end());
    while (remaining.size() > 0) {
        tx.setSerialized(remaining);
        mapDependencies[tx.getHashLittleEndian()] = tx;
        remaining.assign(remaining.begin() + tx.getSize(), remaining.end());
    }
}

uchar_vector TransactionBuilder::getSerialized() const
{
    uchar_vector rval = getTx(SCRIPT_SIG_EDIT).getSerialized().getHex();

    std::map<uchar_vector, Transaction>::const_iterator it = mapDependencies.begin();
    for (; it != mapDependencies.end(); ++it) {
        rval += it->second.getSerialized();
    }
    return rval;
}

void TransactionBuilder::setTx(const Transaction& tx)
{
    version = tx.version;
    lockTime = tx.lockTime;
    clearInputs();
    clearOutputs();

    for (uint i = 0; i < tx.inputs.size(); i++) {
        std::vector<uchar_vector> objects;
        uint s = 0;
        while (s < tx.inputs[i].scriptSig.size()) {
            uint size = bytesPushData(tx.inputs[i].scriptSig, s);
            if (s + size > tx.inputs[i].scriptSig.size()) {
                std::stringstream ss;
                ss << "Tried to push object that exceeeds scriptSig size in input " << i << ".";
                throw std::runtime_error(ss.str());
            }
            objects.push_back(uchar_vector(tx.inputs[i].scriptSig.begin() + s, tx.inputs[i].scriptSig.begin() + s + size));
            s += size;
        }

        if (objects.size() == 2) {
            // P2Address
            P2AddressTxIn* pTxIn = new P2AddressTxIn(tx.inputs[i]);
            pTxIn->addPubKey(objects[1]);
            if (objects[0].size() > 0) {
                pTxIn->addSig(objects[1], objects[0], SIGHASH_ALREADYADDED);
            }            
            inputs.push_back(pTxIn); 
        }
        else if (objects.size() >= 3 && objects[0].size() == 0) {
            // MofN
            MultiSigRedeemScript multiSig(objects.back());
            if (multiSig.getPubKeyCount() < objects.size() - 2) {
                // Only supports SIG_SCRIPT_EDIT input
                std::stringstream ss;
                ss << "Insufficient signatures in input " << i << ".";
                throw std::runtime_error(ss.str());
            }
            MofNTxIn* pTxIn = new MofNTxIn(tx.inputs[i]);
            pTxIn->setRedeemScript(multiSig);
            std::vector<uchar_vector> pubKeys = multiSig.getPubKeys();
            for (uint k = 1; k < objects.size() - 1; k++) {
                if (objects[k].size() > 0) {
                    pTxIn->addSig(pubKeys[k-1], objects[k], SIGHASH_ALREADYADDED);      
                }
            }
            inputs.push_back(pTxIn); 
        }
        else {
            std::stringstream ss;
            ss << "Nonstandard script in input " << i << ".";
            throw std::runtime_error(ss.str());
        }
    }

    for (uint i = 0; i < tx.outputs.size(); i++) {
        StandardTxOut* pTxOut = new StandardTxOut(tx.outputs[i]);
        outputs.push_back(pTxOut);
    }

    bMissingSigsUpdated = false;
}

// TODO: make this more efficient - avoid unnecessary reallocations and copies
Transaction TransactionBuilder::getTx(ScriptSigType scriptSigType, int index) const
{
    Transaction tx;
    tx.version = version;
    tx.lockTime = lockTime;

    for (uint i = 0; i < inputs.size(); i++) {
        if (index == -1 || index == (int)i) {
            inputs[i]->setScriptSig(scriptSigType);
        }
        else {
            inputs[i]->scriptSig.clear();
        }
        tx.addInput(*inputs[i]);
    }

    for (uint i = 0; i < outputs.size(); i++) {
        tx.addOutput(*outputs[i]);
    }

    return tx;
}

void TransactionBuilder::addDependency(const Transaction& tx)
{
    mapDependencies[tx.getHashLittleEndian()] = tx;
    bMissingSigsUpdated = false;
}

bool TransactionBuilder::removeDependency(const uchar_vector& txHash)
{
    return (mapDependencies.erase(txHash) > 0);
}

bool TransactionBuilder::stripDependencies()
{
    std::set<uchar_vector> outHashes;
    for (uint i = 0; i < inputs.size(); i++) {
        outHashes.insert(inputs[i]->getOutpointHash());
    }

    std::vector<uchar_vector> removeHashes;
    for (auto it = mapDependencies.begin(); it != mapDependencies.end(); ++it) {
        if (outHashes.find(it->first) == outHashes.end())   {
            removeHashes.push_back(it->first);
        }
    }

    for (auto it = removeHashes.begin(); it != removeHashes.end(); ++it) {
        mapDependencies.erase(*it);
    }

    return (removeHashes.size() > 0);
}

void TransactionBuilder::clearDependencies()
{
    mapDependencies.clear();
    bMissingSigsUpdated = false;
}

std::vector<uchar_vector> TransactionBuilder::getDependencyHashes() const
{
    std::vector<uchar_vector> hashes;

    std::map<uchar_vector, Transaction>::const_iterator it = mapDependencies.begin();
    for (; it != mapDependencies.end(); ++it) {
        hashes.push_back(it->first);
    }
    return hashes;
}

uint64_t TransactionBuilder::getDependencyOutputValue(const uchar_vector& outHash, uint32_t outIndex) const
{
    std::map<uchar_vector, Transaction>::const_iterator it = mapDependencies.find(outHash);
    if (it == mapDependencies.end() || outIndex >= it->second.outputs.size()) {
        throw std::runtime_error("Dependency output not found.");
    }

    return it->second.outputs[outIndex].value;
}

void TransactionBuilder::addInput(const uchar_vector& outHash, uint32_t outIndex, const uchar_vector& pubKey, uint32_t sequence)
{
    std::map<uchar_vector, Transaction>::const_iterator it = mapDependencies.find(outHash);
    if (it == mapDependencies.end()) {
        std::stringstream ss;
        ss << "Transaction with hash " << outHash.getHex() << " is not a dependency.";
        throw std::runtime_error(ss.str());
    }

    const Transaction& tx = it->second;
    if (outIndex > tx.outputs.size() - 1) {
        throw std::runtime_error("Invalid output index.");
    }

    StandardTxOut txOut(tx.outputs[outIndex]);
    if (txOut.getType() == TXOUT_UNKNOWN) {
        throw std::runtime_error("Unknown output type.");
    }

    if (txOut.getPubKeyHash() != ripemd160(sha256(pubKey))) {
        std::stringstream ss;
        ss << "Public key " << pubKey.getHex() << " does not hash to correct value.";
        throw std::runtime_error(ss.str());
    }

    if (txOut.getType() == TXOUT_P2A) {
        P2AddressTxIn* pTxIn = new P2AddressTxIn(outHash, outIndex, pubKey, sequence);
        inputs.push_back(pTxIn);
    }
    else if (txOut.getType() == TXOUT_P2SH) {
        // We can only sign M of N right now
        MofNTxIn* pTxIn = new MofNTxIn(outHash, outIndex, pubKey, sequence);
        inputs.push_back(pTxIn);    
    }

    bMissingSigsUpdated = false;
}

void TransactionBuilder::removeInput(uint32_t index)
{
    if (index > inputs.size() - 1) {
        std::stringstream ss;
        ss << "Invalid index " << index << ".";
        throw std::runtime_error(ss.str());
    }

    delete inputs[index];
    inputs.erase(inputs.begin() + index);

    bMissingSigsUpdated = false;
}

void TransactionBuilder::addOutput(const std::string& address, uint64_t value, const unsigned char addressVersions[])
{
    StandardTxOut* pTxOut = new StandardTxOut;
    try {
        pTxOut->set(address, value, addressVersions);
    }
    catch (const std::exception& e) {
        delete pTxOut;
        throw e;
    }
    outputs.push_back(pTxOut);
}

void TransactionBuilder::removeOutput(uint32_t index)
{
    if (index > outputs.size() - 1) {
        std::stringstream ss;
        ss << "Invalid index " << index << ".";
        throw std::runtime_error(ss.str());
    }

    delete outputs[index];
    outputs.erase(outputs.begin() + index);
}

std::vector<InputSigRequest>& TransactionBuilder::getMissingSigs() const
{
    if (bMissingSigsUpdated) {
        return missingSigs;
    }

    missingSigs.clear();
    for (uint i = 0; i < inputs.size(); i++) {
        std::vector<uchar_vector> pubKeys;
        uint32_t minSigsStillNeeded;
        inputs[i]->getPubKeysMissingSig(pubKeys, minSigsStillNeeded);

        uint64_t value;
        try {
            value = getDependencyOutputValue(inputs[i]->getOutpointHash(), inputs[i]->getOutpointIndex());
        }
        catch (...) {
            value = 0xffffffffffffffffull;
        }
        missingSigs.push_back(InputSigRequest(i, minSigsStillNeeded, pubKeys, value));
    }
    bMissingSigsUpdated = true;
    return missingSigs;
}

std::string TransactionBuilder::getMissingSigsJson() const
{
    getMissingSigs();
    std::stringstream ss;
    ss << "[\n";
    for (uint i = 0; i < missingSigs.size(); i++) {
        if (i > 0) ss << ",\n";
        ss << "    {"
           <<  "\n        \"index\" : " << missingSigs[i].inputIndex
           << ",\n        \"minSigsStillNeeded\" : " << missingSigs[i].minSigsStillNeeded
           << ",\n        \"pubKeys\" : [";
        for (uint j = 0; j < missingSigs[i].pubKeys.size(); j++) {
            if (j > 0) ss << ", ";
            ss << "\"" << missingSigs[i].pubKeys[j].getHex() << "\"";
        }
        ss << "]";

        if (missingSigs[i].value != 0xffffffffffffffffull) {
            ss << ",\n        \"value\" : \"" << missingSigs[i].value << "\"";
        }
        ss << "\n    }";
    }
    ss << "\n]";
    return ss.str();
}

void TransactionBuilder::sign(uint index, const uchar_vector& pubKey, const uchar_vector& privKey, SigHashType sigHashType)
{
    if (index > inputs.size() - 1) {
        throw std::runtime_error("Invalid input index.");
    }

    Transaction tx = getTx(SCRIPT_SIG_SIGN, index);
    uchar_vector hashToSign = tx.getHashWithAppendedCode(sigHashType);
    std::cout << "~~hashToSign: " << hashToSign.getHex() << std::endl;

    CoinKey key;
    if (!key.setPublicKey(pubKey)) {
        throw std::runtime_error("Invalid public key.");
    }

    if (!key.setPrivateKey(privKey)) {
        throw std::runtime_error("Invalid private key.");
    }

    if (key.getPublicKey() != pubKey) {
        throw std::runtime_error("Private key does not correspond to public key.");
    }

    uchar_vector sig;
    if (!key.sign(hashToSign, sig)) {
        throw std::runtime_error("Signing failed.");
    }

    inputs[index]->addSig(pubKey, sig, sigHashType);
}

void TransactionBuilder::sign(uint index, const uchar_vector& pubKey, const std::string& privKey, SigHashType sigHashType)
{
    if (index > inputs.size() - 1) {
        throw std::runtime_error("Invalid input index.");
    }

    Transaction tx = getTx(SCRIPT_SIG_SIGN, index);
    uchar_vector hashToSign = tx.getHashWithAppendedCode(sigHashType);
    std::cout << "~~hashToSign: " << hashToSign.getHex() << std::endl;

    CoinKey key;
    if (!key.setWalletImport(privKey)) {
        throw std::runtime_error("Invalid private key.");
    }

    if (key.getPublicKey() != pubKey) {
        throw std::runtime_error("Private key does not correspond to public key.");
    }

    uchar_vector sig;
    if (!key.sign(hashToSign, sig)) {
        throw std::runtime_error("Signing failed.");
    }

    inputs[index]->addSig(pubKey, sig, sigHashType);
}

void TransactionBuilder::clearInputs()
{
    if (inputs.size() > 0) {
        for (uint i = 0; i < inputs.size(); i++) {
            delete inputs[i];
        }
        inputs.clear();
    }
}

void TransactionBuilder::clearOutputs()
{
    if (outputs.size() > 0) {
        for (uint i = 0; i < outputs.size(); i++) {
            delete outputs[i];
        }
        outputs.clear();
    }
}

TransactionBuilder::~TransactionBuilder()
{
    clearInputs();
    clearOutputs();
}

