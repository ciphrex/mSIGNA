///////////////////////////////////////////////////////////////////////////////
//
// Passphrase.h
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include <CoinCore/typedefs.h>
#include <CoinCore/hash.h>

template<typename S>
inline secure_bytes_t passphraseHash(const S& passphrase)
{
    return sha256_2(secure_bytes_t((unsigned char*)&passphrase[0], (unsigned char*)&passphrase[0] + passphrase.size()));
}

