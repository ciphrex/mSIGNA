////////////////////////////////////////////////////////////////////////////////
//
// CoinKey.h
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

#ifndef COINKEY_H__
#define COINKEY_H__

#include "BigInt.h"
#include "hash.h"

#include <stdutils/uchar_vector.h>

#include <stdexcept>

#define PRIVATE_KEY_DER_LENGTH    279//bytes
#define PRIVATE_KEY_LENGTH         32//bytes

#define PUBLIC_KEY_HASH_LENGTH     20//bytes

enum {
    PRIVATE_KEY_32 = 1,
    PRIVATE_KEY_DER_279
};

// Bitcoin version numbers
#define DEFAULT_ADDRESS_VERSION         0x00
#define DEFAULT_WALLET_IMPORT_VERSION   0x80

bool isValidCoinAddress(const std::string& address, unsigned int addressVersion = DEFAULT_ADDRESS_VERSION);

typedef struct ec_key_st EC_KEY;

class CoinKeyError : public std::runtime_error
{
public:
    explicit CoinKeyError(const std::string& error) : runtime_error(error) { }
};

class CoinKey
{
protected:
    EC_KEY* pKey;
    bool bSet;
    bool bCompressed;

    unsigned int addressVersion;
    unsigned int walletImportVersion;

public:
    CoinKey(unsigned int addressVersion = DEFAULT_ADDRESS_VERSION, unsigned int walletImportVersion = DEFAULT_WALLET_IMPORT_VERSION);
    CoinKey(const CoinKey& other);
    ~CoinKey();

    CoinKey& operator=(const CoinKey& other);

    void setVersionBytes(unsigned int addressVersion, unsigned int walletImportVersion) { this->addressVersion = addressVersion, this->walletImportVersion = walletImportVersion; }
    void setCompressed(bool bCompressed) { this->bCompressed = bCompressed; }
    bool isCompressed() const { return bCompressed; }

    bool isSet() const;
    void generateNewKey();

    // determines whether we're using a full 279-byte DER key or a shorter 32-byte one according to the length of privateKey
    bool setPrivateKey(const uchar_vector_secure& privateKey, bool bCompressed = true);
    uchar_vector_secure getPrivateKey(unsigned int privateKeyFormat = PRIVATE_KEY_32) const;

    bool setWalletImport(const string_secure& walletImport);
    string_secure getWalletImport() const;

    bool setPublicKey(const uchar_vector& publicKey);
    uchar_vector getPublicKey() const;

    std::string getAddress() const;

    bool sign(const uchar_vector& digest, uchar_vector& signature);
    // create a compact signature (65 bytes), which allows reconstructing the used public key
    bool signCompact(const uchar_vector& digest, uchar_vector& signature);
    // reconstruct public key from a compact signature
    bool setCompactSignature(const uchar_vector& digest, const uchar_vector& signature);

    // Return values: -1 = error, 0 = invalid signature, 1 = valid
    int verify(const uchar_vector& digest, const uchar_vector& signature);
    bool verifyCompact(const uchar_vector& digest, const uchar_vector& signature);
};

#endif
