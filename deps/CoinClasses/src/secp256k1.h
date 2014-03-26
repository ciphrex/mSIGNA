////////////////////////////////////////////////////////////////////////////////
//
// secp256k1.h
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

#ifndef __SECP256K1_H_
#define __SECP256K1_H_

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

    EC_KEY* getKey() const { return bSet ? pKey : NULL; }
    EC_KEY* newKey();
    bytes_t getPrivKey() const;
    EC_KEY* setPrivKey(const bytes_t& key);
    bytes_t getPubKey() const;

private:
    EC_KEY* pKey;
    bool bSet;
};

bytes_t secp256k1_sign(const secp256k1_key& key, const bytes_t& data);


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
    bool is_at_infinity() const { return EC_POINT_is_at_infinity(group, point); }

protected:
    void init();

private:
    EC_GROUP* group;
    EC_POINT* point;
    BN_CTX*   ctx;    
};

}

#endif // __SECP256K1_H_1
