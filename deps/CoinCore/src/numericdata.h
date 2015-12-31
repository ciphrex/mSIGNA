///////////////////////////////////////////////////////////////////////////////
//
// numericdata.h 
//
// Copyright (c) 2012 Eric Lombrozo
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
