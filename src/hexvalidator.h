///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// hexvalidator.h
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

#include <QValidator>
#include <QString>

class HexValidator : public QValidator
{
    Q_OBJECT

public:
    enum Flags
    {
        ACCEPT_LOWER = 1,
        ACCEPT_UPPER = 2,
        ACCEPT_BOTH = 3
    };

    HexValidator(unsigned int minLength, unsigned int maxLength, int flags, QObject* parent = nullptr);
    QValidator::State validate(QString& input, int& pos) const;

private:
    unsigned int m_minLength;
    unsigned int m_maxLength;
    int m_flags;
};

