///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_blocks.h 
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include "CoinQ_exceptions.h"
#include "CoinQ_signals.h"
#include "CoinQ_slots.h"

#include <CoinCore/CoinNodeData.h>

#include <set>
#include <map>
#include <stack>
#include <stdexcept>
#include <fstream>

#include <assert.h>

#include <boost/filesystem.hpp>

// TODO: Create subclass for childHashes
class ChainHeader : public Coin::CoinBlockHeader
{
public:
    bool inBestChain;
    int height;
    BigInt chainWork; // total work for the chain with this header as its leaf
    std::set<uchar_vector> childHashes;

    ChainHeader() : Coin::CoinBlockHeader(), inBestChain(false), height(-1), chainWork(0) { }
    ChainHeader(const Coin::CoinBlockHeader& header, bool _inBestChain = false, int _height = -1, const BigInt& _chainWork = 0) : Coin::CoinBlockHeader(header), inBestChain(_inBestChain), height(_height), chainWork(_chainWork) { }
    ChainHeader(uint32_t _version, uint32_t _timestamp, uint32_t _bits, uint32_t _nonce = 0, const uchar_vector& _prevBlockHash = g_zero32bytes, const uchar_vector& _merkleRoot = g_zero32bytes, bool _inBestChain = false, int _height = -1, const BigInt& _chainWork = 0) : Coin::CoinBlockHeader(_version, _timestamp, _bits, _nonce, _prevBlockHash, _merkleRoot), inBestChain(_inBestChain), height(_height), chainWork(_chainWork) { }

    // TODO: add these operators for CoinClasses and compare directly instead of using hashes.
    bool operator==(const ChainHeader& rhs) const { return ((getHash() == rhs.getHash()) && (inBestChain == rhs.inBestChain) && (height == rhs.height) && (chainWork == rhs.chainWork)); }
    bool operator!=(const ChainHeader& rhs) const { return !(*this == rhs); }

    void clear() { inBestChain = false; height = -1; chainWork = 0; childHashes.clear(); }

};


class ChainBlock : public Coin::CoinBlock
{
public:
    bool inBestChain;
    int height;
    BigInt chainWork;

    ChainBlock() : Coin::CoinBlock(), inBestChain(false), height(-1), chainWork(0) { }
    ChainBlock(const Coin::CoinBlock& block, bool _inBestChain = false, int _height = -1, const BigInt& _chainWork = 0) : Coin::CoinBlock(block), inBestChain(_inBestChain), height(_height), chainWork(_chainWork) { }

    ChainHeader getHeader() const { return ChainHeader(blockHeader, inBestChain, height, chainWork); }
};

class ChainMerkleBlock : public Coin::MerkleBlock
{
public:
    bool inBestChain;
    int height;
    BigInt chainWork;

    ChainMerkleBlock() : Coin::MerkleBlock(), inBestChain(false), height(-1), chainWork(0) { }
    ChainMerkleBlock(const Coin::MerkleBlock& merkleBlock, bool _inBestChain = false, int _height = -1, const BigInt& _chainWork = 0) : Coin::MerkleBlock(merkleBlock), inBestChain(_inBestChain), height(_height), chainWork(_chainWork) { }

    ChainHeader getHeader() const { return ChainHeader(blockHeader, inBestChain, height, chainWork); }
};

typedef std::function<void(const ChainHeader&)>      chain_header_slot_t;
typedef std::function<void(const ChainBlock&)>       chain_block_slot_t;
typedef std::function<void(const ChainMerkleBlock&)> chain_merkle_block_slot_t;

class ICoinQBlockTree
{
public:
    virtual void subscribeAddBestChain(chain_header_slot_t slot) = 0;
    virtual void subscribeRemoveBestChain(chain_header_slot_t slot) = 0;
    virtual void subscribeInsert(chain_header_slot_t slot) = 0;
    virtual void subscribeDelete(chain_header_slot_t slot) = 0;

    virtual void clearAddBestChain() = 0;
    virtual void clearRemoveBestChain() = 0;
    virtual void clearInsert() = 0;
    virtual void clearDelete() = 0;
    void unsubscribeAll() { clearAddBestChain(); clearRemoveBestChain(); clearInsert(); clearDelete(); }

    // slot is passed the header of the oldest block not in the old chain
    virtual void subscribeReorg(chain_header_slot_t slot) = 0;

    virtual void setGenesisBlock(const Coin::CoinBlockHeader& header) = 0;

    virtual bool isEmpty() const = 0;

    // returns true if new header added, false if header already exists
    // throws runtime_error if header invalid or parent not known
//    virtual bool insertHeader(const Coin::CoinBlockHeader& header) = 0;
    virtual bool insertHeader(const Coin::CoinBlockHeader& header, bool bCheckProofOfWork, bool bReplaceTip) = 0;

    // returns true if header removed, false if header unknown
    virtual bool deleteHeader(const uchar_vector& hash) = 0;
 
    virtual bool hasHeader(const uchar_vector& hash) const = 0;
    virtual const ChainHeader& getHeader(const uchar_vector& hash) const = 0;
    virtual const ChainHeader& getHeader(int height) const = 0; // Use -1 to get top block
    virtual const ChainHeader& getTip() const = 0;
    virtual int getTipHeight() const = 0;
    virtual const ChainHeader& getHeaderBefore(uint32_t timestamp) const = 0;

    virtual const uchar_vector& getBestHash() const = 0;
    virtual int getBestHeight() const = 0;
    virtual BigInt getTotalWork() const = 0;

    virtual std::vector<uchar_vector> getLocatorHashes(int maxSize) const = 0;

    virtual int getConfirmations(const uchar_vector& hash) const = 0;
    virtual void clear() = 0;
};

class CoinQBlockTreeMem : public ICoinQBlockTree
{
private:
    bool bFlushed;

    typedef std::map<uchar_vector, ChainHeader> header_hash_map_t;
    header_hash_map_t mHeaderHashMap;

    typedef std::map<unsigned int, ChainHeader*> header_height_map_t;
    header_height_map_t mHeaderHeightMap;

    int mBestHeight;
    BigInt mTotalWork;

    ChainHeader* pHead;    

    bool bCheckTimestamp;
    bool bCheckProofOfWork;

    CoinQSignal<const ChainHeader&> notifyAddBestChain;
    CoinQSignal<const ChainHeader&> notifyRemoveBestChain;
    CoinQSignal<const ChainHeader&> notifyInsert;
    CoinQSignal<const ChainHeader&> notifyDelete;
    CoinQSignal<const ChainHeader&> notifyReorg;

protected:
    bool setBestChain(ChainHeader& header);
    bool unsetBestChain(ChainHeader& header);

public:
    CoinQBlockTreeMem(bool _bCheckTimestamp = true, bool _bCheckProofOfWork = true)
        : bFlushed(true), mBestHeight(-1), mTotalWork(0), pHead(NULL), bCheckTimestamp(_bCheckTimestamp), bCheckProofOfWork(_bCheckProofOfWork) { }
    CoinQBlockTreeMem(const Coin::CoinBlockHeader& header, bool _bCheckTimestamp = true, bool _bCheckProofOfWork = true)
        : bFlushed(true), mBestHeight(-1), mTotalWork(0), pHead(NULL), bCheckTimestamp(_bCheckTimestamp), bCheckProofOfWork(_bCheckProofOfWork) { setGenesisBlock(header); }

    void subscribeAddBestChain(chain_header_slot_t slot) { notifyAddBestChain.connect(slot); }
    void subscribeRemoveBestChain(chain_header_slot_t slot) { notifyRemoveBestChain.connect(slot); }
    void subscribeInsert(chain_header_slot_t slot) { notifyInsert.connect(slot); }
    void subscribeDelete(chain_header_slot_t slot) { notifyDelete.connect(slot); }
    void subscribeReorg(chain_header_slot_t slot) { notifyReorg.connect(slot); }

    void clearAddBestChain() { notifyAddBestChain.clear(); }
    void clearRemoveBestChain() { notifyRemoveBestChain.clear(); }
    void clearInsert() { notifyInsert.clear(); }
    void clearDelete() { notifyDelete.clear(); }
    void clearReorg() { notifyReorg.clear();; }

    void setGenesisBlock(const Coin::CoinBlockHeader& header);
    bool isEmpty() const { return pHead == nullptr; }
    bool insertHeader(const Coin::CoinBlockHeader& header, bool bCheckProofOfWork = true, bool bReplaceTip = false);
    bool deleteHeader(const uchar_vector& hash);

    bool hasHeader(const uchar_vector& hash) const;
    const ChainHeader& getHeader(const uchar_vector& hash) const;
    const ChainHeader& getHeader(int height) const;
    const ChainHeader& getTip() const;
    int getTipHeight() const;
    const ChainHeader& getHeaderBefore(uint32_t timestamp) const;

    const uchar_vector& getBestHash() const { return getHeader(-1).hash(); }
    int getBestHeight() const { return mBestHeight; }
    BigInt getTotalWork() const { return mTotalWork; }

    std::vector<uchar_vector> getLocatorHashes(int maxSize) const;

    int getConfirmations(const uchar_vector& hash) const;
    void clear() { mHeaderHashMap.clear(); mHeaderHeightMap.clear(); mBestHeight = -1; mTotalWork = 0; pHead = NULL; }

    typedef std::function<bool(const CoinQBlockTreeMem&)> callback_t;
    void loadFromFile(const std::string& filename, bool bCheckProofOfWork = true, callback_t callback = nullptr); 

    void flushToFile(const std::string& filename);

    bool flushed() const { return bFlushed; }
};

