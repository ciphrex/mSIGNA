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

#include <QStringList>

CoinQ::NetworkSelector& getNetworkSelector();

const CoinQ::CoinParams& getCoinParams(const std::string& network_name = "");

QStringList getValidCurrencyPrefixes();
void setCurrencyUnitPrefix(const QString& unitPrefix);
QString getFormattedCurrencyAmount(uint64_t value);

uint64_t getCurrencyDivisor();
const QString& getCurrencySymbol();
int getCurrencyDecimals();
uint64_t getCurrencyMax();
