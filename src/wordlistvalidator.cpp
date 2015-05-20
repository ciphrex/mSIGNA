///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// wordlistvalidator.cpp
//
// Copyright (c) 2015 Eric Lombrozo
//
// All Rights Reserved.

#include "wordlistvalidator.h"

WordlistValidator::WordlistValidator(QObject* parent) :
    QValidator(parent)
{
}

void WordlistValidator::fixup(QString& input) const
{
    input = input.toLower();
}

QValidator::State WordlistValidator::validate(QString& input, int& /*pos*/) const
{
    if (input.isEmpty()) return Intermediate;
    return Acceptable; 
}

