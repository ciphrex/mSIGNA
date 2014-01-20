///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// numberformats.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef COINVAULT_NUMBERFORMATS_H
#define COINVAULT_NUMBERFORMATS_H

#include <QRegExp>
#include <string>
#include <stdint.h>
#include <stdexcept>

// TODO: allow setting via coinparams
const QRegExp AMOUNT_REGEXP("(([1-9]\\d{0,6}|1\\d{7}|20\\d{6}|0|)(\\.\\d{0,8})?|21000000(\\.0{0,8})?)");

// disallow more than 8 decimals and amounts > 21 million
uint64_t btcStringToSatoshis(const std::string& btcString);

#endif // COINVAULT_NUMBERFORMATS_H
