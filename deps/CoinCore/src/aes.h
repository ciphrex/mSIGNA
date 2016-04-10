////////////////////////////////////////////////////////////////////////////////
//
// aes.h
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
//
// Uses public domain code by Saju Pillai (saju.pillai@gmail.com)


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

