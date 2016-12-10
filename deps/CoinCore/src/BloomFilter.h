////////////////////////////////////////////////////////////////////////////////
//
// BloomFilter.h
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

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
