////////////////////////////////////////////////////////////////////////////////
//
// aes.cpp
//
// Copyright (c) 2014 Eric Lombrozo
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

// Uses public domain code by Saju Pillai (saju.pillai@gmail.com)

// TODO: Use secure buffers

#include "aes.h"
#include "random.h"

#include <openssl/aes.h>
#include <openssl/evp.h>

#include <cstring>

using namespace std;

namespace AES
{

uint64_t random_salt()
{
    uint64_t salt;
    do
    {
        secure_bytes_t bytes = random_bytes(8);
        memcpy((void*)&salt, (const void*)&bytes[0], 8);
    }
    while (salt == 0);
    return salt;
}

/**
 * Create an 256 bit key and IV using the supplied key_data. salt can be added for taste.
 * Fills in the encryption and decryption ctx objects and returns 0 on success
 **/
void init(const unsigned char* key_data, int key_data_len, const unsigned char* salt, EVP_CIPHER_CTX* e_ctx, EVP_CIPHER_CTX* d_ctx)
{
    int nrounds = 5;
    unsigned char key[32], iv[32];

    /*
    * Gen key & IV for AES 256 CBC mode. A SHA1 digest is used to hash the supplied key material.
    * nrounds is the number of times the we hash the material. More rounds are more secure but
    * slower.
    */
    int i = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), salt, key_data, key_data_len, nrounds, key, iv);
    if (i != 32) throw AESInitException();

    EVP_CIPHER_CTX_init(e_ctx);
    EVP_EncryptInit_ex(e_ctx, EVP_aes_256_cbc(), NULL, key, iv);
    EVP_CIPHER_CTX_init(d_ctx);
    EVP_DecryptInit_ex(d_ctx, EVP_aes_256_cbc(), NULL, key, iv);
}

/*
 * Encrypt *len bytes of data
 * All data going in & out is considered binary (unsigned char[])
 */
unsigned char* encrypt(EVP_CIPHER_CTX* e, unsigned char* plaintext, int* len)
{
    /* max ciphertext len for a n bytes of plaintext is n + AES_BLOCK_SIZE -1 bytes */
    int c_len = *len + AES_BLOCK_SIZE, f_len = 0;
    unsigned char* ciphertext = (unsigned char*)malloc(c_len);

    /* allows reusing of 'e' for multiple encryption cycles */
    if (!EVP_EncryptInit_ex(e, NULL, NULL, NULL, NULL))
    {
        throw std::runtime_error("EVP_EncryptInit_ex() failed.");
    }

    /* update ciphertext, c_len is filled with the length of ciphertext generated,
    *len is the size of plaintext in bytes */
    if (!EVP_EncryptUpdate(e, ciphertext, &c_len, plaintext, *len))
    {
        throw std::runtime_error("EVP_EncryptUpdate() failed.");
    }

    /* update ciphertext with the final remaining bytes */
    if (!EVP_EncryptFinal_ex(e, ciphertext+c_len, &f_len))
    {
        throw std::runtime_error("EVP_EncryptFinal_ex() failed.");
    }

    *len = c_len + f_len;
    return ciphertext;
}

bytes_t encrypt(const secure_bytes_t& key, const secure_bytes_t& plaintext, bool useSalt, uint64_t salt)
{
    EVP_CIPHER_CTX en, de;

    int key_len = key.size();

    /* gen key and iv. init the cipher ctx object */
    const unsigned char* pSalt = useSalt ? (const unsigned char*)&salt : nullptr;
    init(&key[0], key_len, pSalt, &en, &de);

    int len = plaintext.size();
    unsigned char* ciphertext_ = encrypt(&en, (unsigned char*)&plaintext[0], &len);

    secure_bytes_t ciphertext(ciphertext_, ciphertext_ + len);

    free(ciphertext_);
    EVP_CIPHER_CTX_cleanup(&en);
    EVP_CIPHER_CTX_cleanup(&de);

    return ciphertext;
}

/*
 * Decrypt *len bytes of ciphertext
 */
unsigned char* decrypt(EVP_CIPHER_CTX* e, unsigned char* ciphertext, int* len)
{
    /* because we have padding ON, we must allocate an extra cipher block size of memory */
    int p_len = *len, f_len = 0;
    unsigned char *plaintext = (unsigned char*)malloc(p_len + AES_BLOCK_SIZE);

    if (!EVP_DecryptInit_ex(e, NULL, NULL, NULL, NULL))
    {
        throw std::runtime_error("EVP_DecryptInit_ex() failed.");
    }

    if (!EVP_DecryptUpdate(e, plaintext, &p_len, ciphertext, *len))
    {
        throw std::runtime_error("EVP_DecryptUpdate() failed.");
    }

    if (!EVP_DecryptFinal_ex(e, plaintext + p_len, &f_len))
    {
        throw AESDecryptException();
    }

    *len = p_len + f_len;
    return plaintext;
}

secure_bytes_t decrypt(const secure_bytes_t& key, const bytes_t& ciphertext, bool useSalt, uint64_t salt)
{
    EVP_CIPHER_CTX en, de;

    int key_len = key.size();

    /* gen key and iv. init the cipher ctx object */
    const unsigned char* pSalt = useSalt ? (const unsigned char*)&salt : nullptr;
    init(&key[0], key_len, pSalt, &en, &de);


    //EVP_CIPHER_CTX_set_padding(&de, 0);
    int len = ciphertext.size();
    unsigned char* plaintext_ = decrypt(&de, (unsigned char*)&ciphertext[0], &len);

    if (len <= 0)
    {
        free(plaintext_);
        EVP_CIPHER_CTX_cleanup(&en);
        EVP_CIPHER_CTX_cleanup(&de);
        throw AESDecryptException();
    }

    secure_bytes_t plaintext(plaintext_, plaintext_ + len);

    free(plaintext_);
    EVP_CIPHER_CTX_cleanup(&en);
    EVP_CIPHER_CTX_cleanup(&de);

    return plaintext;
}

}
