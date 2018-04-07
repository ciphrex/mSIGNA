////////////////////////////////////////////////////////////////////////////////
//
// random.h
//
// Copyright (c) 2011-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

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

