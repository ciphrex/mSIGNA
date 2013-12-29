////////////////////////////////////////////////////////////////////////////////
//
// BloomFilter.cpp
//
// Copyright (c) 2013 Eric Lombrozo
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

#include "BloomFilter.h"
#include <cmath>
#include <algorithm>

#define LN2SQUARED 0.4804530139182014246671025263266649717305529515945455
#define LN2 0.6931471805599453094172321214581765680755001343602552

using namespace Coin;

static const unsigned char bit_mask[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

inline uint32_t ROTL32(uint32_t x, int8_t r)
{
    return (x << r) | (x >> (32 - r));
}

uint32_t murmurHash3(uint32_t seed, const uchar_vector& data)
{
    // The following is MurmurHash3 (x86_32), see http://code.google.com/p/smhasher/source/browse/trunk/MurmurHash3.cpp
    uint32_t h1 = seed;
    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    const int nblocks = data.size() / 4;

    //----------
    // body
    const uint32_t * blocks = (const uint32_t *)(&data[0] + nblocks*4);

    for(int i = -nblocks; i; i++)
    {
        uint32_t k1 = blocks[i];

        k1 *= c1;
        k1 = ROTL32(k1,15);
        k1 *= c2;

        h1 ^= k1;
        h1 = ROTL32(h1,13);
        h1 = h1*5+0xe6546b64;
    }

    //----------
    // tail
    const uint8_t * tail = (const uint8_t*)(&data[0] + nblocks*4);

    uint32_t k1 = 0;

    switch(data.size() & 3)
    {
    case 3: k1 ^= tail[2] << 16;
    case 2: k1 ^= tail[1] << 8;
    case 1: k1 ^= tail[0];
            k1 *= c1; k1 = ROTL32(k1,15); k1 *= c2; h1 ^= k1;
    };

    //----------
    // finalization
    h1 ^= data.size();
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;

    return h1;
}


inline uint32_t BloomFilter::hash(uint n, const uchar_vector& data) const
{
    return murmurHash3(n * 0xfba4c795 + nTweak, data) % (filter.size() * 8);
}

BloomFilter::BloomFilter(uint32_t nElements, double falsePositiveRate, uint32_t _nTweak, uint8_t _nFlags) :
    bSet(true),
    filter(std::min((uint)(-1 / LN2SQUARED * nElements * log(falsePositiveRate)), MAX_BLOOM_FILTER_SIZE * 8) / 8, 0),
    bFull(false),
    bEmpty(false),
    nHashFuncs(std::min((uint)(filter.size() * 8 / nElements * LN2), MAX_BLOOM_FILTER_HASH_FUNCS)),
    nTweak(_nTweak),
    nFlags(_nFlags)
{
}

void BloomFilter::set(uint32_t nElements, double falsePositiveRate, uint32_t _nTweak, uint8_t _nFlags)
{
    filter = uchar_vector(std::min((uint)(-1 / LN2SQUARED * nElements * log(falsePositiveRate)), MAX_BLOOM_FILTER_SIZE * 8) / 8, 0);
    bFull = false;
    bEmpty = false;
    nHashFuncs = std::min((uint)(filter.size() * 8 / nElements * LN2), MAX_BLOOM_FILTER_HASH_FUNCS);
    nTweak = _nTweak;
    nFlags = _nFlags;
    bSet = true;
}

void BloomFilter::insert(const uchar_vector& data)
{
    if (bFull) return;
    for (uint i = 0; i < nHashFuncs; i++) {
        uint index = hash(i, data);
        filter[index >> 3] |= bit_mask[7 & index];
    }
    bEmpty = false;
}

bool BloomFilter::match(const uchar_vector& data) const
{
    if (bFull) return true;
    if (bEmpty) return false;

    for (uint i = 0; i < nHashFuncs; i++) {
        uint index = hash(i, data);
        if (!(filter[index >> 3] & bit_mask[7 & index])) return false;
    }
    return true;
}
