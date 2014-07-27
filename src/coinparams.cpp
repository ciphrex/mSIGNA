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

#include <stdexcept>

#define DEFAULT_NETWORK "bitcoin"

CoinQ::NetworkSelector selector(DEFAULT_NETWORK);
CoinQ::NetworkSelector& getNetworkSelector() { return selector; }

const CoinQ::CoinParams& getCoinParams(const std::string& network_name)
{
    return selector.getCoinParams(network_name);
}

QString currencySymbol;
uint64_t currencyDivisor;
int currencyDecimals;

QStringList getValidCurrencyPrefixes()
{
    QStringList prefixes;
    prefixes << "";

    uint64_t currency_divisor = getCoinParams().currency_divisor();
    if (currency_divisor >= 1000) { prefixes << "m"; }
    if (currency_divisor >= 1000000) { prefixes << "u"; }
    return prefixes;
}

void setCurrencyUnitPrefix(const QString& unitPrefix)
{
    if (unitPrefix == "")
    {
        currencyDivisor = getCoinParams().currency_divisor();
    }
    else if (unitPrefix == "m")
    {
        currencyDivisor = getCoinParams().currency_divisor() / 1000;
    }
    else if (unitPrefix == "u")
    {
        currencyDivisor = getCoinParams().currency_divisor() / 1000000;
    }
    else throw std::runtime_error("Invalid currency unit prefix.");

    if (currencyDivisor == 0) throw std::runtime_error("Invalid currency unit prefix.");

    currencySymbol = unitPrefix + getCoinParams().currency_symbol();

    currencyDecimals = 0;
    uint64_t n = currencyDivisor;
    while (n > 1)
    {
        currencyDecimals++;
        n /= 10;
    }
}

QString getFormattedCurrencyAmount(uint64_t value)
{
    if (currencyDivisor == 0) throw std::runtime_error("Invalid currency unit.");

    return QString::number(value/(1.0 * currencyDivisor), 'g', currencyDecimals);
}

const QString& getCurrencySymbol()
{
    return currencySymbol;
}

