///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// coinparams.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include "coinparams.h"

const CoinQ::CoinParams COIN_PARAMS = CoinQ::getBitcoinParams();

const CoinQ::CoinParams& getCoinParams()
{
    return COIN_PARAMS;
}

