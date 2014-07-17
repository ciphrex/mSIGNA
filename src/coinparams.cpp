///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// coinparams.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#include "coinparams.h"

#define DEFAULT_NETWORK "bitcoin"

CoinQ::NetworkSelector selector(DEFAULT_NETWORK);
CoinQ::NetworkSelector& getNetworkSelector() { return selector; }

const CoinQ::CoinParams& getCoinParams(const std::string& network_name)
{
    return selector.getCoinParams(network_name);
}

