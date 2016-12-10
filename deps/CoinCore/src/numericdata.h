///////////////////////////////////////////////////////////////////////////////
//
// numericdata.h 
//
// Copyright (c) 2012 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#ifndef __NUMERIC_DATA_H
#define __NUMERIC_DATA_H

#include <vector>

enum {
    BIG_ENDIAN_ = 1,
    LITTLE_ENDIAN_
};

template<typename T>
std::vector<unsigned char> uint_to_vch(T n, uint endianness)
{
    uchar_vector rval;

    for (uint i = 0; i < sizeof(T)-1; i++) {
        rval.push_back(n % 0x100);
        n /= 0x100;
    }
    rval.push_back(n);

    if (endianness == BIG_ENDIAN_)
        rval.reverse();

    return rval;
}

template<typename T>
T vch_to_uint(const std::vector<unsigned char>& vch, uint endianness)
{
    T n = 0;
    uchar_vector bytes(vch.begin(), vch.begin() + sizeof(T));

    if (endianness == LITTLE_ENDIAN_)
        bytes.reverse();

    for (uint i = 0; i < sizeof(T)-1; i++) {
        n += bytes[i];
        n *= 0x100;
    }
    n += bytes[sizeof(T)-1];

    return n;
}

#endif
