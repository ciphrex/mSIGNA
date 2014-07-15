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

// Constrain input to valid value
uint64_t valueStringToInteger(const std::string& valueString, uint64_t maxValue, uint64_t divisor, unsigned int maxDecimals);

#endif // COINVAULT_NUMBERFORMATS_H
