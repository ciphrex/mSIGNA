////////////////////////////////////////////////////////////////////////////////
//
// IPv6.h
//
// Copyright (c) 2011-2012 Eric Lombrozo
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
