///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_exceptions.h
//
// Copyright (c) 2012-2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

#include <stdutils/customerror.h>

namespace CoinQ {

enum ErrorCodes
{
    NETWORK_SELECTOR_NO_NETWORK_SELECTED = 10000,
    NETWORK_SELECTOR_NETWORK_NOT_RECOGNIZED
};

// EXCEPTIONS
class NetworkSelectorException : public stdutils::custom_error
{
public:
    virtual ~NetworkSelectorException() throw() { }

protected:
    explicit NetworkSelectorException(const std::string& what, int code) : stdutils::custom_error(what, code) { }
};

class NetworkSelectorNoNetworkSelectedException : public NetworkSelectorException
{
public:
    explicit NetworkSelectorNoNetworkSelectedException() : NetworkSelectorException("No network selected.", NETWORK_SELECTOR_NO_NETWORK_SELECTED) {}
};

class NetworkSelectorNetworkNotRecognizedException : public NetworkSelectorException
{
public:
    explicit NetworkSelectorNetworkNotRecognizedException(const std::string& network) : NetworkSelectorException("Network not recognized.", NETWORK_SELECTOR_NETWORK_NOT_RECOGNIZED), network_(network) {}

    const std::string& network() const { return network_; }

private:
    std::string network_;
};

}

