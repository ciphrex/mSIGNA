////////////////////////////////////////////////////////////////////////////////
//
// secp256k1_openssl.cpp
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

#include "secp256k1_openssl.h"
#include "hash.h"

#include <string>
#include <cassert>

#ifdef TRACE_RFC6979
  #include <iostream>
#endif

using namespace CoinCrypto;

const uchar_vector SECP256K1_FIELD_MOD("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F");
const uchar_vector SECP256K1_GROUP_ORDER("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141");
const uchar_vector SECP256K1_GROUP_HALFORDER("7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF5D576E7357A4501DDFE92F46681B20A0");

bool static EC_KEY_regenerate_key(EC_KEY* eckey, BIGNUM* priv_key)
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

    
secp256k1_key::secp256k1_key()
{
    pKey = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!pKey) {
        throw std::runtime_error("secp256k1_key::secp256k1_key() : EC_KEY_new_by_curve_name failed.");
    }
    EC_KEY_set_conv_form(pKey, POINT_CONVERSION_COMPRESSED);
    bSet = false;
}

EC_KEY* secp256k1_key::newKey()
{
    if (!EC_KEY_generate_key(pKey)) {
        throw std::runtime_error("secp256k1_key::newKey() : EC_KEY_generate_key failed.");
    }
    EC_KEY_set_conv_form(pKey, POINT_CONVERSION_COMPRESSED);
    bSet = true;
    return pKey;
}

bytes_t secp256k1_key::getPrivKey() const
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

EC_KEY* secp256k1_key::setPrivKey(const bytes_t& privkey)
{
    BIGNUM* bn = BN_bin2bn(&privkey[0], privkey.size(), NULL);
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

bytes_t secp256k1_key::getPubKey(bool bCompressed) const
{
    if (!bSet) {
        throw std::runtime_error("secp256k1_key::getPubKey() : key is not set.");
    }

    if (!bCompressed) EC_KEY_set_conv_form(pKey, POINT_CONVERSION_UNCOMPRESSED);
    int nSize = i2o_ECPublicKey(pKey, NULL);
    if (nSize == 0) {
        if (!bCompressed) EC_KEY_set_conv_form(pKey, POINT_CONVERSION_COMPRESSED);
        throw std::runtime_error("secp256k1_key::getPubKey() : i2o_ECPublicKey failed.");
    }

    bytes_t pubKey(nSize, 0);
    unsigned char* pBegin = &pubKey[0];
    if (i2o_ECPublicKey(pKey, &pBegin) != nSize) {
        if (!bCompressed) EC_KEY_set_conv_form(pKey, POINT_CONVERSION_COMPRESSED);
        throw std::runtime_error("secp256k1_key::getPubKey() : i2o_ECPublicKey returned unexpected size.");
    }

    if (!bCompressed) EC_KEY_set_conv_form(pKey, POINT_CONVERSION_COMPRESSED);
    return pubKey;
}

EC_KEY* secp256k1_key::setPubKey(const bytes_t& pubkey)
{
    if (pubkey.empty()) throw std::runtime_error("secp256k1_key::setPubKey() : pubkey is empty.");

    const unsigned char* pBegin = (unsigned char*)&pubkey[0];
    if (!o2i_ECPublicKey(&pKey, &pBegin, pubkey.size())) throw std::runtime_error("secp256k1_key::setPubKey() : o2i_ECPublicKey failed.");
    bSet = true;
    return pKey;
}



secp256k1_point::secp256k1_point(const secp256k1_point& source)
{
    init();
    if (!EC_GROUP_copy(group, source.group)) throw std::runtime_error("secp256k1_point::secp256k1_point(const secp256k1_point&) - EC_GROUP_copy failed.");
    if (!EC_POINT_copy(point, source.point)) throw std::runtime_error("secp256k1_point::secp256k1_point(const secp256k1_point&) - EC_POINT_copy failed.");
}

secp256k1_point::secp256k1_point(const bytes_t& bytes)
{
    init();
    this->bytes(bytes);
}

    
secp256k1_point::~secp256k1_point()
{
    if (point) EC_POINT_free(point);
    if (group) EC_GROUP_free(group);
    if (ctx)   BN_CTX_free(ctx);
}

secp256k1_point& secp256k1_point::operator=(const secp256k1_point& rhs)
{
    if (!EC_GROUP_copy(group, rhs.group)) throw std::runtime_error("secp256k1_point::operator= - EC_GROUP_copy failed.");
    if (!EC_POINT_copy(point, rhs.point)) throw std::runtime_error("secp256k1_point::operator= - EC_POINT_copy failed.");

    return *this;
}

void secp256k1_point::bytes(const bytes_t& bytes)
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

bytes_t secp256k1_point::bytes() const
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

secp256k1_point& secp256k1_point::operator+=(const secp256k1_point& rhs)
{
    if (!EC_POINT_add(group, point, point, rhs.point, ctx)) {
        throw std::runtime_error("secp256k1_point::operator+= - EC_POINT_add failed.");
    }
    return *this;
}

secp256k1_point& secp256k1_point::operator*=(const bytes_t& rhs)
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

    return *this;
}

// Computes n*G + K where K is this and G is the group generator
void secp256k1_point::generator_mul(const bytes_t& n)
{
    BIGNUM* bn = BN_bin2bn(&n[0], n.size(), NULL);
    if (!bn) throw std::runtime_error("secp256k1_point::generator_mul  - BN_bin2bn failed."); 

    //int rval = EC_POINT_mul(group, point, bn, (is_at_infinity() ? NULL : point), BN_value_one(), ctx);
    int rval = EC_POINT_mul(group, point, bn, point, BN_value_one(), ctx);
    BN_clear_free(bn);

    if (rval == 0) throw std::runtime_error("secp256k1_point::generator_mul - EC_POINT_mul failed.");
}

// Sets to n*G
void secp256k1_point::set_generator_mul(const bytes_t& n)
{
    BIGNUM* bn = BN_bin2bn(&n[0], n.size(), NULL);
    if (!bn) throw std::runtime_error("secp256k1_point::set_generator_mul  - BN_bin2bn failed."); 

    int rval = EC_POINT_mul(group, point, bn, NULL, NULL, ctx);
    BN_clear_free(bn);

    if (rval == 0) throw std::runtime_error("secp256k1_point::set_generator_mul - EC_POINT_mul failed.");
}

void secp256k1_point::init()
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

bytes_t CoinCrypto::secp256k1_sigToLowS(const bytes_t& signature)
{
    const unsigned char* pvch = (const unsigned char*)&signature[0];
    ECDSA_SIG* sig = d2i_ECDSA_SIG(NULL, &pvch, signature.size());
    if (!sig) throw std::runtime_error("secp256k1_sigToLowS(): d2i_ECDSA_SIG failed.");

    BIGNUM* order = BN_bin2bn(&SECP256K1_GROUP_ORDER[0], SECP256K1_GROUP_ORDER.size(), NULL);
    if (!order)
    {
        ECDSA_SIG_free(sig);
        throw std::runtime_error("secp256k1_sigToLowS(): BN_bin2bn failed.");
    }

    BIGNUM* halforder = BN_bin2bn(&SECP256K1_GROUP_HALFORDER[0], SECP256K1_GROUP_HALFORDER.size(), NULL);
    if (!halforder)
    {
        ECDSA_SIG_free(sig);
        BN_clear_free(order);
        throw std::runtime_error("secp256k1_sigToLowS(): BN_bin2bn failed.");
    }
    
    if (BN_cmp(sig->s, halforder) > 0) { BN_sub(sig->s, order, sig->s); }

    BN_clear_free(order);
    BN_clear_free(halforder);

    unsigned char buffer[1024];
    unsigned char* pos = &buffer[0];
    int nSize = i2d_ECDSA_SIG(sig, &pos);
    ECDSA_SIG_free(sig);

    return bytes_t(buffer, buffer + nSize);
}

// Signing function
bytes_t CoinCrypto::secp256k1_sign(const secp256k1_key& key, const bytes_t& data)
{
    unsigned char signature[1024];
    unsigned int nSize = 0;
    if (!ECDSA_sign(0, (const unsigned char*)&data[0], data.size(), signature, &nSize, key.getKey())) {
        throw std::runtime_error("secp256k1_sign(): ECDSA_sign failed.");
    }
    return secp256k1_sigToLowS(bytes_t(signature, signature + nSize));
}

// Verification function
bool CoinCrypto::secp256k1_verify(const secp256k1_key& key, const bytes_t& data, const bytes_t& signature, int flags)
{
    if (flags & SIGNATURE_ENFORCE_LOW_S)
    {
        if (signature != secp256k1_sigToLowS(signature)) return false;
    }

    int rval = ECDSA_verify(0, (const unsigned char*)&data[0], data.size(), (const unsigned char*)&signature[0], signature.size(), key.getKey());
    if (rval == -1) throw std::runtime_error("secp256k1_verify(): ECDSA_verify error.");
    return (rval == 1);
}

bytes_t CoinCrypto::secp256k1_rfc6979_k(const secp256k1_key& key, const bytes_t& data)
{
    uchar_vector hash = sha256(data);
    uchar_vector v("0101010101010101010101010101010101010101010101010101010101010101");
    uchar_vector k("0000000000000000000000000000000000000000000000000000000000000000");
    uchar_vector privkey = key.getPrivKey();
    k = hmac_sha256(k, v + uchar_vector("00") + privkey + hash);
    v = hmac_sha256(k, v);
    k = hmac_sha256(k, v + uchar_vector("01") + privkey + hash);
    v = hmac_sha256(k, v);
    v = hmac_sha256(k, v);

    return v; 
}


bytes_t CoinCrypto::secp256k1_sign_rfc6979(const secp256k1_key& key, const bytes_t& data)
{
    bytes_t k = secp256k1_rfc6979_k(key, data); 
    BIGNUM* bn = BN_bin2bn(&k[0], k.size(), NULL);
    if (!bn) throw std::runtime_error("secp256k1_sign_rfc6979() : BN_bin2bn failed for k.");

    BIGNUM* q = BN_bin2bn(&SECP256K1_GROUP_ORDER[0], SECP256K1_GROUP_ORDER.size(), NULL);
    if (!q)
    {
        BN_clear_free(bn);
        throw std::runtime_error("secp256k1_sign_rfc6979() : BN_bin2bn failed for field modulus.");
    }

    BN_CTX* ctx = BN_CTX_new();
    if (!ctx)
    {
        BN_clear_free(bn);
        BN_clear_free(q);
        throw std::runtime_error("secp256k1_sign_rfc6979() : BN_CTX_new failed.");
    }

    BIGNUM* kinv = BN_mod_inverse(NULL, bn, q, ctx);
    BN_clear_free(bn);
    BN_clear_free(q);
    BN_CTX_free(ctx);

    if (!kinv) throw std::runtime_error("secp256k1_sign_rfc6979() : BN_mod_inverse failed.");

    unsigned char kinvbytes[32];
    assert(BN_num_bytes(kinv) <= 32);
    BN_bn2bin(kinv, kinvbytes);
    bytes_t kinv_(kinvbytes, kinvbytes + 32);
#ifdef TRACE_RFC6979
    std::cout << "--------------------" << std::endl << "kinv = " << uchar_vector(kinv_).getHex() << std::endl;
#endif

    secp256k1_point point;
    point.set_generator_mul(k);
    BIGNUM* rp = BN_new();
    if (!EC_POINT_get_affine_coordinates_GFp(point.getGroup(), point.getPoint(), rp, NULL, NULL))
    {
        BN_clear_free(rp);
        throw std::runtime_error("secp256k1_sign_rfc6979() : EC_POINT_get_affine_coordinates_GFp failed.");
    }

    unsigned char rpbytes[32];
    assert(BN_num_bytes(rp) <= 32);
    BN_bn2bin(rp, rpbytes);
    bytes_t rp_(rpbytes, rpbytes + 32);
#ifdef TRACE_RFC6979
    std::cout << "--------------------" << std::endl << "rp = " << uchar_vector(rp_).getHex() << std::endl;
#endif

    unsigned char signature[1024];
    unsigned int nSize = 0;
    int res = ECDSA_sign_ex(0, (const unsigned char*)&data[0], data.size(), signature, &nSize, kinv, rp, key.getKey());

    BN_clear_free(kinv);
    BN_clear_free(rp);

    if (!res) throw std::runtime_error("secp256k1_sign_rfc6979(): ECDSA_sign_ex failed.");

    return secp256k1_sigToLowS(bytes_t(signature, signature + nSize));
}
    
