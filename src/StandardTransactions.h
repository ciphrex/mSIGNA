////////////////////////////////////////////////////////////////////////////////
//
// StandardTransactions.h
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

#ifndef STANDARD_TRANSACTIONS_H__
#define STANDARD_TRANSACTIONS_H__

#include "CoinNodeData.h"
#include "Base58Check.h"
#include "CoinKey.h"
#include "hash.h"

#include <map>
#include <set>
#include <sstream>

namespace Coin {

const unsigned char BITCOIN_ADDRESS_VERSIONS[] = {0x00, 0x05};

// TODO: Move opPushData and bytesPushData to a script manipulation module and use them
// wherever scripts are accessed.
extern uchar_vector opPushData(uint32_t nBytes);
extern uint32_t bytesPushData(const uchar_vector& script, uint& pos);


enum TxOutType { TXOUT_UNKNOWN, TXOUT_P2A, TXOUT_P2SH };

class StandardTxOut : public TxOut
{
private:
    TxOutType txOutType;
    uchar_vector pubKeyHash;

public:
    StandardTxOut() : txOutType(TXOUT_UNKNOWN) { }
    StandardTxOut(const TxOut& txOut);

    void set(const std::string& address, uint64_t value, const unsigned char addressVersions[] = BITCOIN_ADDRESS_VERSIONS);

    TxOutType getType() const { return txOutType; }
    const uchar_vector& getPubKeyHash() { return pubKeyHash; }
};




enum ScriptSigType { SCRIPT_SIG_BROADCAST, SCRIPT_SIG_EDIT, SCRIPT_SIG_SIGN };
enum SigHashType {
    SIGHASH_ALREADYADDED    = 0x00,
    SIGHASH_ALL             = 0x01,
    SIGHASH_NONE            = 0x02,
    SIGHASH_SINGLE          = 0x03,
    SIGHASH_ANYONECANPAY    = 0x80
};

class StandardTxIn : public TxIn
{
public:
    StandardTxIn() { }
    StandardTxIn(const TxIn& txIn) : TxIn(txIn) { }
    StandardTxIn(const uchar_vector& _outhash, uint32_t _outindex, uint32_t _sequence) :
        TxIn(OutPoint(_outhash, _outindex), "", _sequence) { }

    virtual void clearPubKeys() = 0;
    virtual void addPubKey(const uchar_vector& _pubKey) = 0;

    virtual void clearSigs() = 0;
    virtual void addSig(const uchar_vector& _pubKey, const uchar_vector& _sig, SigHashType sigHashType) = 0;
    virtual void getPubKeysMissingSig(std::vector<uchar_vector>& pubKeys, uint& minSigsStillNeeded) const = 0;

    virtual void setScriptSig(ScriptSigType scriptSigType) = 0;
};

class P2AddressTxIn : public StandardTxIn
{
private:
    uchar_vector pubKey;
    uchar_vector sig;

public:
    P2AddressTxIn() : StandardTxIn() { }
    P2AddressTxIn(const TxIn& txIn) : StandardTxIn(txIn) { }
    P2AddressTxIn(const uchar_vector& _outhash, uint32_t _outindex, const uchar_vector& _pubKey = uchar_vector(), uint32_t _sequence = 0xffffffff) :
        StandardTxIn(_outhash, _outindex, _sequence), pubKey(_pubKey) { }

    void clearPubKeys() { pubKey.clear(); }
    void addPubKey(const uchar_vector& _pubKey);

    void clearSigs() { this->sig = ""; }
    void addSig(const uchar_vector& _pubKey, const uchar_vector& _sig, SigHashType sigHashType = SIGHASH_ALL);
    void getPubKeysMissingSig(std::vector<uchar_vector>& pubKeys, uint& minSigsStillNeeded) const
    {
        pubKeys.clear();
        if (sig.size() > 0) {
            minSigsStillNeeded = 0;
        }
        else {
            minSigsStillNeeded = 1;
            pubKeys.push_back(pubKey);
        }
    }

    void setScriptSig(ScriptSigType scriptSigType);
};



class MultiSigRedeemScript
{
private:
    uint minSigs;
    std::vector<uchar_vector> pubKeys;

    const unsigned char* addressVersions;
    const char* base58chars;

    mutable uchar_vector redeemScript;
    mutable bool bUpdated;

public:
    MultiSigRedeemScript(const uchar_vector& redeemScript) { parseRedeemScript(redeemScript); }
    MultiSigRedeemScript(const std::string& redeemScript) { parseRedeemScript(uchar_vector(redeemScript)); }
    MultiSigRedeemScript(uint minSigs = 1,
                         const unsigned char* _addressVersions = BITCOIN_ADDRESS_VERSIONS,
                         const char* _base58chars = BITCOIN_BASE58_CHARS) :
        addressVersions(_addressVersions), base58chars(_base58chars), bUpdated(false) { this->setMinSigs(minSigs); }

    void setMinSigs(uint minSigs);
    uint getMinSigs() const { return minSigs; }

    void setAddressTypes(const unsigned char* addressVersions, const char* base58chars = BITCOIN_BASE58_CHARS)
    {
        this->addressVersions = addressVersions;
        this->base58chars = base58chars;
    }

    void clearPubKeys() { pubKeys.clear(); this->bUpdated = false; }
    void addPubKey(const uchar_vector& pubKey);
    uint getPubKeyCount() const { return pubKeys.size(); }
    std::vector<uchar_vector> getPubKeys() const { return pubKeys; }

    void parseRedeemScript(const uchar_vector& redeemScript);
    uchar_vector getRedeemScript() const;
    std::string getAddress() const;

    std::string toJson(bool bShowPubKeys = false) const;
};



class MofNTxIn : public StandardTxIn
{
private:
    uint minSigs;

    std::map<uchar_vector, uchar_vector> mapPubKeyToSig;
    std::vector<uchar_vector> pubKeys;

public:
    MofNTxIn() : StandardTxIn(), minSigs(0) { }
    MofNTxIn(const TxIn& txIn) : StandardTxIn(txIn), minSigs(0) { }
    MofNTxIn(const uchar_vector& outhash, uint32_t outindex, const MultiSigRedeemScript& redeemScript = MultiSigRedeemScript(), uint32_t sequence = 0xffffffff) :
        StandardTxIn(outhash, outindex, sequence) { setRedeemScript(redeemScript); }

    void setRedeemScript(const MultiSigRedeemScript& redeemScript);

    void clearPubKeys() { mapPubKeyToSig.clear(); pubKeys.clear(); }
    void addPubKey(const uchar_vector& pubKey);

    void clearSigs();
    void addSig(const uchar_vector& pubKey, const uchar_vector& sig, SigHashType sigHashType = SIGHASH_ALL);
    void getPubKeysMissingSig(std::vector<uchar_vector>& pubKeys, uint& minSigsStillNeeded) const;

    void setScriptSig(ScriptSigType scriptSigType);
};



class P2SHTxIn : public StandardTxIn
{
private:
    uchar_vector redeemScript;
    
    std::vector<uchar_vector> sigs;

public:
    P2SHTxIn() : StandardTxIn() { }
    P2SHTxIn(const uchar_vector& outhash, uint32_t outindex, const uchar_vector& _redeemScript = uchar_vector(), uint32_t sequence = 0xffffffff) :
        StandardTxIn(outhash, outindex, sequence), redeemScript(_redeemScript) { }

    void setRedeemScript(const uchar_vector& redeemScript) { this->redeemScript = redeemScript; }
    const uchar_vector& getRedeemScript() const { return this->redeemScript; }

    void clearPubKeys() { }
    void addPubKey(const uchar_vector& pubKey) { }

    void clearSigs() { sigs.clear(); }
    void addSig(const uchar_vector& pubKey, const uchar_vector& sig, SigHashType sigHashType = SIGHASH_ALL)
    {
        uchar_vector _sig = sig;
        if (sigHashType != SIGHASH_ALREADYADDED) {
            _sig.push_back(sigHashType);
        }
        sigs.push_back(_sig);
    }
    void getPubKeysMissingSig(std::vector<uchar_vector>& pubKeys, uint& minSigsStillNeeded) const { }

    void setScriptSig(ScriptSigType scriptSigType);
};



class InputSigRequest
{
public:
    uint32_t inputIndex;
    uint  minSigsStillNeeded;
    std::vector<uchar_vector> pubKeys;
    uint64_t value; // set to 0xffffffffffffffffull if not known

    InputSigRequest(const InputSigRequest& req) :
        inputIndex(req.inputIndex), minSigsStillNeeded(req.minSigsStillNeeded), pubKeys(req.pubKeys), value(req.value) { }

    InputSigRequest(uint32_t inputIndex, uint minSigsStillNeeded, const std::vector<uchar_vector>& pubKeys, uint64_t value = 0xffffffffffffffffull)
    {
        this->inputIndex = inputIndex;
        this->minSigsStillNeeded = minSigsStillNeeded;
        this->pubKeys = pubKeys;
        this->value = value;
    }
};

class TransactionBuilder
{
private:
    uint32_t version;
    std::vector<StandardTxIn*> inputs;
    std::vector<StandardTxOut*> outputs;
    uint32_t lockTime;

    std::map<uchar_vector, Transaction> mapDependencies;

    mutable std::vector<InputSigRequest> missingSigs;
    mutable bool bMissingSigsUpdated;

public:
    TransactionBuilder() : bMissingSigsUpdated(false) { }
    TransactionBuilder(const Transaction& tx) : bMissingSigsUpdated(false) { this->setTx(tx); }
    
    ~TransactionBuilder();

    void clearInputs();
    void clearOutputs();

    void setSerialized(const uchar_vector& hex);
    uchar_vector getSerialized() const;
    
    void setTx(const Transaction& tx);
    Transaction getTx(ScriptSigType scriptSigType, int index = -1) const;

    void addDependency(const Transaction& tx);
    bool removeDependency(const uchar_vector& txHash); // Remove specific dependency by hash. Returns true if anything changed.

    bool stripDependencies(); // Only remove unused dependencies. Returns true if anything changed.
    void clearDependencies(); // Remove all dependencies

    std::vector<uchar_vector> getDependencyHashes() const;
    uint64_t getDependencyOutputValue(const uchar_vector& outHash, uint32_t outIndex) const;
    
    void addInput(const uchar_vector& outHash, uint outIndex, const uchar_vector& pubKey, uint32_t sequence = 0xffffffff);
    void removeInput(uint32_t index);

    void addOutput(const std::string& address, uint64_t value, const unsigned char addressVersions[] = BITCOIN_ADDRESS_VERSIONS);
    void removeOutput(uint32_t index);

    std::vector<InputSigRequest>& getMissingSigs() const;
    std::string getMissingSigsJson() const;

    void sign(uint index, const uchar_vector& pubKey, const uchar_vector& privKey, SigHashType sigHashType = SIGHASH_ALL);
    void sign(uint index, const uchar_vector& pubKey, const std::string& privKey, SigHashType sigHashType = SIGHASH_ALL);
};


} // namespace Coin

#endif // STANDARD_TRANSACTIONS_H__
