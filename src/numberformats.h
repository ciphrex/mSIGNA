///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// numberformats.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include <string>
#include <stdint.h>
#include <stdexcept>

std::string getDecimalRegExpString(uint64_t maxAmount, unsigned int maxDecimals, char decimalSymbol = '.');

// Constrain input to valid value
uint64_t decimalStringToInteger(const std::string& decimalString, uint64_t maxAmount, uint64_t divisor, unsigned int maxDecimals);

