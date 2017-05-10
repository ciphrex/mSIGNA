///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// coinparams.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "coinparams.h"

#include <stdexcept>

#ifdef DEFAULT_NETWORK_LITECOIN
#define DEFAULT_NETWORK "litecoin"
#else
#define DEFAULT_NETWORK "bitcoin"
#endif

QString getDefaultNetwork() { return DEFAULT_NETWORK; }

CoinQ::NetworkSelector selector(DEFAULT_NETWORK);
CoinQ::NetworkSelector& getNetworkSelector() { return selector; }

const CoinQ::CoinParams& getCoinParams(const std::string& network_name)
{
    return selector.getCoinParams(network_name);
}

bool userTrailingDecimals = true;
QString currencySymbol;
uint64_t currencyDivisor;
int currencyDecimals;
uint64_t currencyMax;
uint64_t defaultFee;

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
        currencyMax = getCoinParams().currency_max();
    }
    else if (unitPrefix == "m")
    {
        currencyDivisor = getCoinParams().currency_divisor() / 1000;
        currencyMax = getCoinParams().currency_max() * 1000;
        if (currencyMax < getCoinParams().currency_max()) { currencyMax = 0xffffffffffffffffull; }
    }
    else if (unitPrefix == "u")
    {
        currencyDivisor = getCoinParams().currency_divisor() / 1000000;
        currencyMax = getCoinParams().currency_max() * 1000000;
        if (currencyMax < getCoinParams().currency_max()) { currencyMax = 0xffffffffffffffffull; }
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

void setTrailingDecimals(bool show)
{
    userTrailingDecimals = show;
}

QString getFormattedCurrencyAmount(int64_t value, TrailingDecimalsFlag flag)
{
    if (currencyDivisor == 0) throw std::runtime_error("Invalid currency unit.");

    QString formattedAmount = QString::number(value/(1.0 * currencyDivisor), 'f', currencyDecimals);

    if ((flag == HIDE_TRAILING_DECIMALS || (flag == USER_TRAILING_DECIMALS && !userTrailingDecimals)) && currencyDecimals > 0)
    {
        int newLength = formattedAmount.length();
        while (newLength > 0 && formattedAmount[newLength - 1] == '0') { newLength--; }
        if (newLength > 0 && formattedAmount[newLength - 1] == '.') { newLength--; }
        formattedAmount = formattedAmount.left(newLength);
    }

    return formattedAmount;
}

const QString& getCurrencySymbol()
{
    return currencySymbol;
}

uint64_t getCurrencyDivisor()
{
    return currencyDivisor;
}

int getCurrencyDecimals()
{
    return currencyDecimals;
}

uint64_t getCurrencyMax()
{
    return currencyMax;
}

uint64_t getDefaultFee()
{
    return getCoinParams().default_fee();
}

#ifdef SUPPORT_OLD_ADDRESS_VERSIONS
static bool useOldAddressVersions = false;

void setAddressVersions(bool useOld)
{
    useOldAddressVersions = useOld;
}

bool getUseOldAddressVersions()
{
    return useOldAddressVersions;
}
#endif
