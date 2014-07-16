///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// coinparams.h 
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

#include <CoinQ/CoinQ_coinparams.h>

extern const CoinQ::NetworkSelector& getNetworkSelector();

extern const CoinQ::CoinParams& getCoinParams(const std::string& network_name = "");
