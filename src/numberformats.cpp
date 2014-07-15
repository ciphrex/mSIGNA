///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// numberformats.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#include "numberformats.h"

#include <sstream>
#include <stdint.h>
#include <stdexcept>

// Example for maxValue = 21000000 and maxDecimals = 8
// const QRegExp AMOUNT_REGEXP("((|0|[1-9]\\d{0,6}|1\\d{7}|20\\d{6})(\\.\\d{0,8})?|21000000(\\.0{0,8})?)");

std::string getDecimalRegExpString(uint64_t maxAmount, unsigned int maxDecimals)
{
    std::stringstream ss;

    if (maxAmount == 0)
    {
        ss << "(|0)(\\.";
        if (maxDecimals > 0) { ss << "0{0," << maxDecimals << "}"; }
        ss << ")?";
    }
    else
    {
        ss << "((|0";

        uint64_t i = maxAmount;
        unsigned int trailingZeros = 0;
        while (i % 10 == 0)
        {
            trailingZeros++;
            i /= 10;
        }

        if (trailingZeros > 0 && i > 9) { ss << "|[1-9]\\d{0," << trailingZeros << "}"; }

        do
        {
            if (i % 10 != 0) { ss << "|" << (i - 1) << "\\d{" << trailingZeros << "}"; }
            trailingZeros++;
            i /= 10;
        } while (i > 1);

        if (i == 1) { ss << "|\\d{" << trailingZeros << "}"; }
        ss << ")(\\.";

        if (maxDecimals > 0) { ss << "\\d{0," << maxDecimals << "}"; }
        ss << ")?|" << maxAmount << "(\\.";

        if (maxDecimals > 0) { ss << "0{0," << maxDecimals << "}"; }
        ss << ")?)";
    }

    return ss.str();
}
 
// Constrain input to valid values
uint64_t decimalStringToInteger(const std::string& decimalString, uint64_t maxAmount, uint64_t divisor, unsigned int maxDecimals)
{
    uint64_t whole = 0;
    uint64_t frac = 0;
    unsigned int decimals = 0;
    bool stateWhole = true;

    for (auto& c: decimalString) {
        if (stateWhole) {
            if (c == '.') {
                stateWhole = false;
                continue;
            }
            else if (whole > maxAmount || c < '0' || c > '9') {
                throw std::runtime_error("Invalid amount.");
            }
            whole *= 10;
            whole += (uint64_t)(c - '0');
        }
        else {
            decimals++;
            if (decimals > maxDecimals || c < '0' || c > '9') {
                throw std::runtime_error("Invalid amount.");
            }
            frac *= 10;
            frac += (uint64_t)(c - '0');
        }
    }
    if (frac > 0) {
        while (decimals < 8) {
            decimals++;
            frac *= 10;
        }
    }
    uint64_t value = (uint64_t)whole * divisor + frac;
    if (value > (maxAmount * divisor)) throw std::runtime_error("Invalid amount.");

    return value;
}
