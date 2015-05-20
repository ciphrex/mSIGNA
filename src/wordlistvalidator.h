///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// wordlistvalidator.h
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

#include <QValidator>
#include <QString>

class WordlistValidator : public QValidator
{
    Q_OBJECT

public:
    explicit WordlistValidator(QObject* parent = nullptr);
    QValidator::State validate(QString& input, int& pos) const;
    void fixup(QString& input) const;
};

