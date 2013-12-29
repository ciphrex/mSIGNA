////////////////////////////////////////////////////////////////////////////////
//
// CoinKey.cpp
//
// Copyright (c) 2011-2012 Eric Lombrozo
//
//
// This code was adapted from the satoshi bitcoin client, source file key.h
//   - https://github.com/bitcoin/bitcoin/blob/master/src/key.h
//
// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2011 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.
//

#include "Base58Check.h"
#include "CoinKey.h"

#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>

#define EC_CURVE_NAME NID_secp256k1

bool isValidCoinAddress(const std::string& address, unsigned int addressVersion)
{
    std::vector<unsigned char> keyHash;
    unsigned int version;
    return (fromBase58Check(address, keyHash, version) &&
            (version == addressVersion) &&
            (keyHash.size() == PUBLIC_KEY_HASH_LENGTH));
}

bool static inline EC_KEY_regenerate_key(EC_KEY* eckey, BIGNUM* priv_key)
{
    if (!eckey) return false;

    const EC_GROUP *group = EC_KEY_get0_group(eckey);

    bool rval = false;
    EC_POINT* pub_key = NULL;
    BN_CTX* ctx = BN_CTX_new();
    if (!ctx) goto err;

    pub_key = EC_POINT_new(group);
    if (!pub_key) goto err;

    if (!EC_POINT_mul(group, pub_key, priv_key, NULL, NULL, ctx)) goto err;

    EC_KEY_set_private_key(eckey, priv_key);
    EC_KEY_set_public_key(eckey, pub_key);

    rval = true;

err:
    if (pub_key) EC_POINT_free(pub_key);
    if (ctx) BN_CTX_free(ctx);
    return rval;
}

int static inline ECDSA_SIG_recover_key_GFp(EC_KEY* eckey, ECDSA_SIG* ecsig, const unsigned char* msg, int msglen, int recid, int check)
{
    if (!eckey) return 0;

    int ret = 0;
    BN_CTX *ctx = NULL;

    BIGNUM *x = NULL;
    BIGNUM *e = NULL;
    BIGNUM *order = NULL;
    BIGNUM *sor = NULL;
    BIGNUM *eor = NULL;
    BIGNUM *field = NULL;
    EC_POINT *R = NULL;
    EC_POINT *O = NULL;
    EC_POINT *Q = NULL;
    BIGNUM *rr = NULL;
    BIGNUM *zero = NULL;
    int n = 0;
    int i = recid / 2;

    const EC_GROUP *group = EC_KEY_get0_group(eckey);
    if ((ctx = BN_CTX_new()) == NULL) { ret = -1; goto err; }
    BN_CTX_start(ctx);
    order = BN_CTX_get(ctx);
    if (!EC_GROUP_get_order(group, order, ctx)) { ret = -2; goto err; }
    x = BN_CTX_get(ctx);
    if (!BN_copy(x, order)) { ret=-1; goto err; }
    if (!BN_mul_word(x, i)) { ret=-1; goto err; }
    if (!BN_add(x, x, ecsig->r)) { ret=-1; goto err; }
    field = BN_CTX_get(ctx);
    if (!EC_GROUP_get_curve_GFp(group, field, NULL, NULL, ctx)) { ret=-2; goto err; }
    if (BN_cmp(x, field) >= 0) { ret=0; goto err; }
    if ((R = EC_POINT_new(group)) == NULL) { ret = -2; goto err; }
    if (!EC_POINT_set_compressed_coordinates_GFp(group, R, x, recid % 2, ctx)) { ret=0; goto err; }
    if (check)
    {
        if ((O = EC_POINT_new(group)) == NULL) { ret = -2; goto err; }
        if (!EC_POINT_mul(group, O, NULL, R, order, ctx)) { ret=-2; goto err; }
        if (!EC_POINT_is_at_infinity(group, O)) { ret = 0; goto err; }
    }
    if ((Q = EC_POINT_new(group)) == NULL) { ret = -2; goto err; }
    n = EC_GROUP_get_degree(group);
    e = BN_CTX_get(ctx);
    if (!BN_bin2bn(msg, msglen, e)) { ret=-1; goto err; }
    if (8*msglen > n) BN_rshift(e, e, 8-(n & 7));
    zero = BN_CTX_get(ctx);
    if (!BN_zero(zero)) { ret=-1; goto err; }
    if (!BN_mod_sub(e, zero, e, order, ctx)) { ret=-1; goto err; }
    rr = BN_CTX_get(ctx);
    if (!BN_mod_inverse(rr, ecsig->r, order, ctx)) { ret=-1; goto err; }
    sor = BN_CTX_get(ctx);
    if (!BN_mod_mul(sor, ecsig->s, rr, order, ctx)) { ret=-1; goto err; }
    eor = BN_CTX_get(ctx);
    if (!BN_mod_mul(eor, e, rr, order, ctx)) { ret=-1; goto err; }
    if (!EC_POINT_mul(group, Q, eor, R, sor, ctx)) { ret=-2; goto err; }
    if (!EC_KEY_set_public_key(eckey, Q)) { ret=-2; goto err; }

    ret = 1;

err:
    if (ctx) {
        BN_CTX_end(ctx);
        BN_CTX_free(ctx);
    }
    if (R != NULL) EC_POINT_free(R);
    if (O != NULL) EC_POINT_free(O);
    if (Q != NULL) EC_POINT_free(Q);
    return ret;
}

CoinKey::CoinKey(unsigned int addressVersion, unsigned int walletImportVersion)
{
    this->bCompressed = true;
    this->addressVersion = addressVersion;
    this->walletImportVersion = walletImportVersion;
    this->pKey = EC_KEY_new_by_curve_name(EC_CURVE_NAME);
    if (!this->pKey)
        throw CoinKeyError("CoinKey::CoinKey() : EC_KEY_new_by_curve_name failed");
    this->bSet = false;
}

CoinKey::CoinKey(const CoinKey& other)
{
    this->bCompressed = other.bCompressed;
    this->pKey = EC_KEY_dup(other.pKey);
    if (!this->pKey)
        throw CoinKeyError("CoinKey::CoinKey(const CoinKey&) : EC_KEY_dup failed");
    this->bSet = other.bSet;
}

CoinKey& CoinKey::operator=(const CoinKey& other)
{
    this->bCompressed = other.bCompressed;
    if (!EC_KEY_copy(this->pKey, other.pKey))
        throw CoinKeyError("CoinKey::operator=(const CoinKey&) : EC_KEY_copy failed");
    this->bSet = other.bSet;
    return (*this);
}

CoinKey::~CoinKey()
{
    EC_KEY_free(this->pKey);
}

bool CoinKey::isSet() const
{
    return this->bSet;
}

void CoinKey::generateNewKey()
{
    if (!EC_KEY_generate_key(this->pKey))
        throw CoinKeyError("CoinKey::generateNewKey() : EC_KEY_generate_key failed");
    this->bSet = true;
}

// determines whether we're using a full 279-byte DER key or a shorter 32-byte one according to the length of privateKey
bool CoinKey::setPrivateKey(const uchar_vector_secure& privateKey, bool bCompressed)
{
    if (privateKey.size() == PRIVATE_KEY_DER_LENGTH) {
        const unsigned char* pBegin = &privateKey[0];
        if (!d2i_ECPrivateKey(&this->pKey, &pBegin, privateKey.size()))
            return false;
        this->bSet = true;
        this->bCompressed = bCompressed;
        return true;
    }
    else if (privateKey.size() == PRIVATE_KEY_LENGTH) {
        EC_KEY_free(this->pKey);
        this->pKey = EC_KEY_new_by_curve_name(EC_CURVE_NAME);
        if (this->pKey == NULL)
            throw CoinKeyError("CoinKey::setPrivateKey() : EC_KEY_new_by_curve_name failed");
        BIGNUM* bn = BN_bin2bn(&privateKey[0], PRIVATE_KEY_LENGTH, BN_new());
        if (!bn)
            throw CoinKeyError("CoinKey::setPrivateKey() : BN_bin2bn failed");
        if (!EC_KEY_regenerate_key(this->pKey, bn))
            throw CoinKeyError("CoinKey::setPrivateKey() : EC_KEY_regenerate_key failed");
        BN_clear_free(bn);
        this->bCompressed = bCompressed;
        this->bSet = true;
        return true;
    }
    throw CoinKeyError("CoinKey::setPrivateKey() : invalid key length");
}

uchar_vector_secure CoinKey::getPrivateKey(unsigned int privateKeyFormat) const
{
    if (!this->bSet) {
        throw CoinKeyError("CoinLey::getPrivateKey() : key not set.");
    }
    if (privateKeyFormat == PRIVATE_KEY_DER_279) {
        int nSize = i2d_ECPrivateKey(this->pKey, NULL);
        if (!nSize)
            throw CoinKeyError("CoinKey::getPrivateKey() : i2d_ECPrivateKey failed");
        uchar_vector_secure privateKey(nSize, 0);
        unsigned char* pBegin = &privateKey[0];
        if (i2d_ECPrivateKey(this->pKey, &pBegin) != nSize)
            throw CoinKeyError("CoinKey::getPrivateKey() : i2d_ECPrivateKey returned unexpected size");
        return privateKey;
    }
    else if (privateKeyFormat == PRIVATE_KEY_32) {
        uchar_vector_secure privateKey(PRIVATE_KEY_LENGTH, 0);
        const BIGNUM *bn = EC_KEY_get0_private_key(this->pKey);
        if (!bn)
            throw CoinKeyError("CoinKey::getPrivateKey() : EC_KEY_get0_private_key failed");
        int nBytes = BN_num_bytes(bn);
        int n = BN_bn2bin(bn, &privateKey[PRIVATE_KEY_LENGTH - nBytes]);
        if (n != nBytes)
            throw CoinKeyError("CoinKey::getPrivateKey(): BN_bn2bin failed");
        return privateKey;
    }
    throw CoinKeyError("CoinKey::getPrivateKey() : invalid key format");
}

bool CoinKey::setWalletImport(const string_secure& walletImport)
{
    uchar_vector_secure privateKey;
    bool bCompressed;

    if (!fromBase58Check(walletImport, privateKey, this->walletImportVersion))
        return false;

    if (privateKey.size() == PRIVATE_KEY_LENGTH) {
        bCompressed = false;
    }
    else if (privateKey.size() == PRIVATE_KEY_LENGTH + 1 && privateKey.back() == 0x01) {
        bCompressed = true;
        privateKey.pop_back();
    }
    else {
        return false;
    }

    return this->setPrivateKey(privateKey, bCompressed);
}

string_secure CoinKey::getWalletImport() const
{
    uchar_vector_secure privKey = this->getPrivateKey(PRIVATE_KEY_32);
    if (bCompressed) privKey.push_back(0x01);
    return toBase58Check(privKey, this->walletImportVersion);
}

bool CoinKey::setPublicKey(const uchar_vector& publicKey)
{
    const unsigned char* pBegin = &publicKey[0];
    if (!o2i_ECPublicKey(&this->pKey, &pBegin, publicKey.size()))
        return false;
    this->bCompressed = (publicKey.size() == 33);
    this->bSet = true;
    return true;
}

uchar_vector CoinKey::getPublicKey() const
{
    if (!this->bSet) {
        throw CoinKeyError("CoinKey::getPublicKey() : key is not set.");
    }
    EC_KEY_set_conv_form(this->pKey, this->bCompressed ? POINT_CONVERSION_COMPRESSED : POINT_CONVERSION_UNCOMPRESSED); 
    int nSize = i2o_ECPublicKey(this->pKey, NULL);
    if (nSize == 0)
        throw CoinKeyError("CoinKey::getPublicKey() : i2o_ECPublicKey failed");
    uchar_vector publicKey(nSize, 0);
    unsigned char* pBegin = &publicKey[0];
    if (i2o_ECPublicKey(this->pKey, &pBegin) != nSize)
        throw CoinKeyError("CoinKey::getPublicKey() : i2o_ECPublicKey returned unexpected size");
    return publicKey;
}

std::string CoinKey::getAddress() const
{
    return toBase58Check(ripemd160(sha256(this->getPublicKey())), this->addressVersion);
}

bool CoinKey::sign(const uchar_vector& digest, uchar_vector& signature)
{
    signature.clear();
    unsigned char buffer[10000];
    unsigned int nSize = 0;
    if (!ECDSA_sign(0, (unsigned char*)&digest[0], digest.size(), buffer, &nSize, this->pKey))
        return false;
    signature.resize(nSize);
    memcpy(&signature[0], buffer, nSize);
    return true;
}

// create a compact signature (65 bytes), which allows reconstructing the used public key
bool CoinKey::signCompact(const uchar_vector& digest, uchar_vector& signature)
{
    bool rval = false;
    ECDSA_SIG* pSig = ECDSA_do_sign((unsigned char*)&digest[0], digest.size(), this->pKey);
    if (!pSig) return false;

    signature.clear();
    signature.resize(65,0);
    int nBitsR = BN_num_bits(pSig->r);
    int nBitsS = BN_num_bits(pSig->s);
    if (nBitsR <= 256 && nBitsS <= 256) {
        int nRecId = -1;
        for (int i = 0; i < 4; i++) {
            CoinKey recoveredKey;
            recoveredKey.bSet = true;
            if (ECDSA_SIG_recover_key_GFp(recoveredKey.pKey, pSig, (unsigned char*)&digest[0], digest.size(), i, 1) == 1) {
                if (recoveredKey.getPublicKey() == this->getPublicKey()) {
                    nRecId = i;
                    break;
                }
            }
        }

        if (nRecId == -1)
        throw CoinKeyError("CoinKey::signCompact() : unable to construct recoverable key");

        signature[0] = nRecId + 27;
        BN_bn2bin(pSig->r, &signature[33 - (nBitsR + 7) / 8]);
        BN_bn2bin(pSig->s, &signature[65 - (nBitsS + 7) / 8]);
        rval = true;
    }
    ECDSA_SIG_free(pSig);
    return rval;
}

// reconstruct public key from a compact signature
bool CoinKey::setCompactSignature(const uchar_vector& digest, const uchar_vector& signature)
{
    if (signature.size() != 65) return false;
    if (signature[0] < 27 || signature[0] >= 31) return false;

    ECDSA_SIG *pSig = ECDSA_SIG_new();
    BN_bin2bn(&signature[1], 32, pSig->r);
    BN_bin2bn(&signature[33], 32, pSig->s);

    EC_KEY_free(this->pKey);
    this->pKey = EC_KEY_new_by_curve_name(EC_CURVE_NAME);
    if (ECDSA_SIG_recover_key_GFp(this->pKey, pSig, (unsigned char*)&digest[0], digest.size(), signature[0] - 27, 0) == 1) {
        this->bSet = true;
        ECDSA_SIG_free(pSig);
        return true;
    }
    return false;
}

// Return values: -1 = error, 0 = invalid signature, 1 = valid
int CoinKey::verify(const uchar_vector& digest, const uchar_vector& signature)
{
    return ECDSA_verify(0, (unsigned char*)&digest[0], digest.size(), &signature[0], signature.size(), this->pKey);
}

bool CoinKey::verifyCompact(const uchar_vector& digest, const uchar_vector& signature)
{
    CoinKey key;
    if (!key.setCompactSignature(digest, signature))
        return false;
    if (this->getPublicKey() != key.getPublicKey())
        return false;
    return true;
}
