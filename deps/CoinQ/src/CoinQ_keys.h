////////////////////////////////////////////////////////////////////////////////
//
// CoinQ_keys.h
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#ifndef _COINQ_KEYS_H_
#define _COINQ_KEYS_H_

#include <CoinCore/CoinNodeData.h>

#include <set>
#include <string>
#include <vector>
#include <stdexcept>

namespace CoinQ {
namespace Keys{

class AddressSet
{
private:
    std::set<std::string> addresses;

public:
    const std::set<std::string>& getAddresses() const { return addresses; }

    void clear() { addresses.clear(); }
    void insert(const std::string& address);
    void insert(const std::vector<uchar_vector>& hashes, unsigned char version, const char* base58chars); 

    void loadFromFile(const std::string& filename);
    void flushToFile(const std::string& filename);
};

class KeychainException : public std::runtime_error
{
private:
    int m_code;

public:
    KeychainException(const std::string& message, int code) : std::runtime_error(message), m_code(code) { }
    int code() const { return m_code; }

    enum {
        UNKNOWN_ERROR = 0,
        KEY_NOT_FOUND,
        UNSUPPORTED_TYPE
    };
};

class IKeychain
{
public:
    enum Type { RECEIVING = 1, CHANGE = 2 };
    enum Status { POOL = 1, ALLOCATED = 2 };

    virtual void generateNewKeys(int count, int type, bool bCompressed) = 0;
    virtual int getPoolSize(int type) const = 0;

    virtual void insertKey(const uchar_vector& privKey, int type, int status, const std::string& label, bool bCompressed, int minHeight, int maxHeight) = 0;
    virtual void insertMultiSigRedeemScript(const std::vector<uchar_vector>& pubKeys, int type, int status, const std::string& label, int minHeight, int maxHeight) = 0;

    virtual std::vector<uchar_vector> getKeyHashes(int type, int status) const = 0;
    virtual std::vector<uchar_vector> getRedeemScriptHashes(int type, int status) const = 0;

    virtual uchar_vector getPublicKeyFromHash(const uchar_vector& keyHash) const = 0;
    virtual uchar_vector getPrivateKeyFromHash(const uchar_vector& keyHash) const = 0;
    virtual uchar_vector getPrivateKeyFromPublicKey(const uchar_vector& pubKey) const = 0;
    virtual uchar_vector getNextPoolPublicKey(int type) const = 0;
    virtual uchar_vector getNextPoolPublicKeyHash(int type) const = 0;
    virtual void removeKeyFromPool(const uchar_vector& keyHash) const = 0;
   
    virtual uchar_vector getRedeemScript(const uchar_vector& scriptHash) const = 0;
    virtual uchar_vector getNextPoolRedeemScript(int type) const = 0;
    virtual uchar_vector getNextPoolRedeemScriptHash(int type) const = 0;
    virtual void removeRedeemScriptFromPool(const uchar_vector& scriptHash) = 0;

    virtual uchar_vector getUnsignedScriptSig(const uchar_vector& scriptPubKey) const = 0;

/*
    // a collection of <scriptSig, scriptPubKey> pairs
    typedef std::vector<std::pair<uchar_vector, uchar_vector>> script_pairs_t;

    // getUnsignedScriptSigs
    virtual int getUnsignedScriptSigs(script_pairs_t& scriptPairs) const = 0;

    // tx_dependency_map_t holds a map from tx hashes to txs
    typedef std::map<uchar_vector, Coin::Transaction> tx_dependency_map_t;

    // Tries to sign the input with index inIndex in transaction tx. Checks dependencies for output values, should be rejected if dependency check fails.
    // Returns the number of signatures added, throws exception if dependency check fails.
    // Pass inIndex = -1 to sign all inputs we can.
    virtual int signTx(Coin::Transaction& tx, int inIndex, const tx_dependency_map_t& dependencies, unsigned char sigHashType = SIGHASH_ALL) const = 0;

    // Remove any keys from the unused pool if they appear anywhere in the transaction.
    virtual void removeUsedKeysFromPool(const Coin::Transaction& tx) = 0;
*/
    virtual uchar_vector getKeysOnlyHash() const = 0;
    virtual uchar_vector getFullKeychainHash() const = 0;
};

}
}

#endif // _COINQ_KEYS_H_
