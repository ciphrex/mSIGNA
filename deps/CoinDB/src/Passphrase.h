///////////////////////////////////////////////////////////////////////////////
//
// Passphrase.h
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.
//

#pragma once

#include <CoinCore/typedefs.h>
#include <CoinCore/hash.h>

template<typename S>
inline secure_bytes_t passphraseHash(const S& passphrase)
{
    return sha256_2(secure_bytes_t((unsigned char*)&passphrase[0], (unsigned char*)&passphrase[0] + passphrase.size()));
}

