///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// currencyvalidator.h
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

#include <QValidator>
#include <QString>

class CurrencyValidator : public QValidator
{
    Q_OBJECT

public:
    explicit CurrencyValidator(uint64_t maxAmount, unsigned int maxDecimals, QObject* parent = nullptr);
    QValidator::State validate(QString& input, int& pos) const;

private:
    uint64_t m_maxAmount;
    unsigned int m_maxDecimals;
};

