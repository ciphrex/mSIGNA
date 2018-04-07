///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// hexvalidator.cpp
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "hexvalidator.h"

#include <string>
#include <stdexcept>

using namespace std;

HexValidator::HexValidator(unsigned int minLength, unsigned int maxLength, int flags, QObject* parent) :
    QValidator(parent), m_minLength(minLength), m_maxLength(maxLength)
{
    if (minLength > maxLength) throw runtime_error("HexValidator - invalid length range.");

    m_flags = 0; 
    if (flags & ACCEPT_LOWER) { m_flags |= ACCEPT_LOWER; }
    if (flags & ACCEPT_UPPER) { m_flags |= ACCEPT_UPPER; }
    if (m_flags == 0) throw std::runtime_error("HexValidator - invalid flags.");
}

QValidator::State HexValidator::validate(QString& input, int& /*pos*/) const
{
    if (input.size() > (int)m_maxLength) return Invalid;
    if (input.size() < (int)m_minLength) return Intermediate;

    string amount = input.toStdString();
    for (auto& c: amount)
    {
        if ((m_flags & ACCEPT_LOWER) && c >= 'a' && c <= 'f') continue;
        if ((m_flags & ACCEPT_UPPER) && c >= 'A' && c <= 'F') continue;
        if (c >= '0' && c <= '9') continue;
        return Invalid;
    }

    return Acceptable; 
}

