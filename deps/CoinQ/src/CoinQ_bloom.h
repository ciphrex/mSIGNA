///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_bloom.h 
//
// Copyright (c) 2012-2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef _COINQ_BLOOM_H_
#define _COINQ_BLOOM_H_

#include <BloomFilter.h>

#include <encodings.h>

namespace CoinQ {

class BloomFilterTextFile : public Coin::BloomFilter
{
public:
    BloomFilterTextFile() : Coin::BloomFilter() { }

    void load(const std::string& filename, const char* base58chars = DEFAULT_BASE58_CHARS);
};

}

#endif // _COINQ_BLOOM_H_

