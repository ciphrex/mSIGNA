////////////////////////////////////////////////////////////////////////////////
//
// BloomFilter.h
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

#ifndef BLOOM_FILTER_H__
#define BLOOM_FILTER_H__

#include <stdutils/uchar_vector.h>

namespace Coin {

// 20,000 items with fp rate < 0.1% or 10,000 items and <0.0001%
static const unsigned int MAX_BLOOM_FILTER_SIZE = 36000; // bytes
static const unsigned int MAX_BLOOM_FILTER_HASH_FUNCS = 50;

class BloomFilter
{
private:
    bool bSet;
    uchar_vector filter;
    bool bFull, bEmpty;
    uint32_t nHashFuncs;
    uint32_t nTweak;
    uint8_t nFlags;

    uint32_t hash(uint n, const uchar_vector& data) const;

public:
    BloomFilter() : bSet(false) { }
    BloomFilter(uint32_t nElements, double falsePositiveRate, uint32_t _nTweak, uint8_t _nFlags);

    void set(uint32_t nElements, double falsePositiveRate, uint32_t _nTweak, uint8_t _nFlags);
    bool isSet() const { return bSet; }

    void clear() { filter.clear(); }

    void insert(const uchar_vector& data);
    bool match(const uchar_vector& data) const;

    const uchar_vector& getFilter() const { return filter; }
    uint32_t getNHashFuncs() const { return nHashFuncs; }
    uint32_t getNTweak() const { return nTweak; }
    uint8_t getNFlags() const { return nFlags; }
};

} // Coin

#endif // BLOOM_FILTER_H__
