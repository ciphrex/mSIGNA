///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// numberformats.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include "numberformats.h"

#include <string>
#include <stdint.h>
#include <stdexcept>

// Constrain input to valid values
uint64_t valueStringToInteger(const std::string& valueString, uint64_t maxValue, uint64_t divisor, unsigned int maxDecimals)
{
    uint64_t maxWhole = maxValue / divisor;

    uint64_t whole = 0;
    uint64_t frac = 0;
    unsigned int decimals = 0;
    bool stateWhole = true;

    for (auto& c: valueString) {
        if (stateWhole) {
            if (c == '.') {
                stateWhole = false;
                continue;
            }
            else if (whole > maxWhole || c < '0' || c > '9') {
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
    if (value > maxValue) throw std::runtime_error("Invalid amount.");

    return value;
}
