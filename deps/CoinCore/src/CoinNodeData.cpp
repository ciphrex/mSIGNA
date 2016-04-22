////////////////////////////////////////////////////////////////////////////////
//
// CoinNodeData.cpp
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

#include "CoinNodeData.h"
#include "numericdata.h"
#include "Base58Check.h"
#include "MerkleTree.h"

// For regular expressions, we can use boost or pcre
#ifndef COIN_USE_PCRE
#include <boost/regex.hpp>
#else
#include <pcre.h>
#endif

#include <iomanip>
#include <algorithm>

#include <assert.h>

using namespace Coin;
using namespace std;

uchar_vector g_zero32bytes("0000000000000000000000000000000000000000000000000000000000000000");

// Globals
unsigned char g_addressVersion = 0x00;
unsigned char g_multiSigAddressVersion = 0x05;

void SetAddressVersion(unsigned char version)
{
    g_addressVersion = version;
}

void SetMultiSigAddressVersion(unsigned char version)
{
    g_multiSigAddressVersion = version;
}

const char* itemTypeToString(uint itemType)
{
    switch (itemType) {
    case 0:
        return "MSG_ERROR";
    case 1:
        return "MSG_TX";
    case 2:
        return "MSG_BLOCK";
    }
    return "UNKNOWN";
}

string timeToString(time_t time)
{
    string rval = asctime(gmtime(&time));
    return rval.substr(0, rval.size()-1);
}

string blankSpaces(uint n)
{
    stringstream ss;
    for (uint i = 0; i < n; i++) ss << " ";
    return ss.str();
}

string satoshisToBtcString(uint64_t satoshis, bool trailing_zeros)
{
    uint64_t wholePart = satoshis / 100000000ull;
    uint64_t fractionalPart = satoshis % 100000000ull;
    uint fractionalLength = 8;
    if (!trailing_zeros) {
        if (fractionalPart == 0) {
            fractionalLength = 1;
        }
        else while (fractionalPart % 10 == 0) {
            fractionalPart /= 10;
            fractionalLength--;
        }
    }
    stringstream ss;
    ss << dec << wholePart << "." << setw(fractionalLength) << setfill('0') << fractionalPart;
    return ss.str();
}

///////////////////////////////////////////////////////////////////////////////
//
// class CoinNodeStructure implementation
//
const uchar_vector& CoinNodeStructure::getHash() const
{
    hash_ = sha256_2(getSerialized());
    return hash_;
}

const uchar_vector& CoinNodeStructure::getHashLittleEndian() const
{
    hashLittleEndian_ = sha256_2(getSerialized()).getReverse();
    return hashLittleEndian_;
}

const uchar_vector& CoinNodeStructure::getHash(hashfunc_t hashfunc) const
{
    hash_ = hashfunc(getSerialized());
    return hash_;
}

const uchar_vector& CoinNodeStructure::getHashLittleEndian(hashfunc_t hashfunc) const
{
    hashLittleEndian_ = hash_.getReverse();
    return hashLittleEndian_;
}

uint32_t CoinNodeStructure::getChecksum() const
{
    getHash();
    return vch_to_uint<uint32_t>(uchar_vector(hash_.begin(), hash_.begin() + 4), LITTLE_ENDIAN_);
}

///////////////////////////////////////////////////////////////////////////////
//
// class VarInt implementation
//
uint64_t VarInt::getSize() const
{
    if (this->value < 0xfd) return 1;
    if (this->value <= 0xffff) return 3;
    if (this->value <= 0xffffffff) return 5;
    return 9;
}

uchar_vector VarInt::getSerialized() const
{
    uchar_vector rval;
    if (this->value < 0xfd) {
        rval.push_back(value);
        return rval;
    }
    if (this->value <= 0xffff) {
        rval.push_back(0xfd);
        rval += uint_to_vch<uint16_t>(this->value, LITTLE_ENDIAN_);
        return rval;
    }
    if (this->value <= 0xffffffff) {
        rval.push_back(0xfe);
        rval += uint_to_vch<uint32_t>(this->value, LITTLE_ENDIAN_);
        return rval;
    }
    rval.push_back(0xff);
    rval += uint_to_vch<uint64_t>(this->value, LITTLE_ENDIAN_);
    return rval;
}

void VarInt::setSerialized(const uchar_vector& bytes)
{
    if (bytes.size() < MIN_VAR_INT_SIZE)
        throw runtime_error("Invalid data - VarInt too small.");

    if (bytes[0] < 0xfd)
        this->value = bytes[0];
    else if ((bytes[0] == 0xfd) && (bytes.size() >= 3))
        this->value = vch_to_uint<uint16_t>(uchar_vector(bytes.begin() + 1, bytes.begin() + 3), LITTLE_ENDIAN_);
    else if ((bytes[0] == 0xfe) && (bytes.size() >= 5))
        this->value = vch_to_uint<uint32_t>(uchar_vector(bytes.begin() + 1, bytes.begin() + 5), LITTLE_ENDIAN_);
    else if (bytes.size() >= 9)
        this->value = vch_to_uint<uint64_t>(uchar_vector(bytes.begin() + 1, bytes.begin() + 9), LITTLE_ENDIAN_);
    else
        throw runtime_error("Invalid data - VarInt length is wrong.");
}

///////////////////////////////////////////////////////////////////////////////
//
// class VarString implementation
//
uint64_t VarString::getSize() const
{
    VarInt length(this->value.size());
    return length.getSize() + this->value.size();
}

uchar_vector VarString::getSerialized() const
{
    uchar_vector rval;
    VarInt length(this->value.size());
    rval += length.getSerialized();
    rval += uchar_vector(this->value.begin(), this->value.end());
    return rval;
}

void VarString::setSerialized(const uchar_vector& bytes)
{
    VarInt length(bytes);
    if (bytes.size() < length.getSize() + length.value)
        throw runtime_error("Invalid data - VarString too small.");

    value = string(bytes.begin() + length.getSize(), bytes.begin() + length.getSize() + length.value);
}

///////////////////////////////////////////////////////////////////////////////
//
// class NetworkAddress implementation
//
NetworkAddress::NetworkAddress(uint32_t time, uint64_t services, unsigned char ipv6_bytes[], uint16_t port)
{
    this->time = time;
    this->services = services;
    this->ipv6 = ipv6_bytes;
    this->port = port;
    this->hasTime = true;
}

NetworkAddress::NetworkAddress(uint64_t services, const unsigned char ipv6_bytes[], uint16_t port)
{
    this->services = services;
    this->ipv6 = ipv6_bytes;
    this->port = port;
    this->hasTime = false;
}

NetworkAddress::NetworkAddress(const NetworkAddress& netaddr)
{
    this->time = netaddr.time;
    this->hasTime = netaddr.hasTime;
    this->services = netaddr.services;
    this->ipv6 = netaddr.ipv6;
    this->port = netaddr.port;
}

uchar_vector NetworkAddress::getSerialized() const
{
    uchar_vector rval;
    if (this->hasTime)
        rval += uint_to_vch(this->time, LITTLE_ENDIAN_);
    rval += uint_to_vch(this->services, LITTLE_ENDIAN_);
    uchar_vector ipv6vch(this->ipv6.getBytes(), 16);
    rval += ipv6vch;
    rval += uint_to_vch(this->port, BIG_ENDIAN_);
    return rval;
}

void NetworkAddress::set(uint64_t services, const unsigned char ipv6_bytes[], uint16_t port)
{
    this->services = services;
    this->ipv6 = ipv6_bytes;
    this->port = port;
    this->hasTime = false;
}

void NetworkAddress::setSerialized(const uchar_vector& bytes)
{
    if (bytes.size() < MIN_NETWORK_ADDRESS_SIZE)
        throw runtime_error("Invalid data - NetworkAddress too small.");

    uint pos = 0;
    this->hasTime = (bytes.size() >= 30);
    if (this->hasTime) {
        this->time = vch_to_uint<uint32_t>(bytes, LITTLE_ENDIAN_);
        pos += sizeof(uint32_t);
    }
    this->services = vch_to_uint<uint64_t>(uchar_vector(bytes.begin() + pos, bytes.begin() + pos + sizeof(uint64_t)), LITTLE_ENDIAN_);
    pos += sizeof(uint64_t);
    unsigned char ipv6_bytes[16];
    uchar_vector(bytes.begin() + pos, bytes.begin() + pos + 16).copyToArray(ipv6_bytes);
    this->ipv6 = ipv6_bytes;
    pos += 16;
    this->port = vch_to_uint<uint16_t>(uchar_vector(bytes.begin() + pos, bytes.begin() + pos + sizeof(uint16_t)), BIG_ENDIAN_);
}

string NetworkAddress::getName() const
{
    stringstream ss;
    ss << this->ipv6.toStringAuto() << ":" << this->port;
    return ss.str();
}

string NetworkAddress::toString() const
{
    stringstream ss;
    if (this->hasTime)
        ss << "time: " << timeToString(this->time) << ", ";
        ss << "services: " << this->services << ", ip: " << this->ipv6.toStringAuto() << ", port: " << this->port;
    return ss.str();
}

string NetworkAddress::toIndentedString(uint spaces) const
{
    stringstream ss;
    if (this->hasTime)
        ss << blankSpaces(spaces) << "time: " << timeToString(this->time) << endl;
    ss << blankSpaces(spaces) << "services: " << this->services << endl
       << blankSpaces(spaces) << "ip: " << this->ipv6.toStringAuto() << endl
       << blankSpaces(spaces) << "port: " << this->port;
    return ss.str();
}

///////////////////////////////////////////////////////////////////////////////
//
// class MessageHeader implementation
//
MessageHeader::MessageHeader(uint32_t magic, const char* command, uint32_t length)
{
    this->magic = magic;
    memset(this->command, 0, 12);
    memcpy(this->command, command, strlen(command));
    this->length = length;
    this->hasChecksum = false;
}

MessageHeader::MessageHeader(uint32_t magic, const char* command, uint32_t length, uint32_t checksum)
{
    this->magic = magic;
    memset(this->command, 0, 12);
    memcpy(this->command, command, strlen(command));
    this->length = length;
    this->checksum = checksum;
    this->hasChecksum = true;
}

MessageHeader::MessageHeader(const MessageHeader& header)
{
    this->magic = header.magic;
    memcpy(this->command, header.command, 12);
    this->length = header.length;
    this->hasChecksum = header.hasChecksum;
    this->checksum = header.checksum; // will be ignored if hasChecksum == false
}

uchar_vector MessageHeader::getSerialized() const
{
    uchar_vector rval = uint_to_vch(this->magic, LITTLE_ENDIAN_);
    rval += uchar_vector((unsigned char*)this->command, 12);
    rval += uint_to_vch(this->length, LITTLE_ENDIAN_);
    if (this->hasChecksum)
        rval += uint_to_vch(this->checksum, LITTLE_ENDIAN_);
    return rval;
}

void MessageHeader::setSerialized(const uchar_vector& bytes)
{
    if (bytes.size() < MIN_MESSAGE_HEADER_SIZE)
        throw runtime_error("Invalid data - MessageHeader too small.");

    this->magic = vch_to_uint<uint32_t>(bytes, LITTLE_ENDIAN_);
    uchar_vector(bytes.begin() + 4, bytes.begin() + 16).copyToArray((unsigned char*)this->command);
    this->length = vch_to_uint<uint32_t>(uchar_vector(bytes.begin() + 16, bytes.begin() + 20), LITTLE_ENDIAN_);
    this->hasChecksum = (bytes.size() >= 24);
    if (this->hasChecksum)
        this->checksum = vch_to_uint<uint32_t>(uchar_vector(bytes.begin() + 20, bytes.begin() + 24), LITTLE_ENDIAN_);
}

string MessageHeader::toString() const
{
    stringstream ss;
    ss << "magic: 0x" << hex << this->magic << ", command: " << this->command << dec << ", length: " << this->length;
    if (this->hasChecksum)
        ss << ", checksum: 0x" << hex << this->checksum;
    return ss.str();
}

string MessageHeader::toIndentedString(uint spaces) const
{
    stringstream ss;
    ss << blankSpaces(spaces) << "magic: 0x" << hex << this->magic << endl
       << blankSpaces(spaces) << "command: " << this->command << dec << endl
       << blankSpaces(spaces) << "length: " << this->length << endl;
    if (this->hasChecksum)
        ss << blankSpaces(spaces) << "checksum: 0x" << hex << this->checksum;
    return ss.str();
}

///////////////////////////////////////////////////////////////////////////////
//
// class CoinNodeMessage implementation
//
void CoinNodeMessage::setMessage(uint32_t magic, CoinNodeStructure* pPayload)
{
    string command = pPayload->getCommand();
    if (command == "version") {
// VERSION_CHECKSUM_CHANGE
        this->header = MessageHeader(magic, command.c_str(), pPayload->getSize(), pPayload->getChecksum());
        VersionMessage* pMessage = static_cast<VersionMessage*>(pPayload);
        this->pPayload = new VersionMessage(*pMessage);
    }
    else if (command == "verack") {
        this->header = MessageHeader(magic, command.c_str(), pPayload->getSize(), pPayload->getChecksum());
        this->pPayload = new BlankMessage("verack");
    }
    else if (command == "mempool") {
        this->header = MessageHeader(magic, command.c_str(), pPayload->getSize(), pPayload->getChecksum());
        this->pPayload = new BlankMessage("mempool");
    }
    else if (command == "addr") {
        this->header = MessageHeader(magic, command.c_str(), pPayload->getSize(), pPayload->getChecksum());
        AddrMessage* pMessage = static_cast<AddrMessage*>(pPayload);
        this->pPayload = new AddrMessage(*pMessage);
    }
    else if (command == "inv") {
        this->header = MessageHeader(magic, command.c_str(), pPayload->getSize(), pPayload->getChecksum());
        Inventory* pMessage = static_cast<Inventory*>(pPayload);
        this->pPayload = new Inventory(*pMessage);
    }
    else if (command == "getdata") {
        this->header = MessageHeader(magic, command.c_str(), pPayload->getSize(), pPayload->getChecksum());
        GetDataMessage* pMessage = static_cast<GetDataMessage*>(pPayload);
        this->pPayload = new GetDataMessage(*pMessage);
    }
    else if (command == "notfound") {
        this->header = MessageHeader(magic, command.c_str(), pPayload->getSize(), pPayload->getChecksum());
        NotFoundMessage* pMessage = static_cast<NotFoundMessage*>(pPayload);
        this->pPayload = new NotFoundMessage(*pMessage);
    }
    else if (command == "getblocks") {
        this->header = MessageHeader(magic, command.c_str(), pPayload->getSize(), pPayload->getChecksum());
        GetBlocksMessage* pMessage = static_cast<GetBlocksMessage*>(pPayload);
        this->pPayload = new GetBlocksMessage(*pMessage);
    }
    else if (command == "getheaders") {
        this->header = MessageHeader(magic, command.c_str(), pPayload->getSize(), pPayload->getChecksum());
        GetHeadersMessage* pMessage = static_cast<GetHeadersMessage*>(pPayload);
        this->pPayload = new GetHeadersMessage(*pMessage);
    }
    else if (command == "tx") {
        this->header = MessageHeader(magic, command.c_str(), pPayload->getSize(), pPayload->getChecksum());
        Transaction* pMessage = static_cast<Transaction*>(pPayload);
        this->pPayload = new Transaction(*pMessage);
    }
    else if (command == "block") {
        this->header = MessageHeader(magic, command.c_str(), pPayload->getSize(), pPayload->getChecksum());
        CoinBlock* pMessage = static_cast<CoinBlock*>(pPayload);
        this->pPayload = new CoinBlock(*pMessage);
    }
    else if (command == "merkleblock") {
        this->header = MessageHeader(magic, command.c_str(), pPayload->getSize(), pPayload->getChecksum());
        MerkleBlock* pMessage = static_cast<MerkleBlock*>(pPayload);
        this->pPayload = new MerkleBlock(*pMessage);
    }
    else if (command == "headers") {
        this->header = MessageHeader(magic, command.c_str(), pPayload->getSize(), pPayload->getChecksum());
        HeadersMessage* pMessage = static_cast<HeadersMessage*>(pPayload);
        this->pPayload = new HeadersMessage(*pMessage);
    }
    else if (command == "getaddr") {
        this->header = MessageHeader(magic, command.c_str(), pPayload->getSize(), pPayload->getChecksum());
        this->pPayload = new GetAddrMessage();
    }
    else if (command == "filterload") {
        this->header = MessageHeader(magic, command.c_str(), pPayload->getSize(), pPayload->getChecksum());
        FilterLoadMessage* pMessage = static_cast<FilterLoadMessage*>(pPayload);
        this->pPayload = new FilterLoadMessage(*pMessage);
    }
    else if (command == "filteradd") {
        this->header = MessageHeader(magic, command.c_str(), pPayload->getSize(), pPayload->getChecksum());
        FilterAddMessage* pMessage = static_cast<FilterAddMessage*>(pPayload);
        this->pPayload = new FilterAddMessage(*pMessage);
    }
    else if (command == "filterclear") {
        this->header = MessageHeader(magic, command.c_str(), pPayload->getSize(), pPayload->getChecksum());
        this->pPayload = new BlankMessage("filterclear");
    }
    else if (command == "ping") {
        this->header = MessageHeader(magic, command.c_str(), pPayload->getSize(), pPayload->getChecksum());
        PingMessage* pMessage = static_cast<PingMessage*>(pPayload);
        this->pPayload = new PingMessage(*pMessage);
    }
    else if (command == "pong") {
        this->header = MessageHeader(magic, command.c_str(), pPayload->getSize(), pPayload->getChecksum());
        PongMessage* pMessage = static_cast<PongMessage*>(pPayload);
        this->pPayload = new PongMessage(*pMessage);
    }
    else {
        string error_msg = "Unrecognized command: ";
        error_msg += command;
        throw runtime_error(error_msg.c_str());
    }
}

CoinNodeMessage::~CoinNodeMessage()
{
    delete pPayload;
}

uint64_t CoinNodeMessage::getSize() const
{
    return this->header.getSize() + this->pPayload->getSize();
}

uchar_vector CoinNodeMessage::getSerialized() const
{
    if (!pPayload) throw runtime_error("Message not initialized.");
    uchar_vector rval = this->header.getSerialized();
    rval += this->pPayload->getSerialized();
    return rval;
}

void CoinNodeMessage::setSerialized(const uchar_vector& bytes)
{
    this->header.setSerialized(bytes);
    string command = this->header.command;
//      if ((command == "version") || (command == "verack"))
// VERSION_CHECKSUM_CHANGE
/*      if (command == "verack")
            this->header.removeChecksum();
*/
    if (bytes.size() < header.getSize() + header.length)
        throw runtime_error("Invalid data - CoinNodeMessage too small.");

    if (pPayload) {
        delete pPayload;
        pPayload = NULL;
    }

    if (command == "version") {
        this->pPayload =
        new VersionMessage(uchar_vector(bytes.begin() + header.getSize(), bytes.begin() + header.getSize() + header.length));
    }
    else if (command == "verack") {
        this->pPayload = new BlankMessage("verack");
    }
    else if (command == "mempool") {
        this->pPayload = new BlankMessage("mempool");
    }
    else if (command == "addr") {
        this->pPayload =
            new AddrMessage(uchar_vector(bytes.begin() + header.getSize(), bytes.begin() + header.getSize() + header.length));
    }
    else if (command == "inv") {
        this->pPayload =
            new Inventory(uchar_vector(bytes.begin() + header.getSize(), bytes.begin() + header.getSize() + header.length));
    }
    else if (command == "getdata") {
        this->pPayload =
            new GetDataMessage(uchar_vector(bytes.begin() + header.getSize(), bytes.begin() + header.getSize() + header.length));
    }
    else if (command == "notfound") {
        this->pPayload =
            new NotFoundMessage(uchar_vector(bytes.begin() + header.getSize(), bytes.begin() + header.getSize() + header.length));
    }
    else if (command == "getblocks") {
        this->pPayload =
            new GetBlocksMessage(uchar_vector(bytes.begin() + header.getSize(), bytes.begin() + header.getSize() + header.length));
    }
    else if (command == "getheaders") {
        this->pPayload =
            new GetHeadersMessage(uchar_vector(bytes.begin() + header.getSize(), bytes.begin() + header.getSize() + header.length));
    }
    else if (command == "tx") {
        this->pPayload =
            new Transaction(uchar_vector(bytes.begin() + header.getSize(), bytes.begin() + header.getSize() + header.length));
    }
    else if (command == "block") {
        this->pPayload =
            new CoinBlock(uchar_vector(bytes.begin() + header.getSize(), bytes.begin() + header.getSize() + header.length));
    }
    else if (command == "merkleblock") {
        this->pPayload =
            new MerkleBlock(uchar_vector(bytes.begin() + header.getSize(), bytes.begin() + header.getSize() + header.length));
    }
    else if (command == "headers") {
        this->pPayload =
            new HeadersMessage(uchar_vector(bytes.begin() + header.getSize(), bytes.begin() + header.getSize() + header.length));
    }
    else if (command == "getaddr") {
        this->pPayload = new GetAddrMessage();
    }
    else if (command == "filterload") {
        this->pPayload =
            new FilterLoadMessage(uchar_vector(bytes.begin() + header.getSize(), bytes.begin() + header.getSize() + header.length));
    }
    else if (command == "filteradd") {
        this->pPayload =
            new FilterAddMessage(uchar_vector(bytes.begin() + header.getSize(), bytes.begin() + header.getSize() + header.length));
    }
    else if (command == "filterclear") {
        this->pPayload = new BlankMessage("filterclear");
    }
    else if (command == "ping") {
        this->pPayload =
            new PingMessage(uchar_vector(bytes.begin() + header.getSize(), bytes.begin() + header.getSize() + header.length));
    }
    else if (command == "pong") {
        this->pPayload =
            new PongMessage(uchar_vector(bytes.begin() + header.getSize(), bytes.begin() + header.getSize() + header.length));
    }
    else {
        string error_msg = "Unrecognized command: ";
        error_msg += command;
        throw runtime_error(error_msg.c_str());
    }
}

bool CoinNodeMessage::isChecksumValid() const
{
    if (!this->pPayload) throw runtime_error("Message not initialized.");

    if (!this->header.hasChecksum) return true;

    return (this->header.checksum == this->pPayload->getChecksum());
}

string CoinNodeMessage::toString() const
{
    stringstream ss;
    ss << "hash: " << uchar_vector(this->pPayload->getHashLittleEndian()).getHex() << endl
       << "header: {" << this->header.toString() << "}, payload: ";
    if (this->pPayload)
        ss << "{" << this->pPayload->toString() << "}";
    else
        ss << "NULL";
    return ss.str();
}

string CoinNodeMessage::toIndentedString(uint spaces) const
{
    stringstream ss;
    ss << blankSpaces(spaces) << "hash: " << uchar_vector(this->pPayload->getHashLittleEndian()).getHex() << endl
       << blankSpaces(spaces) << "header:" << endl << this->header.toIndentedString(spaces + 2) << endl
       << blankSpaces(spaces) << "payload:";
    if (this->pPayload)
        ss << endl << this->pPayload->toIndentedString(spaces + 2);
    else
        ss << "NULL";
    return ss.str();
}

///////////////////////////////////////////////////////////////////////////////
//
// class VersionMessage implementation
//
VersionMessage::VersionMessage(
    int32_t version,
    uint64_t services,
    int64_t timestamp,
    const NetworkAddress& recipientAddress,
    const NetworkAddress& senderAddress,
    uint64_t nonce,
    const char* subVersion,
    int32_t startHeight,
    bool relay
) :
    version_(version),
    services_(services),
    timestamp_(timestamp),
    recipientAddress_(recipientAddress),
    senderAddress_(senderAddress),
    nonce_(nonce),
    subVersion_(subVersion),
    startHeight_(startHeight),
    relay_(relay)
{
}

uint64_t VersionMessage::getSize() const
{
    uint64_t size = subVersion_.getSize() + MIN_VERSION_MESSAGE_SIZE;
    if (version_ >= 70001) { size++; }
    return size;
}

uchar_vector VersionMessage::getSerialized() const
{
    uchar_vector rval = uint_to_vch(version_, LITTLE_ENDIAN_);
    rval += uint_to_vch(services_, LITTLE_ENDIAN_);
    rval += uint_to_vch(timestamp_, LITTLE_ENDIAN_);
    rval += recipientAddress_.getSerialized();
    rval += senderAddress_.getSerialized();
    rval += uint_to_vch(nonce_, LITTLE_ENDIAN_);
    rval += subVersion_.getSerialized();
    rval += uint_to_vch(startHeight_, LITTLE_ENDIAN_);
    if (version_ >= 70001) { rval.push_back(relay_ ? 1 : 0); }
    return rval;
}

void VersionMessage::setSerialized(const uchar_vector& bytes)
{
    if (bytes.size() < MIN_VERSION_MESSAGE_SIZE)
        throw runtime_error("Invalid data - VersionMessage too small.");

    version_ = vch_to_uint<uint32_t>(bytes, LITTLE_ENDIAN_);
    if (version_ >= 70001 && bytes.size() < MIN_VERSION_MESSAGE_SIZE + 1)
        throw runtime_error("Invalid data - VersionMessage is too small for version >= 70001.");

    uint pos = 4;
    services_ = vch_to_uint<uint64_t>(uchar_vector(bytes.begin() + pos, bytes.begin() + pos + 8), LITTLE_ENDIAN_);
    pos += 8;
    timestamp_ = vch_to_uint<uint64_t>(uchar_vector(bytes.begin() + pos, bytes.begin() + pos + 8), LITTLE_ENDIAN_);
    pos += 8;
    recipientAddress_ = NetworkAddress(uchar_vector(bytes.begin() + pos, bytes.begin() + pos + 26));
    pos += 26;
    senderAddress_ = NetworkAddress(uchar_vector(bytes.begin() + pos, bytes.begin() + pos + 26));
    pos += 26;
    nonce_ = vch_to_uint<uint64_t>(uchar_vector(bytes.begin() + pos, bytes.begin() + pos + 8), LITTLE_ENDIAN_);
    pos += 8;
    subVersion_ = VarString(uchar_vector(bytes.begin() + pos, bytes.end()));
    pos += subVersion_.getSize();
    if (bytes.size() < pos + 4)
        throw runtime_error("Invalid data - VersionMessage missing startHeight.");
    startHeight_ = vch_to_uint<uint32_t>(uchar_vector(bytes.begin() + pos, bytes.begin() + pos + 4), LITTLE_ENDIAN_);
    if (version_ >= 70001)
    {
        pos += 4;
        relay_ = (bytes[pos] != 0);
    }
    else
    {
        relay_ = true;
    }
}

string VersionMessage::toString() const
{
    stringstream ss;
    ss << "version: " << version_ << ", services: " << services_ << ", timestamp: " << timeToString(timestamp_)
       << ", recipientAddress: {" << recipientAddress_.toString()
       << "}, senderAddress: {" << senderAddress_.toString()
       << "}, nonce: " << nonce_ << ", subVersion: \"" << subVersion_.toString() << "\", startHeight: " << startHeight_;
    if (version_ >= 70001)
    {
        ss << ", relay: " << (relay_ ? "true" : "false");
    }
    return ss.str();
}

string VersionMessage::toIndentedString(uint spaces) const
{
    stringstream ss;
    ss << blankSpaces(spaces) << "version: " << version_ << endl
       << blankSpaces(spaces) << "services: " << services_ << endl
       << blankSpaces(spaces) << "timestamp: " << timeToString(timestamp_) << endl
       << blankSpaces(spaces) << "recipientAddress:" << endl << recipientAddress_.toIndentedString(spaces + 2) << endl
       << blankSpaces(spaces) << "senderAddress:" << endl << senderAddress_.toIndentedString(spaces + 2) << endl
       << blankSpaces(spaces) << "nonce: " << nonce_ << endl
       << blankSpaces(spaces) << "subVersion: \"" << subVersion_.toString() << "\"" << endl
       << blankSpaces(spaces) << "startHeight: " << startHeight_;
    if (version_ >= 70001)
    {
        ss << endl << blankSpaces(spaces) << "relay: " << (relay_ ? "true" : "false");
    }
    return ss.str();
}

///////////////////////////////////////////////////////////////////////////////
//
// class AddrMessage implementation
//
uchar_vector AddrMessage::getSerialized() const
{
    uchar_vector rval = VarInt(addrList.size()).getSerialized();
    for (uint i = 0; i < addrList.size(); i++) {
        rval += addrList[i].getSerialized();
    }
    return rval;
}

void AddrMessage::setSerialized(const uchar_vector& bytes)
{
    if (bytes.size() < MIN_ADDR_MESSAGE_SIZE)
        throw runtime_error("Invalid data - AddrMessage too small.");

    addrList.clear();

    VarInt count(bytes);
    uint pos = count.getSize();
    for (uint i = 0; i < count.value; i++) {
        NetworkAddress addr(uchar_vector(bytes.begin() + pos, bytes.begin() + pos + 30));
        addrList.push_back(addr);
        pos += 30;
    }
}

string AddrMessage::toString() const
{
    stringstream ss;
    ss << "[";
    for (uint i = 0; i < addrList.size(); i++) {
        if (i > 0) ss << ", ";
        ss << i << ": {" << addrList[i].toString() << "}";
    }
    ss << "]";
    return ss.str();
}

string AddrMessage::toIndentedString(uint spaces) const
{
    stringstream ss;
    for (uint i = 0; i < addrList.size(); i++) {
        if (i > 0) ss << endl;
        ss << blankSpaces(spaces) << i << ":" << endl << addrList[i].toIndentedString(spaces + 2);
    }
    return ss.str();
}

///////////////////////////////////////////////////////////////////////////////
//
// class InventoryItem implementation
//
uchar_vector InventoryItem::getSerialized() const
{
    uchar_vector rval = uint_to_vch(itemType, LITTLE_ENDIAN_);
    uchar_vector hashBytes((unsigned char*)hash, 32);
    hashBytes.reverse(); // to little endian
    rval += hashBytes;
    return rval;
}

void InventoryItem::setSerialized(const uchar_vector& bytes)
{
    if (bytes.size() < MIN_INVENTORY_ITEM_SIZE)
        throw runtime_error("Invalid data - InventoryItem too small.");

    this->itemType = vch_to_uint<uint32_t>(bytes, LITTLE_ENDIAN_);
    uchar_vector hashBytes(bytes.begin() + 4, bytes.begin() + 36);
    hashBytes.reverse(); // to big endian
    hashBytes.copyToArray((unsigned char*)this->hash);
}

string InventoryItem::toString() const
{
    stringstream ss;
    ss << "itemType: " << itemTypeToString(this->itemType) << ", hash: " << uchar_vector((unsigned char*)this->hash, 32).getHex();
    return ss.str();
}

string InventoryItem::toIndentedString(uint spaces) const
{
    stringstream ss;
    ss << blankSpaces(spaces) << "itemType: " << itemTypeToString(this->itemType) << endl
       << blankSpaces(spaces) << "hash: " << uchar_vector((unsigned char*)this->hash, 32).getHex();
    return ss.str();
}

///////////////////////////////////////////////////////////////////////////////
//
// class Inventory implementation
//
uchar_vector Inventory::getSerialized() const
{
    uchar_vector data = VarInt(this->items.size()).getSerialized();

    for (uint i = 0; i < this->items.size(); i++)
        data += (this->items)[i].getSerialized();

    return data;
}

void Inventory::setSerialized(const uchar_vector& bytes)
{
    VarInt count(bytes);
    uint pos = count.getSize();
    if (bytes.size() < pos + MIN_INVENTORY_ITEM_SIZE*count.value)
        throw runtime_error("Invalid data - message too small.");

    for (uint i = 0; i < count.value; i++) {
        uchar_vector field(bytes.begin() + pos, bytes.end());
        InventoryItem item(field); pos += item.getSerialized().size();
        (this->items).push_back(item);
    }
}

string Inventory::toString() const
{
    stringstream ss;
    ss << "[";
    for (uint i = 0; i < this->items.size(); i++) {
        if (i > 0) ss << ", ";
        ss << i << ": " << this->items[i].toString();
    }
    ss << "]";
    return ss.str();
}

string Inventory::toIndentedString(uint spaces) const
{
    stringstream ss;
    for (uint i = 0; i < this->items.size(); i++) {
        if (i > 0) ss << endl;
        ss << blankSpaces(spaces) << i << ": " << endl << this->items[i].toIndentedString(spaces + 2);
    }
    return ss.str();
}

///////////////////////////////////////////////////////////////////////////////
//
// class GetDataMessage implementation
//
/*
uchar_vector GetDataMessage::getSerialized() const
{
    uchar_vector data = VarInt(this->items.size()).getSerialized();

    for (uint i = 0; i < this->items.size(); i++)
        data += (this->items)[i].getSerialized();

    return data;
}

void GetDataMessage::setSerialized(const uchar_vector& bytes)
{
    VarInt count(bytes);
    uint pos = count.getSize();
    if (bytes.size() < pos + MIN_INVENTORY_ITEM_SIZE*count.value)
        throw runtime_error("Invalid data - GetDataMessage too small.");

    for (uint i = 0; i < count.value; i++) {
        uchar_vector field(bytes.begin() + pos, bytes.end());
        InventoryItem item(field); pos += item.getSerialized().size();
        (this->items).push_back(item);
    }
}

string GetDataMessage::toString() const
{
    stringstream ss;
    ss << "[";
    for (uint i = 0; i < this->items.size(); i++) {
        if (i > 0) ss << ", ";
        ss << i << ": " << this->items[i].toString();
    }
    ss << "]";
    return ss.str();
}

string GetDataMessage::toIndentedString(uint spaces) const
{
    stringstream ss;
    for (uint i = 0; i < this->items.size(); i++) {
        if (i > 0) ss << endl;
        ss << blankSpaces(spaces) << i << ":" << endl << this->items[i].toIndentedString(spaces + 2);
    }
    return ss.str();
}
*/
///////////////////////////////////////////////////////////////////////////////
//
// class GetBlocksMessage implementation
//
GetBlocksMessage::GetBlocksMessage(const string& hex)
{
    uchar_vector bytes;
    bytes.setHex(hex);
    this->setSerialized(bytes);
}

uchar_vector GetBlocksMessage::getSerialized() const
{
    uchar_vector rval = uint_to_vch(this->version, LITTLE_ENDIAN_);
    rval += VarInt(this->blockLocatorHashes.size()).getSerialized();
    for (uint i = 0; i < this->blockLocatorHashes.size(); i++)
        rval += uchar_vector(this->blockLocatorHashes[i].begin(), this->blockLocatorHashes[i].begin() + 32).getReverse();
    rval += uchar_vector(this->hashStop).getReverse();
    return rval;
}

void GetBlocksMessage::setSerialized(const uchar_vector& bytes)
{
    if (bytes.size() < MIN_GET_BLOCKS_SIZE)
        throw runtime_error("Invalid data - GetBlocksMessage too small.");

    this->version = vch_to_uint<uint32_t>(bytes, LITTLE_ENDIAN_); uint pos = 4;
    VarInt count(uchar_vector(bytes.begin() + 4, bytes.end())); pos += count.getSize();
    if (bytes.size() < pos + 32*(count.value + 1))
        throw runtime_error("Invalid data - GetBlocksMessage has wrong length.");
    this->blockLocatorHashes.clear();
    uchar_vector hash;
    for (uint i = 0; i < count.value; i++) {
        hash.assign(bytes.begin() + pos, bytes.begin() + pos + 32); pos += 32;
        hash.reverse();
        this->blockLocatorHashes.push_back(hash);
    }
    this->hashStop.assign(bytes.begin() + pos, bytes.begin() + pos + 32);
    this->hashStop.reverse();
}

string GetBlocksMessage::toString() const
{
    stringstream ss;
    ss << "version: " << this->version << ", blockLocatorHashes: [";
    for (uint i = 0; i < this->blockLocatorHashes.size(); i++) {
        if (i > 0) ss << ", ";
        ss << i << ": " << this->blockLocatorHashes[i].getHex();
    }
    ss << "], hashStop: " << this->hashStop.getHex();
    return ss.str();
}

string GetBlocksMessage::toIndentedString(uint spaces) const
{
    stringstream ss;
    ss << blankSpaces(spaces) << "version: " << this->version << endl
       << blankSpaces(spaces) << "blockLocatorHashes:" << endl;
    for (uint i = 0; i < this->blockLocatorHashes.size(); i++) {
        stringstream is;
        is << i;
        ss << blankSpaces(spaces + 8 - is.str().size()) // pad with spaces on the left so that the numbers are right-aligned.
           << is.str() << ": " << this->blockLocatorHashes[i].getHex() << endl;
    }
    ss << blankSpaces(spaces) << "hashStop: " << this->hashStop.getHex();
    return ss.str();
}

///////////////////////////////////////////////////////////////////////////////
//
// class GetHeadersMessage implementation
//
uchar_vector GetHeadersMessage::getSerialized() const
{
    uchar_vector rval = uint_to_vch(this->version, LITTLE_ENDIAN_);
    rval += VarInt(this->blockLocatorHashes.size()).getSerialized();
    for (uint i = 0; i < this->blockLocatorHashes.size(); i++)
        rval += uchar_vector(this->blockLocatorHashes[i].begin(), this->blockLocatorHashes[i].begin() + 32).getReverse();
    rval += uchar_vector(this->hashStop).getReverse();
    return rval;
}

void GetHeadersMessage::setSerialized(const uchar_vector& bytes)
{
    if (bytes.size() < MIN_GET_BLOCKS_SIZE)
        throw runtime_error("Invalid data - GetHeadersMessage too small.");

    this->version = vch_to_uint<uint32_t>(bytes, LITTLE_ENDIAN_); uint pos = 4;
    VarInt count(uchar_vector(bytes.begin() + pos, bytes.end())); pos += count.getSize();
    if (bytes.size() < pos + 32*(count.value + 1))
        throw runtime_error("Invalid data - GetHeadersMessage has wrong length.");

    this->blockLocatorHashes.clear();
    uchar_vector hash;
    for (uint i = 0; i < count.value; i++) {
        hash.assign(bytes.begin() + pos, bytes.begin() + pos + 32); pos += 32;
        hash.reverse();
        this->blockLocatorHashes.push_back(hash);
    }
    this->hashStop.assign(bytes.begin() + pos, bytes.begin() + pos + 32);
    this->hashStop.reverse();
}

string GetHeadersMessage::toString() const
{
    stringstream ss;
    ss << "blockLocatorHashes: [";
    for (uint i = 0; i < this->blockLocatorHashes.size(); i++) {
        if (i > 0) ss << ", ";
        ss << i << ": " << this->blockLocatorHashes[i].getHex();
    }
    ss << "], hashStop: " << this->hashStop.getHex();
    return ss.str();
}

string GetHeadersMessage::toIndentedString(uint spaces) const
{
    stringstream ss;
    ss << blankSpaces(spaces) << "blockLocatorHashes:" << endl;
    for (uint i = 0; i < this->blockLocatorHashes.size(); i++) {
        stringstream is;
        is << i;
        ss << blankSpaces(spaces + 8 - is.str().size()) // pad with spaces on the left so that the numbers are right-aligned.
           << is.str() << ": " << this->blockLocatorHashes[i].getHex() << endl;
    }
    ss << blankSpaces(spaces) << "hashStop: " << this->hashStop.getHex();
    return ss.str();
}

///////////////////////////////////////////////////////////////////////////////
//
// class OutPoint implementation
//
void OutPoint::setPoint(const uchar_vector& hashBytes, uint index)
{
    uchar_vector(hashBytes).copyToArray(hash);
    this->index = index;
}

uchar_vector OutPoint::getSerialized() const
{
    uchar_vector rval(this->hash, 32);
    rval.reverse(); // to big endian
    rval += uint_to_vch(this->index, LITTLE_ENDIAN_);
    return rval;
}

void OutPoint::setSerialized(const uchar_vector& bytes)
{
    if (bytes.size() < MIN_OUT_POINT_SIZE)
        throw runtime_error("Invalid data - OutPoint too small.");

    uchar_vector hashBytes(bytes.begin(), bytes.begin() + 32);
    hashBytes.reverse(); // to little endian

    memcpy(this->hash, &hashBytes[0], 32);
    this->index = vch_to_uint<uint32_t>(uchar_vector(bytes.begin() + 32, bytes.begin() + 36), LITTLE_ENDIAN_);
}

string OutPoint::toDelimited(const string& delimiter) const
{
    stringstream ss;
    ss << uchar_vector(this->hash, 32).getHex() << delimiter << this->index;
    return ss.str();
}

string OutPoint::toString() const
{
    stringstream ss;
    ss << "hash: " << uchar_vector(this->hash, 32).getHex() << ", index: " << this->index;
    return ss.str();
}

string OutPoint::toIndentedString(uint spaces) const
{
    stringstream ss;
    ss << blankSpaces(spaces) << "hash: " << uchar_vector(this->hash, 32).getHex() << endl
       << blankSpaces(spaces) << "index: " << this->index;
    return ss.str();
}

///////////////////////////////////////////////////////////////////////////////
//
// class TxIn implementation
//
TxIn::TxIn(const OutPoint& previousOut, const string& scriptSigHex, uint32_t sequence)
{
    this->previousOut = previousOut;
    uchar_vector myScriptSig;
    myScriptSig.setHex(scriptSigHex);
    this->scriptSig = myScriptSig;
    this->sequence = sequence;
}

uchar_vector TxIn::getSerialized(bool includeScriptSigLength) const
{
    uchar_vector rval = this->previousOut.getSerialized();
    if (includeScriptSigLength)
        rval += VarInt(this->scriptSig.size()).getSerialized();
    rval += this->scriptSig;
    rval += uint_to_vch(this->sequence, LITTLE_ENDIAN_);
    return rval;
}

void TxIn::setSerialized(const uchar_vector& bytes)
{
    if (bytes.size() < MIN_TX_IN_SIZE)
        throw runtime_error("Invalid data - TxIn too small.");

    this->previousOut.setSerialized(bytes);
    VarInt scriptLength(uchar_vector(bytes.begin() + 36, bytes.end()));
    uint pos =  scriptLength.getSize() + 36;
    if (bytes.size() < pos + scriptLength.value) {
        cout << "TxIn: " << bytes.getHex() << endl;
        throw runtime_error("Invalid data - TxIn script length too small.");
    }

    this->scriptSig.assign(bytes.begin() + pos, bytes.begin() + pos + scriptLength.value);
    pos += scriptLength.value;
    this->sequence = vch_to_uint<uint32_t>(uchar_vector(bytes.begin() + pos, bytes.begin() + pos + 4), LITTLE_ENDIAN_);
}

string TxIn::getAddress() const
{
    // first check if it is coinbase
    if ((uchar_vector(this->previousOut.hash, 32) == g_zero32bytes) &&
        (this->previousOut.index == 0xffffffff)) return "Coinbase";
	
    uint nObjects = 0;
    uint i = 0;
    uint pubkeyBegin = 0;
    while (i < scriptSig.size()) {
        nObjects++;
        pubkeyBegin = i + 1;
        i += scriptSig[i] + 1;
    }
	
    unsigned char version;
    if (nObjects == 1) return "";

    // old address type
    if (nObjects == 2) version = g_addressVersion;
    else version = g_multiSigAddressVersion;

    if (scriptSig.size() == 0) return "zero length";
    else return toBase58Check(ripemd160(sha256(uchar_vector(scriptSig.begin() + pubkeyBegin, scriptSig.end()))), version);	
}

string TxIn::toString() const
{
    stringstream ss;
    ss << "previousOut: {" << this->previousOut.toString() << "}, scriptSig: (" << this->getAddress() << ") " << this->scriptSig.getHex()
       << ", sequence: " << this->sequence;
    return ss.str();
}

string TxIn::toIndentedString(uint spaces) const
{
    stringstream ss;
    ss << blankSpaces(spaces) << "previousOut:" << endl << this->previousOut.toIndentedString(spaces + 2) << endl
       << blankSpaces(spaces) << "scriptSig: (" << this->getAddress() << ") "<< this->scriptSig.getHex() << endl
       << blankSpaces(spaces) << "sequence: " << this->sequence;
    return ss.str();
}

string TxIn::toJson() const
{
    stringstream ss;
    ss << "{\"outpoint_hash\":\"" << uchar_vector(this->previousOut.hash, 32).getHex() << "\",\"outpoint_index\":"
       << this->previousOut.index << ",\"script\":\"" << this->scriptSig.getHex() << "\",\"address\":\""
       << this->getAddress() << "\",\"sequence\":" << this->sequence << "}";
    return ss.str();
}

///////////////////////////////////////////////////////////////////////////////
//
// class TxOut implementation
//
TxOut::TxOut(uint64_t value, const string& scriptPubKeyHex)
{
    this->value = value;
    uchar_vector script;
    script.setHex(scriptPubKeyHex);
    this->scriptPubKey = script;
}

uchar_vector TxOut::getSerialized() const
{
    uchar_vector rval = uint_to_vch(this->value, LITTLE_ENDIAN_);
    rval += VarInt(this->scriptPubKey.size()).getSerialized();
    rval += this->scriptPubKey;
    return rval;
}

void TxOut::setSerialized(const uchar_vector& bytes)
{
    if (bytes.size() < MIN_TX_OUT_SIZE)
        throw runtime_error("Invalid data - TxOut too small.");

    this->value = vch_to_uint<uint64_t>(bytes, LITTLE_ENDIAN_);
    VarInt scriptLength(uchar_vector(bytes.begin() + 8, bytes.end()));
    uint pos = scriptLength.getSize() + 8;
    if (bytes.size() < pos + scriptLength.value)
        throw runtime_error("Invalid data - TxOut script length too small.");

    this->scriptPubKey.assign(bytes.begin() + pos, bytes.begin() + pos + scriptLength.value);
}

string TxOut::getAddress() const
{
    string scriptPubKeyHex = this->scriptPubKey.getHex();
    boost::match_results<string::const_iterator> publicKey;

    // standard transaction using public key hash
    boost::regex rx_pubKeyHash("76a914([0-9a-fA-F]{40})88ac");
    if (boost::regex_search(scriptPubKeyHex, publicKey, rx_pubKeyHash))
        return toBase58Check(uchar_vector(publicKey[1]), g_addressVersion);

    // standard transaction using full public key
    boost::regex rx_pubKeyFull("([0-9a-fA-F]{130})ac");
    if (boost::regex_search(scriptPubKeyHex, publicKey, rx_pubKeyFull))
        return toBase58Check(mdsha(uchar_vector(publicKey[1])), g_addressVersion);

    // standard transaction using x-coordinate of public key only
    boost::regex rx_pubKeyShort("21([0-9a-fA-F]{66})ac");
    if (boost::regex_search(scriptPubKeyHex, publicKey, rx_pubKeyShort))
        return toBase58Check(mdsha(uchar_vector(publicKey[1])), g_addressVersion);
	
    // multisig transaction using hash
    boost::regex rx_multiSigHash("a914([0-9a-fA-F]{40})87");
    if (boost::regex_search(scriptPubKeyHex, publicKey, rx_multiSigHash))
        return toBase58Check(uchar_vector(publicKey[1]), g_multiSigAddressVersion);
	
    // nonstandard
    return "";
}

string TxOut::toString() const
{
    stringstream ss;
    ss << "value: " << satoshisToBtcString(this->value) << ", scriptPubKey: (" << this->getAddress() << ") " << this->scriptPubKey.getHex();
    return ss.str();
}

string TxOut::toIndentedString(uint spaces) const
{
    stringstream ss;
    ss << blankSpaces(spaces) << "value: " << satoshisToBtcString(this->value) << endl
       << blankSpaces(spaces) << "scriptPubKey: (" << this->getAddress() << ") " << this->scriptPubKey.getHex();
    return ss.str();
}

string TxOut::toJson() const
{
    stringstream ss;
    ss << "{\"amount\":" << satoshisToBtcString(this->value) << ",\"amount_int\":" << this->value << ",\"script\":\"" << this->scriptPubKey.getHex()
       << "\",\"address\":\"" << this->getAddress() << "\"}";
    return ss.str();
}

///////////////////////////////////////////////////////////////////////////////
//
// class Transaction implementation
//
Transaction::Transaction(const string& hex)
{
    uchar_vector bytes;
    bytes.setHex(hex);
    this->setSerialized(bytes);
}

uint64_t Transaction::getSize() const
{
    uint64_t count = 8; // version + locktime
    count += VarInt(this->inputs.size()).getSize();
    count += VarInt(this->outputs.size()).getSize();

    uint64_t i;
    for (i = 0; i < this->inputs.size(); i++)
        count += this->inputs[i].getSize();

    for (i = 0; i < this->outputs.size(); i++)
        count += this->outputs[i].getSize();

    return count;
}

uchar_vector Transaction::getSerialized(bool includeScriptSigLength) const
{
    // version
    uchar_vector rval = uint_to_vch(this->version, LITTLE_ENDIAN_);

    uint64_t i;
    // inputs
    rval += VarInt(this->inputs.size()).getSerialized();
    for (i = 0; i < this->inputs.size(); i++)
        rval += this->inputs[i].getSerialized(includeScriptSigLength);

    // outputs
    rval += VarInt(this->outputs.size()).getSerialized();
    for (i = 0; i < this->outputs.size(); i++)
        rval += this->outputs[i].getSerialized();

    // lock time
    rval += uint_to_vch(this->lockTime, LITTLE_ENDIAN_);

    return rval;
}

void Transaction::setSerialized(const uchar_vector& bytes)
{
    if (bytes.size() < MIN_TRANSACTION_SIZE)
        throw runtime_error(string("Invalid data - Transaction too small: ") + bytes.getHex());

    // version
    this->version = vch_to_uint<uint32_t>(uchar_vector(bytes.begin(), bytes.begin() + 4), LITTLE_ENDIAN_);

    uint64_t i;
    // inputs
    this->inputs.clear();
    VarInt count(uchar_vector(bytes.begin() + 4, bytes.end()));
    uint pos = count.getSize() + 4;
    for (i = 0; i < count.value; i++) {
        TxIn txIn(uchar_vector(bytes.begin() + pos, bytes.end()));
        this->addInput(txIn);
        pos += txIn.getSize();
    }

    // outputs
    this->outputs.clear();
    count.setSerialized(uchar_vector(bytes.begin() + pos, bytes.end()));
    pos = pos + count.getSize();
    for (i = 0; i < count.value; i++) {
        TxOut txOut(uchar_vector(bytes.begin() + pos, bytes.end()));
        this->addOutput(txOut);
        pos += txOut.getSize();
    }

    if (bytes.size() < pos + 4)
        throw runtime_error("Invalid data - Transaction missing lockTime.");

    // lock time
    this->lockTime = vch_to_uint<uint32_t>(uchar_vector(bytes.begin() + pos, bytes.begin() + pos + 4), LITTLE_ENDIAN_);
}

string Transaction::toString() const
{
    stringstream ss;
    ss << "version: " << this->version << ", inputs: [";
    uint i;
    for (i = 0; i < this->inputs.size(); i++) {
        if (i > 0) ss << ", ";
        ss << i << ": {" << this->inputs[i].toString() << "}";
    }
    ss << "], outputs: [";
    for (i = 0; i < this->outputs.size(); i++) {
        if (i > 0) ss << ", ";
        ss << i << ": {" << this->outputs[i].toString() << "}";
    }
    ss << "], lockTime: 0x" << hex << this->lockTime;
    return ss.str();
}

string Transaction::toIndentedString(uint spaces) const
{
    stringstream ss;
    ss << blankSpaces(spaces) << "version: " << this->version << endl
       << blankSpaces(spaces) << "inputs:";
    uint i;
    for (i = 0; i < this->inputs.size(); i++)
        ss << endl << blankSpaces(spaces + 2) << i << ":" << endl << this->inputs[i].toIndentedString(spaces + 4);
    ss << endl << blankSpaces(spaces) << "outputs:";
    for (i = 0; i < this->outputs.size(); i++)
        ss << endl << blankSpaces(spaces + 2) << i << ":" << endl << this->outputs[i].toIndentedString(spaces + 4);
    ss << endl << blankSpaces(spaces) << "lockTime: 0x" << hex << this->lockTime;
    return ss.str();
}

string Transaction::toJson() const
{
    stringstream ss;
    ss << "{\"hash\":\"" << this->getHashLittleEndian().getHex() << "\",\"version\":" << this->version << ",\"inputs\":[";
    for (uint i = 0; i < this->inputs.size(); i++) {
        if (i > 0) ss << ",";
        ss << this->inputs[i].toJson();
    }
    ss << "],\"outputs\":[";
    for (uint i = 0; i < this->outputs.size(); i++) {
        if (i > 0) ss << ",";
        ss << this->outputs[i].toJson();
    }
    ss << "],\"locktime\":" << this->lockTime << "}";
    return ss.str();
}

void Transaction::clearScriptSigs()
{
    for (uint i = 0; i < this->inputs.size(); i++)
        this->inputs[i].scriptSig.clear();
}

void Transaction::setScriptSig(uint index, const uchar_vector& scriptSig)
{
    if (index > inputs.size()-1)
        throw runtime_error("Index out of range.");
    inputs[index].scriptSig = scriptSig;
}

void Transaction::setScriptSig(uint index, const string& scriptSigHex)
{
    uchar_vector script;
    script.setHex(scriptSigHex);
    this->setScriptSig(index, script);
}

uint64_t Transaction::getTotalSent() const
{
    uint64_t totalSent = 0;
    for (uint i = 0; i < this->outputs.size(); i++)
        totalSent += this->outputs[i].value;
    return totalSent;
}

uchar_vector Transaction::getHashWithAppendedCode(uint32_t code) const
{
    return sha256_2(this->getSerialized() + uint_to_vch(code, LITTLE_ENDIAN_));
}

///////////////////////////////////////////////////////////////////////////////
//
// class CoinBlockHeader implementation
//

hashfunc_t CoinBlockHeader::hashfunc_ = &sha256_2; // use Hashcash as default. Change with CoinBlockHeader::setHashFunc(<hash function>).
hashfunc_t CoinBlockHeader::powhashfunc_ = &sha256_2;

CoinBlockHeader::CoinBlockHeader(const string& hex)
{
    uchar_vector bytes;
    bytes.setHex(hex);
    setSerialized(bytes);

    resetHash();
}

uchar_vector CoinBlockHeader::getSerialized() const
{
    uchar_vector rval = uint_to_vch(version_, LITTLE_ENDIAN_);
    rval += prevBlockHash_.getReverse(); // all big endian
    rval += merkleRoot_.getReverse();
    rval += uint_to_vch(timestamp_, LITTLE_ENDIAN_);
    rval += uint_to_vch(bits_, LITTLE_ENDIAN_);
    rval += uint_to_vch(nonce_, LITTLE_ENDIAN_);
    return rval;
}

void CoinBlockHeader::setSerialized(const uchar_vector& bytes)
{
    if (bytes.size() < MIN_COIN_BLOCK_HEADER_SIZE)
        throw runtime_error("Invalid data - CoinBlockHeader too small.");

    version_ = vch_to_uint<uint32_t>(bytes, LITTLE_ENDIAN_); uint pos = 4;

    uchar_vector subbytes(bytes.begin() + pos, bytes.begin() + pos + 32); pos += 32;
    subbytes.reverse();
    prevBlockHash_ = subbytes;

    subbytes.assign(bytes.begin() + pos, bytes.begin() + pos + 32); pos += 32;
    subbytes.reverse();
    merkleRoot_ = subbytes;

    timestamp_ = vch_to_uint<uint32_t>(uchar_vector(bytes.begin() + pos, bytes.begin() + pos + 4), LITTLE_ENDIAN_); pos += 4;
    bits_ = vch_to_uint<uint32_t>(uchar_vector(bytes.begin() + pos, bytes.begin() + pos + 4), LITTLE_ENDIAN_); pos += 4;
    nonce_ = vch_to_uint<uint32_t>(uchar_vector(bytes.begin() + pos, bytes.begin() + pos + 4), LITTLE_ENDIAN_); pos += 4;

    resetHash();
}

const BigInt CoinBlockHeader::getTarget() const
{
    uint32_t nExp = bits_ >> 24;
    uint32_t nMantissa = bits_ & 0x007fffff;
    if (nExp <= 3)
    {
        nMantissa >>= 8*(3 - nExp);
        BigInt target(nMantissa);
        return target;
    }
    else
    {
        BigInt target(nMantissa);
        return target << (8*(nExp - 3));
    }    
}

void CoinBlockHeader::setTarget(const BigInt& target)
{
    uint32_t nExp =  target.numBytes();
    uint32_t nMantissa;
    if (nExp <= 3)
    {
        nMantissa = target.getWord() << 8*(3 - nExp);
    }
    else
    {
        nMantissa = (target >> (8*(nExp - 3))).getWord();
    }

    if (nMantissa >> 24) throw std::runtime_error("Mantissa too large.");

    if (nMantissa & 0x00800000)
    {
        nMantissa >>= 8;
        nExp++;
    }

    if (nExp >> 8) throw std::runtime_error("Exponent too large.");

    bits_ = (nExp << 24) | nMantissa;

    resetHash();
}

const BigInt CoinBlockHeader::getWork() const
{
    BigInt target = getTarget();
    if (target <= 0) return 0;
    return (BigInt(1) << 256) / (getTarget() + 1);
}

string CoinBlockHeader::toString() const
{
    stringstream ss;
    ss << "hash: " << getHashLittleEndian().getHex() << ", version: " << version_ << ", prevBlockHash: " << prevBlockHash_.getHex()
       << ", merkleRoot: " << merkleRoot_.getHex() << ", timestamp: " << timeToString(timestamp_)
       << ", bits: " << bits_ << ", nonce: " << nonce_;
    return ss.str();
}

string CoinBlockHeader::toIndentedString(uint spaces) const
{
    stringstream ss;
    ss << blankSpaces(spaces) << "hash: " << getHashLittleEndian().getHex() << endl
       << blankSpaces(spaces) << "version: " << dec << version_ << endl
       << blankSpaces(spaces) << "prevBlockHash: " << prevBlockHash_.getHex() << endl
       << blankSpaces(spaces) << "merkleRoot: " << merkleRoot_.getHex() << endl
       << blankSpaces(spaces) << "timestamp: 0x" << hex << timestamp_ << " (" << timeToString(timestamp_) << ")" << endl
       << blankSpaces(spaces) << "bits: " << dec << bits_ << endl
       << blankSpaces(spaces) << "nonce: " << dec << nonce_;
    return ss.str();
}

const uchar_vector& CoinBlockHeader::getHash() const
{
    if (!isHashSet_)
    {
        hash_ = CoinNodeStructure::getHash(hashfunc_);
        hashLittleEndian_ = hash_.getReverse();
        isHashSet_ = true;
    }
    return hash_; 
}

const uchar_vector& CoinBlockHeader::getHashLittleEndian() const
{
    if (!isHashSet_)
    {
        hash_ = CoinNodeStructure::getHash(hashfunc_);
        hashLittleEndian_ = hash_.getReverse();
        isHashSet_ = true;
    }
    return hashLittleEndian_; 
}

const uchar_vector& CoinBlockHeader::getPOWHash() const
{
    if (!isPOWHashSet_)
    {
        POWHash_ = CoinNodeStructure::getHash(powhashfunc_);
        POWHashLittleEndian_ = POWHash_.getReverse();
        isPOWHashSet_ = true;
    }
    return POWHash_; 
}

const uchar_vector& CoinBlockHeader::getPOWHashLittleEndian() const
{
    if (!isPOWHashSet_)
    {
        POWHash_ = CoinNodeStructure::getHash(powhashfunc_);
        POWHashLittleEndian_ = POWHash_.getReverse();
        isPOWHashSet_ = true;
    }
    return POWHashLittleEndian_; 
}

///////////////////////////////////////////////////////////////////////////////
//
// class CoinBlock implementation
//
CoinBlock::CoinBlock(const string& hex)
{
    uchar_vector bytes;
    bytes.setHex(hex);
    this->setSerialized(bytes);
}

uint64_t CoinBlock::getSize() const
{
    uint64_t size = 80 + VarInt(this->txs.size()).getSize();
    for (uint i = 0; i < txs.size(); i++)
        size += txs[i].getSize();
    return size;
}

uchar_vector CoinBlock::getSerialized() const
{
    uchar_vector rval = this->blockHeader.getSerialized();

    // add transactions
    rval += VarInt(this->txs.size()).getSerialized();
    for (uint i = 0; i < this->txs.size(); i++)
        rval += this->txs[i].getSerialized();
    return rval;
}

void CoinBlock::setSerialized(const uchar_vector& bytes)
{
    if (bytes.size() < MIN_COIN_BLOCK_SIZE)
        throw runtime_error("Invalid data - CoinBlock too small.");

    this->blockHeader.setSerialized(bytes);
    uint pos = MIN_COIN_BLOCK_HEADER_SIZE;

    MerkleTree txMerkleTree;
    VarInt count(uchar_vector(bytes.begin() + pos, bytes.end())); pos += count.getSize();
    for (uint i = 0; i < count.value; i++) {
        Transaction tx(uchar_vector(bytes.begin() + pos, bytes.end())); pos += tx.getSize();
        this->txs.push_back(tx);
        txMerkleTree.addHash(tx.getHash());
        if (bytes.size() < pos)
            throw runtime_error("Invalid data - CoinBlock transactions exceed block size.");
    }
    if (blockHeader.merkleRoot() != txMerkleTree.getRootLittleEndian()) {
        throw runtime_error("Invalid data - CoinBlock merkle root mismatch.");
    }
}

string CoinBlock::toString() const
{
    stringstream ss;
    ss << this->blockHeader.toString() << ", txs: [";
    for (uint i = 0; i < this->txs.size(); i++) {
        if (i > 0) ss << ", ";
        ss << i << ": {" << this->txs[i].toString() << "}";
    }
    ss << "]";
    return ss.str();
}

string CoinBlock::toIndentedString(uint spaces) const
{
    int64_t height = this->getHeight();
    stringstream ss;
    ss << blankSpaces(spaces) << "blockHeader:" << endl << this->blockHeader.toIndentedString(spaces + 2) << endl
       << blankSpaces(spaces) << "txs:";
    for (uint i = 0; i < this->txs.size(); i++) {
        ss << endl << blankSpaces(spaces + 2) << i << ":" << endl << this->txs[i].toIndentedString(spaces + 4);
    }
    if (height >= 0) ss << endl << blankSpaces(spaces) << "height: " << height;
    return ss.str();
}

string CoinBlock::toRedactedIndentedString(uint spaces) const
{
    int64_t height = this->getHeight();
    stringstream ss;
    ss << blankSpaces(spaces) << "blockHeader:" << endl << this->blockHeader.toIndentedString(spaces + 2) << endl
       << blankSpaces(spaces) << "txs:";
    for (uint i = 0; i < this->txs.size(); i++) {
        ss << endl << blankSpaces(spaces + 2) << i << ":" << this->txs[i].getHashLittleEndian().getHex();
    }
    if (height >= 0) ss << endl << blankSpaces(spaces) << "height: " << height;
    return ss.str();
}

bool CoinBlock::isValidMerkleRoot() const
{
    MerkleTree tree;
    for (uint i = 0; i < this->txs.size(); i++)
        tree.addHash(txs[i].getHash());

    return (blockHeader.merkleRoot() == tree.getRootLittleEndian());
}

void CoinBlock::updateMerkleRoot()
{
    MerkleTree tree;
    for (uint i = 0; i < this->txs.size(); i++)
        tree.addHash(txs[i].getHash());

    blockHeader.merkleRoot_ = tree.getRootLittleEndian();
    blockHeader.resetHash();
}

uint64_t CoinBlock::getTotalSent() const
{
    uint64_t totalSent = 0;
    for (uint i = 0; i < this->txs.size(); i++)
        totalSent += this->txs[i].getTotalSent();
    return totalSent;
}

int64_t CoinBlock::getHeight() const
{
    if (this->blockHeader.version() < 2) return -1;

    assert(txs.size() > 0);
    assert(txs[0].inputs.size() > 0);
    assert(txs[0].inputs[0].scriptSig.size() > 0);
    uint nBytes = (uint)txs[0].inputs[0].scriptSig[0];
    assert(nBytes < txs[0].inputs[0].scriptSig.size());
    int64_t height = 0;
    int64_t coef = 1;
    for (uint i = 1; i <= nBytes; i++) {
        height += (int64_t)txs[0].inputs[0].scriptSig[i]*coef;
        coef *= 256;
    }
    return height;
}

///////////////////////////////////////////////////////////////////////////////
//
// class MerkleBlock implementation
//
MerkleBlock::MerkleBlock(const PartialMerkleTree& merkleTree, uint32_t version, const uchar_vector& prevBlockHash, uint32_t timestamp, uint32_t bits, uint32_t nonce)
{
    blockHeader = CoinBlockHeader(version, prevBlockHash, merkleTree.getRootLittleEndian(), timestamp, bits, nonce);

    nTxs = merkleTree.getNTxs();
    hashes = merkleTree.getMerkleHashesVector();
    flags = merkleTree.getFlags();
}

PartialMerkleTree MerkleBlock::merkleTree() const
{
    return PartialMerkleTree(nTxs, hashes, flags, blockHeader.merkleRoot());
}

uint64_t MerkleBlock::getSize() const
{
    return MIN_COIN_BLOCK_HEADER_SIZE + 4 + VarInt(hashes.size()).getSize() + (hashes.size() * 32) + VarInt(flags.size()).getSize() + flags.size();
}

uchar_vector MerkleBlock::getSerialized() const
{
    uchar_vector rval = blockHeader.getSerialized();
    rval += uint_to_vch(nTxs, LITTLE_ENDIAN_);
    rval += VarInt(hashes.size()).getSerialized();
    for (uint i = 0; i < hashes.size(); i++) {
        // TODO: make sure hashes are all 32 bytes
        rval += hashes[i];
    }
    rval += VarInt(flags.size()).getSerialized();
    rval += flags;
    return rval;
}

void MerkleBlock::setSerialized(const uchar_vector& bytes)
{
    if (bytes.size() < MIN_MERKLE_BLOCK_SIZE)
        throw runtime_error("Invalid data - MerkleBlock too small.");

    this->blockHeader.setSerialized(bytes);
    uint pos = MIN_COIN_BLOCK_HEADER_SIZE;

    nTxs = vch_to_uint<uint32_t>(uchar_vector(bytes.begin() + pos, bytes.begin() + pos + 4), LITTLE_ENDIAN_); pos += 4;

    VarInt nHashes(uchar_vector(bytes.begin() + pos, bytes.end())); pos += nHashes.getSize();
    if (bytes.size() < pos + (nHashes.value * 32) + 1)
        throw runtime_error("Invalid data - MerkleBlock hash count invalid.");

    hashes.clear();
    for (uint i = 0; i < nHashes.value; i++) {
        hashes.push_back(uchar_vector(bytes.begin() + pos, bytes.begin() + pos + 32)); pos += 32;
    }
        
    VarInt nFlags(uchar_vector(bytes.begin() + pos, bytes.end())); pos += nFlags.getSize();
    if (bytes.size() < pos + nFlags.value)
        throw runtime_error("Invalid data - MerkleBlock flag count invalid.");

    flags.clear();
    for (uint i = 0; i < nFlags.value; i++) {
        flags.push_back(bytes[pos + i]);
    }
}

string MerkleBlock::toString() const
{
    stringstream ss;
    ss << this->blockHeader.toString() << ", nTxs: " << this->nTxs << ", hashes: [";
    for (uint i = 0; i < this->hashes.size(); i++) {
        if (i > 0) ss << ", ";
        uchar_vector hashLittleEndian(this->hashes[i]);
        std::reverse(hashLittleEndian.begin(), hashLittleEndian.end());
        ss << i << ": " << hashLittleEndian.getHex();
    }
    ss << "], flags: " << this->flags.getHex();
    return ss.str();
}

string MerkleBlock::toIndentedString(uint spaces) const
{
    stringstream ss;
    ss << blankSpaces(spaces) << "blockHeader:" << endl << this->blockHeader.toIndentedString(spaces + 2) << endl
       << blankSpaces(spaces) << "nTxs: " << this->nTxs << endl
       << blankSpaces(spaces) << "hashes:";
    for (uint i = 0; i < this->hashes.size(); i++) {
        uchar_vector hashLittleEndian(this->hashes[i]);
        std::reverse(hashLittleEndian.begin(), hashLittleEndian.end());
        ss << endl << blankSpaces(spaces + 2) << i << ":" << hashLittleEndian.getHex();
    }
    ss << endl << blankSpaces(spaces) << "flags: " << this->flags.getHex() << endl;
    return ss.str();
}

bool MerkleBlock::isValidMerkleRoot() const
{
    return false;
}

void MerkleBlock::updateMerkleRoot()
{
}

///////////////////////////////////////////////////////////////////////////////

//
// class HeadersMessage implementation
//
HeadersMessage::HeadersMessage(const string& hex)
{
    uchar_vector bytes;
    bytes.setHex(hex);
    this->setSerialized(bytes);
}

uint64_t HeadersMessage::getSize() const
{
    return VarInt(this->headers.size()).getSize() + this->headers.size()*(MIN_COIN_BLOCK_HEADER_SIZE + 1);
}

uchar_vector HeadersMessage::getSerialized() const
{
    uchar_vector rval = VarInt(this->headers.size()).getSerialized();
    for (uint i = 0; i < this->headers.size(); i++) {
        rval += this->headers[i].getSerialized();
        rval.push_back(0);
    }
    return rval;
}

void HeadersMessage::setSerialized(const uchar_vector& bytes)
{
    VarInt count(bytes);
    if (bytes.size() < count.getSize() + count.value*(MIN_COIN_BLOCK_HEADER_SIZE + 1))
        throw runtime_error("Invalid data - HeadersMessage too small.");

    uint pos = count.getSize();
    this->headers.clear();
    for (uint i = 0; i < count.value; i++) {
        CoinBlockHeader header(uchar_vector(bytes.begin() + pos, bytes.begin() + pos + MIN_COIN_BLOCK_HEADER_SIZE));
        this->headers.push_back(header);
        pos += MIN_COIN_BLOCK_HEADER_SIZE + 1; // an extra blank byte is added.
    }
}

string HeadersMessage::toString() const
{
    stringstream ss;
    ss << "[";
    for (uint i = 0; i < this->headers.size(); i++) {
        if (i > 0) ss << ", ";
        ss << i << ": {" << this->headers[i].toString() << "}";
    }
    ss << "]";
    return ss.str();
}

string HeadersMessage::toIndentedString(uint spaces) const
{
    stringstream ss;
    for (uint i = 0; i < this->headers.size(); i++) {
        if (i > 0) ss << endl;
        ss << blankSpaces(spaces) << i << ":" << endl << this->headers[i].toIndentedString(spaces + 2);
    }
    return ss.str();
}

///////////////////////////////////////////////////////////////////////////////
//
// class FilterLoadMessage implementation
//
uint64_t FilterLoadMessage::getSize() const
{
    return VarInt(filter.size()).getSize() + filter.size() + 9; 
}

uchar_vector FilterLoadMessage::getSerialized() const
{
    uchar_vector rval = VarInt(filter.size()).getSerialized();
    rval += filter;
    rval += uint_to_vch(nHashFuncs, LITTLE_ENDIAN_);
    rval += uint_to_vch(nTweak, LITTLE_ENDIAN_);
    rval.push_back(nFlags);
    return rval;
}

void FilterLoadMessage::setSerialized(const uchar_vector& bytes)
{
    if (bytes.size() < MIN_FILTER_LOAD_SIZE) {
        throw std::runtime_error("Invalid data - FilterLoadMessage too small.");
    }

    VarInt filterSize(bytes);
    std::size_t filterSizeLength = filterSize.getSize();
    if (bytes.size() != filterSizeLength + filterSize.value + 9) {
        throw std::runtime_error("Invalid data - filter length incorrect.");
    }

    uint pos = filterSizeLength + filterSize.value;
    filter.assign(bytes.begin() + filterSizeLength, bytes.begin() + pos);
    nHashFuncs = vch_to_uint<uint32_t>(uchar_vector(bytes.begin() + pos, bytes.begin() + pos + 4), LITTLE_ENDIAN_); pos += 4;
    nTweak = vch_to_uint<uint32_t>(uchar_vector(bytes.begin() + pos, bytes.begin() + pos + 4), LITTLE_ENDIAN_); pos += 4;
    nFlags = (uint8_t)bytes[pos]; 
}

std::string FilterLoadMessage::toString() const
{
    return "";
}

std::string FilterLoadMessage::toIndentedString(uint spaces) const
{
    return "";
}

///////////////////////////////////////////////////////////////////////////////
//
// class FilterAddMessage implementation
//
void FilterAddMessage::setSerialized(const uchar_vector& bytes)
{
    if (bytes.empty()) {
        throw std::runtime_error("Invalid data - cannot be empty.");
    }

    VarInt dataSize(bytes);
    uint pos = dataSize.getSize();
    if (bytes.size() < pos + dataSize.value) {
        throw std::runtime_error("Invalid data - too short.");
    }

    data.assign(bytes.begin() + pos, bytes.begin() + pos + dataSize.value);
}

std::string FilterAddMessage::toString() const
{
    return "";
}

std::string FilterAddMessage::toIndentedString(uint spaces) const
{
    return "";
}

///////////////////////////////////////////////////////////////////////////////
//
// class PingMessage implementation
//
PingMessage::PingMessage()
{
    nonce = (((uint64_t) rand() <<  0) & 0x00000000FFFFFFFFull) | (((uint64_t) rand() << 32) & 0xFFFFFFFF00000000ull);
}

uchar_vector PingMessage::getSerialized() const
{
    return uint_to_vch(nonce, LITTLE_ENDIAN_);
}

void PingMessage::setSerialized(const uchar_vector& bytes)
{
    if (bytes.size() < sizeof(uint64_t)) {
        throw std::runtime_error("Invalid data - PingMessage too small.");
    }

    nonce = vch_to_uint<uint64_t>(uchar_vector(bytes.begin(), bytes.begin() + 8), LITTLE_ENDIAN_);
}

std::string PingMessage::toString() const
{
    return "";
}

std::string PingMessage::toIndentedString(uint spaces) const
{
    return "";
}

///////////////////////////////////////////////////////////////////////////////
//
// class PongMessage implementation
//
uchar_vector PongMessage::getSerialized() const
{
    return uint_to_vch(nonce, LITTLE_ENDIAN_);
}

void PongMessage::setSerialized(const uchar_vector& bytes)
{
    if (bytes.size() < sizeof(uint64_t)) {
        throw std::runtime_error("Invalid data - PongMessage too small.");
    }

    nonce = vch_to_uint<uint64_t>(uchar_vector(bytes.begin(), bytes.begin() + 8), LITTLE_ENDIAN_);
}

std::string PongMessage::toString() const
{
    return "";
}

std::string PongMessage::toIndentedString(uint spaces) const
{
    return "";
}

