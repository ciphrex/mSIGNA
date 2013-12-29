////////////////////////////////////////////////////////////////////////////////
//
// secp256k1.h
//
// Copyright (c) 2013 Eric Lombrozo
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

#include <assert.h>
#include <vector>
#include <stdexcept>

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>

#include "typedefs.h"

bool static inline EC_KEY_regenerate_key(EC_KEY* eckey, BIGNUM* priv_key)
{
    if (!eckey) return false;

    const EC_GROUP *group = EC_KEY_get0_group(eckey);

    bool rval = false;
    EC_POINT* pub_key = NULL;
    BN_CTX* ctx = BN_CTX_new();
    if (!ctx) goto finish;

    pub_key = EC_POINT_new(group);
    if (!pub_key) goto finish;

    if (!EC_POINT_mul(group, pub_key, priv_key, NULL, NULL, ctx)) goto finish;

    EC_KEY_set_private_key(eckey, priv_key);
    EC_KEY_set_public_key(eckey, pub_key);

    rval = true;

finish:
    if (pub_key) EC_POINT_free(pub_key);
    if (ctx) BN_CTX_free(ctx);
    return rval;
}

class secp256k1_key
{
public:
    secp256k1_key()
    {
        pKey = EC_KEY_new_by_curve_name(NID_secp256k1);
        if (!pKey) {
            throw std::runtime_error("secp256k1_key::secp256k1_key() : EC_KEY_new_by_curve_name failed.");
        }
        EC_KEY_set_conv_form(pKey, POINT_CONVERSION_COMPRESSED);
        bSet = false;
    }

    ~secp256k1_key()
    {
        EC_KEY_free(pKey);
    }

    EC_KEY* getKey() const { return bSet ? pKey : NULL; }

    EC_KEY* newKey()
    {
        if (!EC_KEY_generate_key(pKey)) {
            throw std::runtime_error("secp256k1_key::newKey() : EC_KEY_generate_key failed.");
        }
        EC_KEY_set_conv_form(pKey, POINT_CONVERSION_COMPRESSED);
        bSet = true;
        return pKey;
    }

    bytes_t getPrivKey() const
    {
        if (!bSet) {
            throw std::runtime_error("secp256k1_key::getPrivKey() : key is not set.");
        }

        const BIGNUM* bn = EC_KEY_get0_private_key(pKey);
        if (!bn) {
            throw std::runtime_error("secp256k1_key::getPrivKey() : EC_KEY_get0_private_key failed.");
        }
        unsigned char privKey[32];
        assert(BN_num_bytes(bn) <= 32);
        BN_bn2bin(bn, privKey);
        return bytes_t(privKey, privKey + 32);
    }

    EC_KEY* setPrivKey(const bytes_t& key)
    {
        BIGNUM* bn = BN_bin2bn(&key[0], key.size(), NULL);
        if (!bn) {
            throw std::runtime_error("secp256k1_key::setPrivKey() : BN_bin2bn failed.");
        }

        bool bFail = !EC_KEY_regenerate_key(pKey, bn);
        BN_clear_free(bn);
        if (bFail) {
            throw std::runtime_error("secp256k1_key::setPrivKey() : EC_KEY_set_private_key failed.");
        }
        bSet = true;
        return pKey;
    }

    bytes_t getPubKey() const
    {
        if (!bSet) {
            throw std::runtime_error("secp256k1_key::getPubKey() : key is not set.");
        }

        int nSize = i2o_ECPublicKey(pKey, NULL);
        if (nSize == 0) {
            throw std::runtime_error("secp256k1_key::getPubKey() : i2o_ECPublicKey failed.");
        }

        bytes_t pubKey(nSize, 0);
        unsigned char* pBegin = &pubKey[0];
        if (i2o_ECPublicKey(pKey, &pBegin) != nSize) {
            throw std::runtime_error("secp256k1_key::getPubKey() : i2o_ECPublicKey returned unexpected size.");
        }
        return pubKey;
    }

private:
    EC_KEY* pKey;
    bool bSet;
};

bytes_t secp256k1_sign(const secp256k1_key& key, const bytes_t& data)
{
    unsigned char signature[1024];
    unsigned int nSize = 0;
    if (!ECDSA_sign(0, (const unsigned char*)&data[0], data.size(), signature, &nSize, key.getKey())) {
        throw std::runtime_error("secp256k1_sign(): ECDSA_sign failed.");
    }
    return bytes_t(signature, signature + nSize);
}

class secp256k1_point
{
public:
    secp256k1_point()
    {
        init();
    }

    secp256k1_point(const secp256k1_point& source)
    {
        init();
        if (!EC_GROUP_copy(group, source.group)) throw std::runtime_error("secp256k1_point::secp256k1_point(const secp256k1_point&) - EC_GROUP_copy failed.");
        if (!EC_POINT_copy(point, source.point)) throw std::runtime_error("secp256k1_point::secp256k1_point(const secp256k1_point&) - EC_POINT_copy failed.");
    }

    secp256k1_point(const bytes_t& bytes)
    {
        init();
        this->bytes(bytes);
    }

    ~secp256k1_point()
    {
        if (point) EC_POINT_free(point);
        if (group) EC_GROUP_free(group);
        if (ctx)   BN_CTX_free(ctx);
    }

    secp256k1_point& operator=(const secp256k1_point& rhs)
    {
        if (!EC_GROUP_copy(group, rhs.group)) throw std::runtime_error("secp256k1_point::operator= - EC_GROUP_copy failed.");
        if (!EC_POINT_copy(point, rhs.point)) throw std::runtime_error("secp256k1_point::operator= - EC_POINT_copy failed.");
    }

    void bytes(const bytes_t& bytes)
    {
        std::string err;

        EC_POINT* rval;

        BIGNUM* bn = BN_bin2bn(&bytes[0], bytes.size(), NULL);
        if (!bn) {
            err = "BN_bin2bn failed.";
            goto finish;
        }

        rval = EC_POINT_bn2point(group, bn, point, ctx);
        if (!rval) {
            err = "EC_POINT_bn2point failed.";
            goto finish;
        }

    finish:
        if (bn) BN_clear_free(bn);

        if (!err.empty()) {
            throw std::runtime_error(std::string("secp256k1_point::set() - ") + err);
        }
    }

    bytes_t bytes() const
    {
        bytes_t bytes(33);

        std::string err;

        BIGNUM* rval;

        BIGNUM* bn = BN_new();
        if (!bn) {
            err = "BN_new failed.";
            goto finish;
        }

        rval = EC_POINT_point2bn(group, point, POINT_CONVERSION_COMPRESSED, bn, ctx);
        if (!rval) {
            err = "EC_POINT_point2bn failed.";
            goto finish;
        }

        assert(BN_num_bytes(bn) == 33);
        BN_bn2bin(bn, &bytes[0]);

    finish:
        if (bn) BN_clear_free(bn);

        if (!err.empty()) {
            throw std::runtime_error(std::string("secp256k1_point::get() - ") + err);
        }

        return bytes; 
    }

    secp256k1_point& operator+=(const secp256k1_point& rhs)
    {
        if (!EC_POINT_add(group, point, point, rhs.point, ctx)) {
            throw std::runtime_error("secp256k1_point::operator+= - EC_POINT_add failed.");
        }
    }

    secp256k1_point& operator*=(const bytes_t& rhs)
    {
        BIGNUM* bn = BN_bin2bn(&rhs[0], rhs.size(), NULL);
        if (!bn) {
            throw std::runtime_error("secp256k1_point::operator*=  - BN_bin2bn failed.");
        }

        int rval = EC_POINT_mul(group, point, NULL, point, bn, ctx);
        BN_clear_free(bn);

        if (rval == 0) {
            throw std::runtime_error("secp256k1_point::operator*=  - EC_POINT_mul failed.");
        }
    }

    const secp256k1_point operator+(const secp256k1_point& rhs) const { return secp256k1_point(*this) += rhs; }
    const secp256k1_point operator*(const bytes_t& rhs) const { return secp256k1_point(*this) *= rhs; }

    // Computes n*G + K where K is this and G is the group generator
    void generator_mul(const bytes_t& n)
    {
        BIGNUM* bn = BN_bin2bn(&n[0], n.size(), NULL);
        if (!bn) throw std::runtime_error("secp256k1_point::generator_mul  - BN_bin2bn failed."); 

        int rval = EC_POINT_mul(group, point, bn, point, BN_value_one(), ctx);
        BN_clear_free(bn);

        if (rval == 0) throw std::runtime_error("secp256k1_point::generator_mul - EC_POINT_mul failed.");
    }

    bool is_at_infinity() const { return EC_POINT_is_at_infinity(group, point); }

protected:
    void init()
    {
        std::string err;

        group = NULL;
        point = NULL;
        ctx   = NULL;

        group = EC_GROUP_new_by_curve_name(NID_secp256k1);
        if (!group) {
            err = "EC_KEY_new_by_curve_name failed.";
            goto finish;
        }

        point = EC_POINT_new(group);
        if (!point) {
            err = "EC_POINT_new failed.";
            goto finish;
        }

        ctx = BN_CTX_new();
        if (!ctx) {
            err = "BN_CTX_new failed.";
            goto finish;
        }

        return;

    finish:
        if (group) EC_GROUP_free(group);
        if (point) EC_POINT_free(point);

        throw std::runtime_error(std::string("secp256k1_point::init() - ") + err);
    }

private:
    EC_GROUP* group;
    EC_POINT* point;
    BN_CTX*   ctx;    
};

#endif // __SECP256K1_H_1
