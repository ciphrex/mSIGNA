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

// disallow more than 8 decimals and amounts > 21 million
uint64_t btcStringToSatoshis(const std::string& btcString)
{
    uint32_t whole = 0;
    uint32_t frac = 0;

    unsigned int i = 0;
    bool stateWhole = true;
    for (auto& c: btcString) {
        i++;
        if (stateWhole) {
            if (c == '.') {
                stateWhole = false;
                i = 0;
                continue;
            }
            else if (i > 8 || c < '0' || c > '9') {
                throw std::runtime_error("Invalid amount.");
            }
            whole *= 10;
            whole += (uint32_t)(c - '0');
        }
        else {
            if (i > 8 || c < '0' || c > '9') {
                throw std::runtime_error("Invalid amount.");
            }
            frac *= 10;
            frac += (uint32_t)(c - '0');
        }
    }
    if (frac > 0) {
        while (i < 8) {
            i++;
            frac *= 10;
        }
    }
    uint64_t value = (uint64_t)whole * 100000000ull + frac;
    if (value > 2100000000000000ull) {
        throw std::runtime_error("Invalid amount.");
    }

    return value;
}
