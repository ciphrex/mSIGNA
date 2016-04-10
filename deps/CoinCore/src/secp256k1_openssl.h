////////////////////////////////////////////////////////////////////////////////
//
// secp256k1_openssl.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
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
//
// Some portions taken from bitcoin/bitcoin,
//      Copyright (c) 2009-2013 Satoshi Nakamoto, the Bitcoin developers

#pragma once

#include <stdexcept>

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>

#include "typedefs.h"

namespace CoinCrypto
{

class secp256k1_key
{
public:
    secp256k1_key();
    ~secp256k1_key() { EC_KEY_free(pKey); }

    EC_KEY* getKey() const { return bSet ? pKey : nullptr; }
    EC_KEY* newKey();
    bytes_t getPrivKey() const;
    EC_KEY* setPrivKey(const bytes_t& privkey);
    bytes_t getPubKey(bool bCompressed = true) const;
    EC_KEY* setPubKey(const bytes_t& pubkey);

private:
    EC_KEY* pKey;
    bool bSet;
};


class secp256k1_point
{
public:
    secp256k1_point() { init(); }
    secp256k1_point(const secp256k1_point& source);
    secp256k1_point(const bytes_t& bytes);
    ~secp256k1_point();

    secp256k1_point& operator=(const secp256k1_point& rhs);

    void bytes(const bytes_t& bytes);
    bytes_t bytes() const;

    secp256k1_point& operator+=(const secp256k1_point& rhs);
    secp256k1_point& operator*=(const bytes_t& rhs);

    const secp256k1_point operator+(const secp256k1_point& rhs) const   { return secp256k1_point(*this) += rhs; }
    const secp256k1_point operator*(const bytes_t& rhs) const           { return secp256k1_point(*this) *= rhs; }

    // Computes n*G + K where K is this and G is the group generator
    void generator_mul(const bytes_t& n);

    // Sets to n*G
    void set_generator_mul(const bytes_t& n);

    bool is_at_infinity() const { return EC_POINT_is_at_infinity(group, point); }
    void set_to_infinity() { EC_POINT_set_to_infinity(group, point); }

    const EC_GROUP* getGroup() const { return group; }
    const EC_POINT* getPoint() const { return point; }

protected:
    void init();

private:
    EC_GROUP* group;
    EC_POINT* point;
    BN_CTX*   ctx;    
};

enum SignatureFlag
{
    SIGNATURE_ENFORCE_LOW_S = 0x1,
};

bytes_t secp256k1_sigToLowS(const bytes_t& signature);

bytes_t secp256k1_sign(const secp256k1_key& key, const bytes_t& data);
bool secp256k1_verify(const secp256k1_key& key, const bytes_t& data, const bytes_t& signature, int flags = 0);

bytes_t secp256k1_rfc6979_k(const secp256k1_key& key, const bytes_t& data);
bytes_t secp256k1_sign_rfc6979(const secp256k1_key& key, const bytes_t& data);

}

