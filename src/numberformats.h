///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// numberformats.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

#include <string>
#include <stdint.h>
#include <stdexcept>

#include <QRegExp>
const QRegExp AMOUNT_REGEXP("(([1-9]\\d{0,6}|1\\d{7}|20\\d{6}|0|)(\\.\\d{0,8})?|21000000(\\.0{0,8})?)");
//std::string getDecimalRegExpString(uint64_t maxAmount, unsigned int maxDecimals);

// Constrain input to valid value
uint64_t decimalStringToInteger(const std::string& decimalString, uint64_t maxAmount, uint64_t divisor, unsigned int maxDecimals);

