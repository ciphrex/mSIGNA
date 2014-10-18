////////////////////////////////////////////////////////////////////////////////
//
// CoinNodeData.h
//
// Copyright (c) 2011-2014 Eric Lombrozo
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

#pragma once

#include "hash.h"
#include "IPv6.h"
#include "MerkleTree.h"

#include "BigInt.h"

#include <stdutils/uchar_vector.h>

#include <functional>
#include <list>
#include <queue>

#include <stdio.h>
#include <cstring>
#include <unistd.h>
#include <stdint.h>
#include <sstream>
#include <time.h>

void SetAddressVersion(unsigned char version);
void SetMultiSigAddressVersion(unsigned char version);

#define NODE_NETWORK                  1

#define MSG_ERROR                     0
#define MSG_TX                        1
#define MSG_BLOCK                     2
#define MSG_FILTERED_BLOCK            3

#define MIN_VAR_INT_SIZE              1
#define MIN_VAR_STR_SIZE              1
#define MIN_NETWORK_ADDRESS_SIZE     26
#define MIN_MESSAGE_HEADER_SIZE      24
#define MIN_VERSION_MESSAGE_SIZE     84
#define MIN_ADDR_MESSAGE_SIZE        31
#define MIN_INVENTORY_ITEM_SIZE      36
#define MIN_GET_BLOCKS_SIZE          69
#define MIN_GET_HEADERS_SIZE         65
#define MIN_OUT_POINT_SIZE           36
#define MIN_TX_IN_SIZE               41
#define MIN_TX_OUT_SIZE               9
//#define MIN_TRANSACTION_SIZE         59
#define MIN_TRANSACTION_SIZE         10 // blank transaction
#define MIN_COIN_BLOCK_HEADER_SIZE   80
#define MIN_COIN_BLOCK_SIZE         140
#define MIN_MERKLE_BLOCK_SIZE        86
#define MIN_FILTER_LOAD_SIZE         10

#define BLOOM_UPDATE_NONE             0
#define BLOOM_UPDATE_ALL              1
#define BLOOM_UPDATE P2PUBKEY_ONLY    2
#define BLOOM_UPDATE_MASK             3

const char* itemTypeToString(uint itemType);

std::string timeToString(time_t time);

std::string blankSpaces(uint n);

std::string satoshisToBtcString(uint64_t satoshis, bool trailing_zeros = true);

extern uchar_vector g_zero32bytes;

namespace Coin
{

typedef std::function<uchar_vector(const uchar_vector&)> hashfunc_t;

class CoinNodeStructure
{
public:
    CoinNodeStructure() : isHashSet_(false) { }

    virtual ~CoinNodeStructure() { }

    virtual const char* getCommand() const = 0;
    virtual uint64_t getSize() const = 0;

    virtual const uchar_vector& getHash() const;
    virtual const uchar_vector& getHashLittleEndian() const;

    virtual const uchar_vector& getHash(hashfunc_t hashfunc) const;
    virtual const uchar_vector& getHashLittleEndian(hashfunc_t hashfunc) const;

    virtual uint32_t getChecksum() const; // 4 least significant bytes, big endian

    virtual uchar_vector getSerialized() const = 0;
    virtual void setSerialized(const uchar_vector& bytes) = 0;

    virtual std::string toString() const = 0;
    virtual std::string toIndentedString(uint spaces = 0) const = 0;

protected:
    mutable uchar_vector hash_;
    mutable uchar_vector hashLittleEndian_;
    mutable bool isHashSet_;
};

class VarInt : public CoinNodeStructure
{
public:
    uint64_t value;

    VarInt() { }
    VarInt(const VarInt& rhs) { this->value = rhs.value; }
    VarInt(uint64_t value) { this->value = value; }
    VarInt(const uchar_vector& bytes) { this->setSerialized(bytes); }

    VarInt& operator=(uint64_t value) { this->value = value; return *this; }

    const char* getCommand() const { return ""; }
    uint64_t getSize() const;
    uchar_vector getSerialized() const;
    void setSerialized(const uchar_vector& bytes);

    std::string toString() const
    {
        std::stringstream ss;
        ss << value;
        return ss.str();
    }
    std::string toIndentedString(uint spaces = 0) const { return blankSpaces(spaces) + this->toString(); }
};

class VarString : public CoinNodeStructure
{
public:
    std::string value;

    VarString() { }
    VarString(const VarString& rhs) { this->value = rhs.value; }
    VarString(const std::string& value) { this->value = value; }
    VarString(const char* value) { this->value = value; }
    VarString(const uchar_vector& bytes) { this->setSerialized(bytes); }

    VarString& operator=(const char* value) { this->value = value; return *this;}
    VarString& operator=(const std::string& value) { this->value = value; return *this; }

    const char* getCommand() const { return ""; }
    uint64_t getSize() const;
    uchar_vector getSerialized() const;
    void setSerialized(const uchar_vector& bytes);

    std::string toString() const { return value; }
    std::string toIndentedString(uint spaces = 0) const { return blankSpaces(spaces) + this->value; }
};

class NetworkAddress : public CoinNodeStructure
{
public:
    uint32_t time;
    uint64_t services;
    IPv6Address ipv6;
    uint16_t port;
    bool hasTime;

    NetworkAddress() { }
    NetworkAddress(uint32_t time, uint64_t services, unsigned char ipv6_bytes[], uint16_t port);
    NetworkAddress(uint64_t services, const unsigned char ipv6_bytes[], uint16_t port);
    NetworkAddress(const NetworkAddress& netaddr);
    NetworkAddress(const uchar_vector& bytes) { this->setSerialized(bytes); }

    void set(uint64_t services, const unsigned char ipv6_bytes[], uint16_t port);
	
    const char* getCommand() const { return ""; }
    uint64_t getSize() const { return hasTime ? 30 : 26; }
    uchar_vector getSerialized() const;
    void setSerialized(const uchar_vector& bytes);

    std::string getName() const; 

    std::string toString() const;
    std::string toIndentedString(uint spaces = 0) const;

    void setTime(uint32_t time) { this->time = time; this->hasTime = true; }
};

class MessageHeader : public CoinNodeStructure
{
public:
    MessageHeader() : hasChecksum(true) { }
    MessageHeader(uint32_t magic, const char* command, uint32_t length); // no checksum
    MessageHeader(uint32_t magic, const char* command, uint32_t length, uint32_t checksum);
    MessageHeader(const MessageHeader&);
    MessageHeader(const uchar_vector& bytes) { this->setSerialized(bytes); }

    const char* getCommand() const { return ""; }
    uint64_t getSize() const { return hasChecksum ? 24 : 20; }
    uchar_vector getSerialized() const;
    void setSerialized(const uchar_vector& bytes);

    std::string toString() const;
    std::string toIndentedString(uint spaces = 0) const;

    void removeChecksum() { hasChecksum = false; }
    uint32_t magic;
    char command[12];
    uint32_t length;
    uint32_t checksum;
    bool hasChecksum;

};

// Full message: header + payload
class CoinNodeMessage : public CoinNodeStructure
{
public:
    MessageHeader header;
    CoinNodeStructure* pPayload;

public:
    CoinNodeMessage(const CoinNodeMessage& message) { this->setMessage(message.header.magic, message.pPayload); }
    CoinNodeMessage(uint32_t magic, CoinNodeStructure* pPayload) { this->setMessage(magic, pPayload); }
    CoinNodeMessage(const uchar_vector& bytes) { this->pPayload = NULL; this->setSerialized(bytes); }
    ~CoinNodeMessage();

    void setMessage(uint32_t magic, CoinNodeStructure* pPayload);

    const char* getCommand() const { return this->pPayload->getCommand(); }
    uint64_t getSize() const;
    uchar_vector getSerialized() const;
    void setSerialized(const uchar_vector& bytes);

    std::string toString() const;
    std::string toIndentedString(uint spaces = 0) const;

    bool isChecksumValid() const;

    MessageHeader getHeader() const { return header; }
    CoinNodeStructure* getPayload() const { return pPayload; }
};

class VersionMessage : public CoinNodeStructure
{
public:
    VersionMessage () { }
    VersionMessage(
        int32_t version,
        uint64_t services,
        int64_t timestamp,
        const NetworkAddress& recipientAddress,
        const NetworkAddress& senderAddress,
        uint64_t nonce,
        const char* subVersion,
        int32_t startHeight,
        bool relay = true
    );
    VersionMessage(const uchar_vector bytes) { this->setSerialized(bytes); }

    int32_t version() const { return version_; }
    uint64_t services() const { return services_; }
    int64_t timestamp() const { return timestamp_; }
    const NetworkAddress& recipientAddress() const { return recipientAddress_; }
    const NetworkAddress& senderAddress() const { return senderAddress_; }
    uint64_t nonce() const { return nonce_; }
    const VarString& subVersion() const { return subVersion_; }
    int32_t startHeight() const { return startHeight_; }
    bool relay() const { return relay_; } 

    const char* getCommand() const { return "version"; }
    uint64_t getSize() const;
    uchar_vector getSerialized() const;
    void setSerialized(const uchar_vector& bytes);

    std::string toString() const;
    std::string toIndentedString(uint spaces = 0) const;

private:
    int32_t version_;
    uint64_t services_;
    int64_t timestamp_;
    NetworkAddress recipientAddress_;
    NetworkAddress senderAddress_;
    uint64_t nonce_;
    VarString subVersion_;
    int32_t startHeight_;
    bool relay_;
};

class BlankMessage : public CoinNodeStructure
{
public:
    std::string command;

    BlankMessage(const std::string& _command) : command(_command) { }

    const char* getCommand() const { return this->command.c_str(); }
    uint64_t getSize() const { return 0; }

    uchar_vector getSerialized() const { uchar_vector rval; return rval; }
    void setSerialized(const uchar_vector& /*bytes*/) { }

    std::string toString() const { return ""; }
    std::string toIndentedString(uint spaces = 0) const { return blankSpaces(spaces); }
};

class VerackMessage : public CoinNodeStructure
{
public:
    VerackMessage() { }

    const char* getCommand() const { return "verack"; }
    uint64_t getSize() const { return 0; }

    uchar_vector getSerialized() const { uchar_vector rval; return rval; }
    void setSerialized(const uchar_vector& /*bytes*/) { }

    std::string toString() const { return ""; }
    std::string toIndentedString(uint spaces = 0) const { return blankSpaces(spaces); }
};

class AddrMessage : public CoinNodeStructure
{
public:
    std::vector<NetworkAddress> addrList;

    AddrMessage(const std::vector<NetworkAddress> addrList) { this->addrList = addrList; }
    AddrMessage(const uchar_vector& bytes) { this->setSerialized(bytes); }

    const char* getCommand() const { return "addr"; }
    uint64_t getSize() const { return VarInt(this->addrList.size()).getSize() + 30*this->addrList.size(); }

    uchar_vector getSerialized() const;
    void setSerialized(const uchar_vector& bytes);

    std::string toString() const;
    std::string toIndentedString(uint spaces = 0) const;
};

class InventoryItem : public CoinNodeStructure
{
public:
    uint32_t itemType;
    unsigned char hash[32];

    InventoryItem() { }
    InventoryItem(const uchar_vector& bytes) { this->setSerialized(bytes); }
    InventoryItem(const InventoryItem& item)
    {
        this->itemType = item.itemType;
        memcpy(this->hash, item.hash, 32);
    }
    InventoryItem(uint32_t itemType, const unsigned char* hash)
    {
        this->itemType = itemType;
        memcpy(this->hash, hash, 32);
    }
    InventoryItem(uint32_t itemType, const uchar_vector& hashBytes)
    {
        this->itemType = itemType;
        memcpy(this->hash, (unsigned char*)&hashBytes[0], 32);
    }
    InventoryItem(uint32_t itemType, const std::string& hashHex)
    {
        this->itemType = itemType;
        uchar_vector hashBytes;
        hashBytes.setHex(hashHex);
        hashBytes.padRight(0, 32);
        hashBytes.reverse();
        memcpy(this->hash, &hashBytes[0], 32);
    }

    const char* getCommand() const { return ""; }
    uint64_t getSize() const { return 36; }
    uchar_vector getSerialized() const;
    void setSerialized(const uchar_vector& bytes);

    std::string toString() const;
    std::string toIndentedString(uint spaces = 0) const;

    uint getType() const { return itemType; }
    uchar_vector getItemHash() const { return uchar_vector(hash, 32); }
};

class Inventory : public CoinNodeStructure
{
public:
    std::vector<InventoryItem> items;

    Inventory() { }
    Inventory(const std::vector<InventoryItem> items) { this->items = items; }
    Inventory(const uchar_vector& bytes) { this->setSerialized(bytes); }
    Inventory(const Inventory& inv) { this->items = inv.getItems(); }

    void addItem(const InventoryItem& item) { (this->items).push_back(item); }
    void addItem(uint itemType, const uchar_vector& hashBytes) { this->addItem(InventoryItem(itemType, hashBytes)); }

    std::vector<InventoryItem> getItems() const { return items; }
    uint getCount() const { return (this->items).size(); }

    const char* getCommand() const { return "inv"; }
    uint64_t getSize() const { return VarInt(this->items.size()).getSize() + 36*this->items.size(); }
    uchar_vector getSerialized() const;
    void setSerialized(const uchar_vector& bytes);

    std::string toString() const;
    std::string toIndentedString(uint spaces = 0) const;
};

class GetDataMessage : public Inventory
{
public:
    GetDataMessage() { }
    GetDataMessage(const std::vector<InventoryItem> items) { this->items = items; }
    GetDataMessage(const uchar_vector& bytes) { this->setSerialized(bytes); }
    GetDataMessage(const Inventory& inv) { this->items = inv.getItems(); }

    const char* getCommand() const { return "getdata"; }

    void toFilteredBlocks() { for (auto& item: this->items) { if (item.itemType == MSG_BLOCK) item.itemType = MSG_FILTERED_BLOCK; } }
};

class NotFoundMessage : public Inventory
{
public:
    NotFoundMessage() { }
    NotFoundMessage(const std::vector<InventoryItem> items) { this->items = items; }
    NotFoundMessage(const uchar_vector& bytes) { this->setSerialized(bytes); }
    NotFoundMessage(const Inventory& inv) { this->items = inv.getItems(); }

    const char* getCommand() const { return "getdata"; }
};

/*
class GetDataMessage : public CoinNodeStructure
{
public:
    std::vector<InventoryItem> items;

    GetDataMessage() { }
    GetDataMessage(const std::vector<InventoryItem> items) { this->items = items; }
    GetDataMessage(const uchar_vector& bytes) { this->setSerialized(bytes); }
    GetDataMessage(const Inventory& inv) { this->items = inv.getItems(); }

    void addItem(const InventoryItem& item) { (this->items).push_back(item); }
    void addItem(uint itemType, const uchar_vector& hashBytes) { this->addItem(InventoryItem(itemType, hashBytes)); }

    std::vector<InventoryItem> getItems() const { return items; }
    uint getCount() const { return (this->items).size(); }

    const char* getCommand() const { return "getdata"; }
    uint64_t getSize() const { return VarInt(this->items.size()).getSize() + 36*this->items.size(); }
    uchar_vector getSerialized() const;
    void setSerialized(const uchar_vector& bytes);

    std::string toString() const;
    std::string toIndentedString(uint spaces = 0) const;
};
*/
class GetBlocksMessage : public CoinNodeStructure
{
public:
    uint32_t version;
    std::vector<uchar_vector> blockLocatorHashes;
    uchar_vector hashStop;

    GetBlocksMessage() { this->hashStop = g_zero32bytes; }
    GetBlocksMessage(const GetBlocksMessage& getBlocksMessage)
    {
        this->version = getBlocksMessage.version;
        this->blockLocatorHashes = getBlocksMessage.blockLocatorHashes;
        this->hashStop = getBlocksMessage.hashStop;
    }
    GetBlocksMessage(const uchar_vector& bytes) { this->setSerialized(bytes); }
    GetBlocksMessage(uint32_t version, const std::vector<uchar_vector>& blockLocatorHashes, const uchar_vector& hashStop = g_zero32bytes)
    {
        this->version = version;
        this->blockLocatorHashes = blockLocatorHashes;
        this->hashStop = hashStop;
    }
    GetBlocksMessage(const std::string& hex);

    const char* getCommand() const { return "getblocks"; }
    uint64_t getSize() const { return VarInt(this->blockLocatorHashes.size()).getSize() + 32*this->blockLocatorHashes.size() + 36; }
    uchar_vector getSerialized() const;
    void setSerialized(const uchar_vector& bytes);

    std::string toString() const;
    std::string toIndentedString(uint spaces = 0) const;

    void addHashStart(const uchar_vector& hashStart) { this->blockLocatorHashes.push_back(hashStart); }
    void setHashStop(const uchar_vector& hashStop) { this->hashStop = hashStop; }
};

class GetHeadersMessage : public CoinNodeStructure
{
public:
    uint32_t version;
    std::vector<uchar_vector> blockLocatorHashes;
    uchar_vector hashStop;

    GetHeadersMessage() { this->hashStop = g_zero32bytes; }
    GetHeadersMessage(const GetHeadersMessage& getHeadersMessage)
    {
        this->version = getHeadersMessage.version;
        this->blockLocatorHashes = getHeadersMessage.blockLocatorHashes;
        this->hashStop = getHeadersMessage.hashStop;
    }
    GetHeadersMessage(const uchar_vector& bytes) { this->setSerialized(bytes); }
    GetHeadersMessage(uint32_t version, const std::vector<uchar_vector>& blockLocatorHashes, const uchar_vector& hashStop = g_zero32bytes)
    {
        this->version = version;
        this->blockLocatorHashes = blockLocatorHashes;
        this->hashStop = hashStop;
    }

    const char* getCommand() const { return "getheaders"; }
    uint64_t getSize() const { return VarInt(this->blockLocatorHashes.size()).getSize() + 32*this->blockLocatorHashes.size() + 36; }
    uchar_vector getSerialized() const;
    void setSerialized(const uchar_vector& bytes);

    std::string toString() const;
    std::string toIndentedString(uint spaces = 0) const;

    void addHashStart(const uchar_vector& hashStart) { this->blockLocatorHashes.push_back(hashStart); }
};

class OutPoint : public CoinNodeStructure
{
public:
    unsigned char hash[32];
    uint32_t index;

    OutPoint() { }
    OutPoint(const uchar_vector& hashBytes, uint index) { this->setPoint(hashBytes, index); }
    OutPoint(const std::string& hashHex, uint index) { uchar_vector hashBytes; hashBytes.setHex(hashHex); this->setPoint(hashBytes, index); }
    OutPoint(const uchar_vector& bytes) { this->setSerialized(bytes); }
    OutPoint(const OutPoint& outPoint)
    {
        memcpy(this->hash, outPoint.hash, 32);
        this->index = outPoint.index;
    }

    void setPoint(const uchar_vector& hashBytes, uint index);

    const char* getCommand() const { return ""; }
    uint64_t getSize() const { return 36; }
    uchar_vector getSerialized() const;
    void setSerialized(const uchar_vector& bytes);

    std::string getTxHash() const { return uchar_vector(this->hash, 32).getHex(); }
	
    std::string toDelimited(const std::string& delimiter) const;
    std::string toString() const;
    std::string toIndentedString(uint spaces = 0) const;
};

class TxIn : public CoinNodeStructure
{
public:
    OutPoint previousOut;
    uchar_vector scriptSig;
    uint32_t sequence;

    TxIn() { }
    TxIn(const TxIn& txIn)
        : previousOut(txIn.previousOut), scriptSig(txIn.scriptSig), sequence(txIn.sequence) { }
    TxIn(const OutPoint& _previousOut, const uchar_vector& _scriptSig, uint32_t _sequence)
        : previousOut(_previousOut), scriptSig(_scriptSig), sequence(_sequence) { }
    TxIn(const OutPoint& previousOut, const std::string& scriptSigHex, uint32_t sequence);
    TxIn(const uchar_vector& bytes) { this->setSerialized(bytes); }

    const char* getCommand() const { return ""; }
    uint64_t getSize() const { return VarInt(this->scriptSig.size()).getSize() + scriptSig.size() + 40; } // 40 = previousOut + sequence
    uchar_vector getSerialized() const { return this->getSerialized(true); }
    uchar_vector getSerialized(bool includeScriptSigLength) const;
    void setSerialized(const uchar_vector& bytes);

    uchar_vector getOutpointHash() const { return uchar_vector(this->previousOut.hash, 32); }
    uint32_t getOutpointIndex() const { return this->previousOut.index; }
    std::string getAddress() const;
    std::string toString() const;
    std::string toIndentedString(uint spaces = 0) const;
    std::string toJson() const;
};

class TxOut : public CoinNodeStructure
{
public:
    uint64_t value;
    uchar_vector scriptPubKey;

    TxOut() { }
    TxOut(const TxOut& txOut)
        : value(txOut.value), scriptPubKey(txOut.scriptPubKey) { }
    TxOut(uint64_t _value, const uchar_vector& _scriptPubKey)
        : value(_value), scriptPubKey(_scriptPubKey) { }
    TxOut(uint64_t value, const std::string& scriptPubKeyHex);
    TxOut(const uchar_vector& bytes) { this->setSerialized(bytes); }

    const char* getCommand() const { return ""; }
    uint64_t getSize() const { return VarInt(this->scriptPubKey.size()).getSize() + scriptPubKey.size() + 8; } // 8 = sizeof(value)
    uchar_vector getSerialized() const;
    void setSerialized(const uchar_vector& bytes);

    std::string getAddress() const;
    std::string toString() const;
    std::string toIndentedString(uint spaces = 0) const;
    std::string toJson() const;
};

class Transaction : public CoinNodeStructure
{
public:
    uint32_t version;
    std::vector<TxIn> inputs;
    std::vector<TxOut> outputs;
    uint32_t lockTime;

    Transaction() { this->version = 1; lockTime = 0; }
    Transaction(const uchar_vector& bytes) { this->setSerialized(bytes); }
    Transaction(const std::string& hex);
    Transaction(const Transaction& tx)
        : version(tx.version), inputs(tx.inputs), outputs(tx.outputs), lockTime(tx.lockTime) { }

    const uchar_vector& hash() const { return getHashLittleEndian(); }

    const char* getCommand() const { return "tx"; }
    uint64_t getSize() const;
    uchar_vector getSerialized() const { return this->getSerialized(true); }
    uchar_vector getSerialized(bool includeScriptSigLength) const;
    void setSerialized(const uchar_vector& bytes);

    std::string toString() const;
    std::string toIndentedString(uint spaces = 0) const;
    std::string toJson() const;

    void clearScriptSigs();
    void setScriptSig(uint index, const uchar_vector& scriptSig);
    void setScriptSig(uint index, const std::string& scriptSigHex);

    void clearInputs() { inputs.clear(); }
    void clearOutputs() { outputs.clear(); }

    void addInput(const TxIn& txin) { inputs.push_back(txin); }
    void addOutput(const TxOut& txout) { outputs.push_back(txout); }
	
    uint64_t getTotalSent() const;

    uchar_vector getHashWithAppendedCode(uint32_t code) const; // in little endian
};

class CoinBlock;
class MerkleBlock;

class CoinBlockHeader : public CoinNodeStructure
{
public:
    CoinBlockHeader() : isPOWHashSet_(false) { }
    CoinBlockHeader(uint32_t version, const uchar_vector& prevBlockHash, const uchar_vector& merkleRoot, uint32_t timestamp, uint32_t bits, uint32_t nonce)
        : isPOWHashSet_(false), version_(version), prevBlockHash_(prevBlockHash), merkleRoot_(merkleRoot), timestamp_(timestamp), bits_(bits), nonce_(nonce) { }
    CoinBlockHeader(uint32_t version, uint32_t timestamp, uint32_t bits, uint32_t nonce = 0, const uchar_vector& prevBlockHash = g_zero32bytes, const uchar_vector& merkleRoot = g_zero32bytes)
        : isPOWHashSet_(false), version_(version), prevBlockHash_(prevBlockHash), merkleRoot_(merkleRoot), timestamp_(timestamp), bits_(bits), nonce_(nonce) { }
    CoinBlockHeader(const uchar_vector& bytes) { setSerialized(bytes); }
    CoinBlockHeader(const std::string& hex);

    void set(uint32_t version, uint32_t timestamp, uint32_t bits, uint32_t nonce = 0, const uchar_vector& prevBlockHash = g_zero32bytes, const uchar_vector& merkleRoot = g_zero32bytes)
    {
        isHashSet_ = false;
        isPOWHashSet_ = false;

        version_ = version;
        prevBlockHash_ = prevBlockHash;
        merkleRoot_ = merkleRoot;
        timestamp_ = timestamp;
        bits_ = bits;
        nonce_ = nonce;
    }

    const uchar_vector& hash() const { return getHashLittleEndian(); }
    uint32_t version() const { return version_; }
    const uchar_vector&  prevBlockHash() const { return prevBlockHash_; }
    const uchar_vector& merkleRoot() const { return merkleRoot_; }
    uint32_t timestamp() const { return timestamp_; }
    uint32_t bits() const { return bits_; }
    uint32_t nonce() const { return nonce_; }

    const char* getCommand() const { return ""; }
    uint64_t getSize() const { return 80; }
    uchar_vector getSerialized() const;
    void setSerialized(const uchar_vector& bytes);

    std::string toString() const;
    std::string toIndentedString(uint spaces = 0) const;

    void nonce(uint32_t nonce) { nonce_ = nonce; isHashSet_ = false; isPOWHashSet_ = false; }
    void incrementNonce() { nonce_++; isHashSet_ = false; isPOWHashSet_ = false; }

    const BigInt getTarget() const;
    void setTarget(const BigInt& target);

    const BigInt getWork() const;

    static void setHashFunc(hashfunc_t hashfunc) { hashfunc_ = hashfunc; }
    static void setPOWHashFunc(hashfunc_t hashfunc) { powhashfunc_ = hashfunc; }

    const uchar_vector& getHash() const;
    const uchar_vector& getHashLittleEndian() const;

    const uchar_vector& getPOWHash() const;
    const uchar_vector& getPOWHashLittleEndian() const;

private:
    friend class CoinBlock;
    friend class MerkleBlock;

    static hashfunc_t hashfunc_;
    static hashfunc_t powhashfunc_;

/*
    // Inherited from CoinNodeStructure
    mutable uchar_vector hash_;
    mutable uchar_vector hashLittleEndian_;
    mutable bool isHashSet_;
*/

    mutable uchar_vector POWHash_;
    mutable uchar_vector POWHashLittleEndian_;
    mutable bool isPOWHashSet_;

    void resetHash() const { isHashSet_ = false; isPOWHashSet_ = false; }

    uint32_t version_;
    uchar_vector prevBlockHash_;
    uchar_vector merkleRoot_;
    uint32_t timestamp_;
    uint32_t bits_;
    uint32_t nonce_;
};

class CoinBlock : public CoinNodeStructure
{
public:
    CoinBlockHeader blockHeader;
    std::vector<Transaction> txs;

    CoinBlock() { }
    CoinBlock(const CoinBlockHeader& _blockHeader, const std::vector<Transaction>& _txs)
        : blockHeader(_blockHeader), txs(_txs) { }
    CoinBlock(const CoinBlock& coinBlock)
        : blockHeader(coinBlock.blockHeader), txs(coinBlock.txs) { }
    CoinBlock(uint32_t version, uint32_t timestamp, uint32_t bits, const uchar_vector& prevBlockHash = g_zero32bytes)
    {
        this->blockHeader = CoinBlockHeader(version, timestamp, bits, 0, prevBlockHash);
    }
    CoinBlock(const uchar_vector& bytes) { this->setSerialized(bytes); }
    CoinBlock(const std::string& hex);

    const uchar_vector& hash() const { return blockHeader.getHashLittleEndian(); }
    uint32_t version() const { return blockHeader.version(); }
    const uchar_vector& prevBlockHash() const { return blockHeader.prevBlockHash(); }
    const uchar_vector& merkleRoot() const { return blockHeader.merkleRoot(); }
    uint32_t timestamp() const { return blockHeader.timestamp(); }
    uint32_t bits() const { return blockHeader.bits(); }
    uint32_t nonce() const { return blockHeader.nonce(); } 

    const BigInt getTarget() const { return blockHeader.getTarget(); }
    const BigInt getWork() const { return blockHeader.getWork(); }
    
    const char* getCommand() const { return "block"; }
    uint64_t getSize() const;
    uchar_vector getSerialized() const;
    void setSerialized(const uchar_vector& bytes);

    std::string toString() const;
    std::string toIndentedString(uint spaces = 0) const;
    std::string toRedactedIndentedString(uint spaces = 0) const;

    void addTransaction(const Transaction& tx) { this->txs.push_back(tx); }

    bool isValidMerkleRoot() const;
    void updateMerkleRoot();

    void incrementNonce() { blockHeader.incrementNonce(); }
	
    uint64_t getTotalSent() const;

    // Only supported for blocks version 2 or higher.
    // Returns -1 for older blocks.
    int64_t getHeight() const;
};

class MerkleBlock : public CoinNodeStructure
{
public:
    CoinBlockHeader blockHeader;
    uint32_t nTxs;
    std::vector<uchar_vector> hashes;
    uchar_vector flags;

    MerkleBlock() { }
    MerkleBlock(const CoinBlockHeader& _blockHeader, uint32_t _nTxs, const std::vector<uchar_vector>& _hashes, const uchar_vector& _flags)
        : blockHeader(_blockHeader), nTxs(_nTxs), hashes(_hashes), flags(_flags) { }
    MerkleBlock(const MerkleBlock& merkleBlock)
        : blockHeader(merkleBlock.blockHeader), nTxs(merkleBlock.nTxs), hashes(merkleBlock.hashes), flags(merkleBlock.flags) { }
    MerkleBlock(const PartialMerkleTree& merkleTree, uint32_t version, const uchar_vector& prevBlockHash, uint32_t timestamp, uint32_t bits, uint32_t nonce);
    explicit MerkleBlock(const uchar_vector& bytes) { setSerialized(bytes); }

    const uchar_vector& hash() const { return blockHeader.getHashLittleEndian(); }
    uint32_t version() const { return blockHeader.version(); }
    const uchar_vector& prevBlockHash() const { return blockHeader.prevBlockHash(); }
    const uchar_vector& merkleRoot() const { return blockHeader.merkleRoot(); }
    uint32_t timestamp() const { return blockHeader.timestamp(); }
    uint32_t bits() const { return blockHeader.bits(); }
    uint32_t nonce() const { return blockHeader.nonce(); } 

    PartialMerkleTree merkleTree() const;

    const BigInt getTarget() const { return blockHeader.getTarget(); }
    const BigInt getWork() const { return blockHeader.getWork(); }

    const char* getCommand() const { return "merkleblock"; }
    uint64_t getSize() const;
    uchar_vector getSerialized() const;
    void setSerialized(const uchar_vector& bytes);

    std::string toString() const;
    std::string toIndentedString(uint spaces = 0) const;

    bool isValidMerkleRoot() const;
    void updateMerkleRoot();
};

class HeadersMessage : public CoinNodeStructure
{
public:
    std::vector<CoinBlockHeader> headers;

    HeadersMessage() { }
    HeadersMessage(const std::vector<CoinBlockHeader>& headers) { this->headers = headers; }
    HeadersMessage(const uchar_vector& bytes) { this->setSerialized(bytes); }
    HeadersMessage(const std::string& hex);

    const char* getCommand() const { return "headers"; }
    uint64_t getSize() const;
    uchar_vector getSerialized() const;
    void setSerialized(const uchar_vector& bytes);

    std::string toString() const;
    std::string toIndentedString(uint spaces = 0) const;

    void clear() { this->headers.clear(); }
    void addHeader(const CoinBlockHeader& header) { this->headers.push_back(header); }
};

class GetAddrMessage : public CoinNodeStructure
{
public:
    GetAddrMessage() { }

    const char* getCommand() const { return "getaddr"; }
    uint64_t getSize() const { return 0; }

    uchar_vector getSerialized() const { uchar_vector rval; return rval; }
    void setSerialized(const uchar_vector& /*bytes*/) { }

    std::string toString() const { return ""; }
    std::string toIndentedString(uint spaces = 0) const { return blankSpaces(spaces); }
};

class FilterLoadMessage : public CoinNodeStructure
{
public:
    uchar_vector filter;
    uint32_t nHashFuncs;
    uint32_t nTweak;
    uint8_t nFlags;

    FilterLoadMessage(uint32_t nHashFuncs_ = 0, uint32_t nTweak_ = 0, uint8_t nFlags_ = 0, const uchar_vector& filter_ = uchar_vector())
        : filter(filter_), nHashFuncs(nHashFuncs_), nTweak(nTweak_), nFlags(nFlags_) { }
    FilterLoadMessage(const uchar_vector& bytes) { setSerialized(bytes); }

    const char* getCommand() const { return "filterload"; }
    uint64_t getSize() const;

    uchar_vector getSerialized() const;
    void setSerialized(const uchar_vector& bytes);

    std::string toString() const;
    std::string toIndentedString(uint spaces = 0) const;
};

class FilterAddMessage : public CoinNodeStructure
{
public:
    uchar_vector data;

    FilterAddMessage() { }
    FilterAddMessage(const uchar_vector& bytes) { setSerialized(bytes); }

    const char* getCommand() const { return "filteradd"; }
    uint64_t getSize() const { return VarInt(data.size()).getSize() + data.size(); }

    uchar_vector getSerialized() const { return VarInt(data.size()).getSerialized() + data; }
    void setSerialized(const uchar_vector& bytes);

    std::string toString() const;
    std::string toIndentedString(uint spaces = 0) const;
};

class FilterClearMessage : public BlankMessage
{
public:
    FilterClearMessage() : BlankMessage("filterclear") { }
};

class PingMessage : public CoinNodeStructure
{
public:
    uint64_t nonce;

    PingMessage();
    PingMessage(const uchar_vector& bytes) { setSerialized(bytes); }

    const char* getCommand() const { return "ping"; }
    uint64_t getSize() const { return sizeof(uint64_t); }

    uchar_vector getSerialized() const;
    void setSerialized(const uchar_vector& bytes);

    std::string toString() const;
    std::string toIndentedString(uint spaces = 0) const;
};

class PongMessage : public CoinNodeStructure
{
public:
    uint64_t nonce;

    PongMessage(uint64_t nonce_) : nonce(nonce_) { }
    PongMessage(const uchar_vector& bytes) { setSerialized(bytes); }

    const char* getCommand() const { return "pong"; }
    uint64_t getSize() const { return sizeof(uint64_t); }

    uchar_vector getSerialized() const;
    void setSerialized(const uchar_vector& bytes);

    std::string toString() const;
    std::string toIndentedString(uint spaces = 0) const;
};

} // namespace Coin

