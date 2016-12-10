///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// currencyvalidator.cpp
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "currencyvalidator.h"

#include <string>

// TODO: Get from locale settings
const char DECIMAL_POINT = '.';

CurrencyValidator::CurrencyValidator(uint64_t maxAmount, unsigned int maxDecimals, QObject* parent) :
    QValidator(parent), m_maxAmount(maxAmount), m_maxDecimals(maxDecimals)
{
}

QValidator::State CurrencyValidator::validate(QString& input, int& /*pos*/) const
{
    if (input.isEmpty()) return Intermediate;

    std::string amount = input.toStdString();
    uint64_t whole = 0;
    //uint64_t frac = 0; // We do not need to compute the value, just count decimals
    unsigned int decimals = 0;
    bool bFirst = true;
    bool bWhole = true;
    for (auto& c: amount)
    {
        if (bWhole)
        {
            if (c == DECIMAL_POINT)
            {
                bWhole = false;
            }
            else if (c >= '0' && c <= '9' && (whole > 0 || bFirst))
            {
                whole *= 10;
                whole += (uint64_t)(c - '0');
                if (whole > m_maxAmount) return Invalid;
            }
            else return Invalid;
        }
        else
        {
            decimals++;
            if (decimals > m_maxDecimals) return Invalid;

            if (c == '0' || (c >= '1' && c <= '9' && whole < m_maxAmount))
            {
                /*
                frac *= 10;
                frac += (uint64_t)(c - '0');
                */
            }
            else return Invalid;
        }

        bFirst = false;
    }

    return Acceptable; 
}

