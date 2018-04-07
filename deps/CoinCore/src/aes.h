////////////////////////////////////////////////////////////////////////////////
//
// aes.h
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//
// Uses public domain code by Saju Pillai (saju.pillai@gmail.com)
//

#pragma once

#include "typedefs.h"
#include "random.h"

#include <stdutils/customerror.h>

namespace AES
{

enum ErrorCodes
{
    AES_INIT_ERROR = 21200,
    AES_DECRYPT_ERROR
};

class AESException : public stdutils::custom_error
{
public:
    virtual ~AESException() throw() { }

protected:
    AESException(const char* what, int code) : stdutils::custom_error(what, code) { }
};

class AESInitException : public AESException
{
public:
    AESInitException() : AESException("Failed to initialize the AES environment.", AES_INIT_ERROR) { }
};

class AESDecryptException : public AESException
{
public:
    AESDecryptException() : AESException("AES decryption failed.", AES_DECRYPT_ERROR) { }
};

uint64_t        random_salt();
bytes_t         encrypt(const secure_bytes_t& key, const secure_bytes_t& plaintext, bool useSalt = false, uint64_t salt = 0);
secure_bytes_t  decrypt(const secure_bytes_t& key, const bytes_t& ciphertext, bool useSalt = false, uint64_t salt = 0);

}

