////////////////////////////////////////////////////////////////////////////////
//
// IPv6.cpp
//
// Copyright (c) 2011-2012 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "IPv6.h"

#include <algorithm>
#include <sstream>
#include <iomanip>

void IPv6Address::set(const std::string& address)
{
    std::stringstream ss(address);
    std::string group;
    
    int i = 0;
    
    // IPv4
    if (address.size() <= 15)
    {
        int byte;
        unsigned char bytes[4];
        
        while (std::getline(ss, group, '.'))
        {
            if (i > 3)
                throw IPv6AddressException("Invalid address");
            
            try {
                byte = strtoul(group.c_str(), NULL, 10);
            }
            catch (const std::exception& e) {
                throw IPv6AddressException("Invalid address");
            }
            
            if ((byte < 0) || (byte > 255))
                throw IPv6AddressException("Invalid address");
            
            bytes[i++] = (unsigned char)byte;
        }
        
        if (i < 4) throw IPv6AddressException("Invalid address");
        
        memset(this->bytes, 0, 10);
        memset(&this->bytes[10], 0xff, 2);
        memcpy(&this->bytes[12], bytes, 4);
        return;
    }
    
    // IPv6
    while (std::getline(ss, group, ':'))
    {
        if (i > 15)
            throw IPv6AddressException("Invalid address");
        
        int bytepair;
        try {
            bytepair = strtoul(group.c_str(), NULL, 16);
        }
        catch (const std::exception& e) {
            throw IPv6AddressException("Invalid address");
        }
        
        if ((bytepair < 0) || (bytepair > 0xffff))
            throw IPv6AddressException("Invalid address");
        
        this->bytes[i++] = (unsigned char)(bytepair / 0x100);
        this->bytes[i++] = (unsigned char)(bytepair % 0x100);
    }
}

bool IPv6Address::isIPv4() const
{
    const unsigned char ipv4LeadBytes[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff };
    return (memcmp(bytes, ipv4LeadBytes, sizeof(ipv4LeadBytes)) == 0);
}

std::string IPv6Address::toString(bool bShorten) const
{
    std::stringstream ss;
    ss  << std::hex << std::setfill('0')
    << std::setw(2) << (int)bytes[0] << std::setw(2) << (int)bytes[1] << ":"
    << std::setw(2) << (int)bytes[2] << std::setw(2) << (int)bytes[3] << ":"
    << std::setw(2) << (int)bytes[4] << std::setw(2) << (int)bytes[5] << ":"
    << std::setw(2) << (int)bytes[6] << std::setw(2) << (int)bytes[7] << ":"
    << std::setw(2) << (int)bytes[8] << std::setw(2) << (int)bytes[9] << ":"
    << std::setw(2) << (int)bytes[10] << std::setw(2) << (int)bytes[11] << ":"
    << std::setw(2) << (int)bytes[12] << std::setw(2) << (int)bytes[13] << ":"
    << std::setw(2) << (int)bytes[14] << std::setw(2) << (int)bytes[15];
    return ss.str();
}

std::string IPv6Address::toIPv4String() const
{
    std::stringstream ss;
    ss << (int)bytes[12] << "." << (int)bytes[13] << "." << (int)bytes[14] << "." << (int)bytes[15];
    return ss.str();
}

std::string IPv6Address::toStringAuto() const
{
    if (isIPv4()) return toIPv4String();
    return toString();
}
