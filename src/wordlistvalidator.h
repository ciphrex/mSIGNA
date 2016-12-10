///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// wordlistvalidator.h
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include <QValidator>
#include <QString>

class WordlistValidator : public QValidator
{
    Q_OBJECT

public:
    explicit WordlistValidator(int minlen, int maxlen, QObject* parent = nullptr);
    QValidator::State validate(QString& input, int& pos) const;

private:
    mutable QString lastInput;
    int minlen_;
    int maxlen_;
};

