///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// numberformats.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "numberformats.h"

#include <sstream>
#include <stdint.h>
#include <stdexcept>

using namespace std;

string getDecimalRegExpString(uint64_t maxAmount, unsigned int maxDecimals, char decimalSymbol)
{
    stringstream ss;
    ss << "(([1-9]\\d{0,6}|1\\d{7}|20\\d{6}|0|)(\\" << decimalSymbol << "\\d{0,8})?|21000000(\\" << decimalSymbol << "0{0,8})?)";
    return ss.str();
}
 
// Example for maxValue = 21000000 and maxDecimals = 8
// const QRegExp AMOUNT_REGEXP("((|0|[1-9]\\d{0,6}|1\\d{7}|20\\d{6})(\\.\\d{0,8})?|21000000(\\.0{0,8})?)");
/*
string nDigitsRegExpString(unsigned int n)
{
    if (n == 0) return "";
    if (n == 1) return "\\d";

    stringstream ss;
    ss << "\\d{" << n << "};
    return ss.str(); 
}

string zeroToNDigitsRegExp(unsigned n)
{
    if (n == 0) return "";

    stringstream ss;
    ss << "\\d{0," << n << "}";
    return ss.str();
}

string getDecimalRegExpString(uint64_t maxAmount, unsigned int maxDecimals)
{
    stringstream ssRegExp;
    if (maxAmount > 0 && maxDecimals > 0) { ssRegExp << "("; }
    ssRegExp << "(";

    uint64_t i = maxAmount;
    if (i > 1 && i % 10 != 0)
    {
        if (maxDecimals > 0) { i--; }
        ssRegExp << i << "|";
    }

    i = maxAmount / 10;
    unsigned int trailingDigits = 1;
    while (i > 10)
    {
        if (i % 10 != 0)
        {
            ssRegExp << (maxAmount - 1) << nDigitsRegExpString(trailingDigits) << "|";
        }
        i /= 10;
        trailingDigits++;
    }

    if (

    return ssRegExp.str();
}
*/
 
// Constrain input to valid values
uint64_t decimalStringToInteger(const string& decimalString, uint64_t maxAmount, uint64_t divisor, unsigned int maxDecimals)
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
        while (decimals < maxDecimals) {
            decimals++;
            frac *= 10;
        }
    }
    uint64_t value = (uint64_t)whole * divisor + frac;
    if (value > (maxAmount * divisor)) throw std::runtime_error("Invalid amount.");

    return value;
}
