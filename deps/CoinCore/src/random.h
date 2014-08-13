////////////////////////////////////////////////////////////////////////////////
//
// random.h
//
// Copyright (c) 2011-2014 Eric Lombrozo
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

#pragma once

#include "typedefs.h"

#include <openssl/rand.h>
#include <openssl/err.h>

#include <stdexcept>

inline bytes_t random_bytes(int length)
{
    bytes_t r(length);
    if (!RAND_bytes(&r[0], length)) { 
        throw std::runtime_error(ERR_error_string(ERR_get_error(), NULL));
    }

    return r;
}

inline secure_bytes_t secure_random_bytes(int length)
{
    secure_bytes_t r(length);
    if (!RAND_bytes(&r[0], length)) { 
        throw std::runtime_error(ERR_error_string(ERR_get_error(), NULL));
    }

    return r;
}

