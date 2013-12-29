///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_addr.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef _COINQ_ADDR_H_
#define _COINQ_ADDR_H_

#include <CoinNodeData.h>

namespace CoinQ {
namespace Addr {

class RatedNetworkAddress : public Coin::NetworkAddress
{
public:
    unsigned char rating;

    RatedNetworkAddress() : Coin::NetworkAddress() { }
    RatedNetworkAddress(const Coin::NetworkAddress& netaddr, unsigned char _rating = 5) : Coin::NetworkAddress(netaddr), rating(_rating) { }

    uchar_vector getSerialized(bool bWithRating)
    {
        uchar_vector rval = Coin::NetworkAddress::getSerialized();
        rval.push_back(rating);
        return rval;
    }

    void setSerialized(const uchar_vector& bytes)
    {
        Coin::NetworkAddress::setSerialized(bytes);
        if (bytes.size() == MIN_NETWORK_ADDRESS_SIZE + 1 || bytes.size() == MIN_NETWORK_ADDRESS_SIZE + 5) {
            rating = bytes.back();
        }
        else {
            rating = 5;
        }
    }
};

}
}

#endif // _COINQ_ADDR_H_

