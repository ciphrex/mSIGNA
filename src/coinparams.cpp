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

const std::string DEFAULT_NETWORK = "bitcoin";

CoinQ::NetworkSelector selector;
const CoinQ::NetworkSelector& getNetworkSelector() { return selector; }

const CoinQ::CoinParams& getCoinParams(const std::string& network_name)
{
    if (network_name.empty()) return selector.getCoinParams(DEFAULT_NETWORK);
    return selector.getCoinParams(network_name);
}

