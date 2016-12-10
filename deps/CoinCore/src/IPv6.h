////////////////////////////////////////////////////////////////////////////////
//
// IPv6.h
//
// Copyright (c) 2011-2012 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#ifndef IPV6_H_INCLUDED
#define IPV6_H_INCLUDED

#include <cstring>
#include <string>
#include <stdexcept>

class IPv6AddressException : public std::runtime_error
{
public:
    IPv6AddressException(const char* description) : std::runtime_error(description) { }
};

class IPv6Address
{
private:
    unsigned char bytes[16];

public:
    IPv6Address() { memset(&bytes[0], 0, sizeof(bytes)); }
    IPv6Address(const std::string& address) { set(address); }
    IPv6Address(const IPv6Address& source) { memcpy(bytes, source.bytes, sizeof(bytes)); }
    IPv6Address(const unsigned char bytes[]) { set(bytes); }

    void set(const unsigned char _bytes[]) { memcpy(bytes, _bytes, 16); }
    void set(const std::string& address);

    IPv6Address& operator=(const unsigned char bytes[]) { set(bytes); return *this; }
    IPv6Address& operator=(const std::string& address) { set(address); return *this; }

    bool isIPv4() const;

    std::string toString(bool bShorten = false) const;
    std::string toIPv4String() const;
    std::string toStringAuto() const;

    const unsigned char* getBytes() const { return bytes; }
};

#endif // IPV6_H_INCLUDED
