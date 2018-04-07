///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// coinparams.h 
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include <CoinQ/CoinQ_coinparams.h>

#include <QStringList>

QString getDefaultNetwork();

CoinQ::NetworkSelector& getNetworkSelector();

const CoinQ::CoinParams& getCoinParams(const std::string& network_name = "");

QStringList getValidCurrencyPrefixes();
void setCurrencyUnitPrefix(const QString& unitPrefix);
void setTrailingDecimals(bool show = true);

enum TrailingDecimalsFlag { USER_TRAILING_DECIMALS, SHOW_TRAILING_DECIMALS, HIDE_TRAILING_DECIMALS };

QString getFormattedCurrencyAmount(int64_t value, TrailingDecimalsFlag flag = USER_TRAILING_DECIMALS);

uint64_t getCurrencyDivisor();
const QString& getCurrencySymbol();
int getCurrencyDecimals();
uint64_t getCurrencyMax();
uint64_t getDefaultFee();

#ifdef SUPPORT_OLD_ADDRESS_VERSIONS
void setAddressVersions(bool useOld = false);
bool getUseOldAddressVersions();
#endif
