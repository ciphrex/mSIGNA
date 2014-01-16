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

const CoinQ::CoinParams COIN_PARAMS = CoinQ::getLitecoinParams();

const CoinQ::CoinParams& getCoinParams()
{
    return COIN_PARAMS;
}

