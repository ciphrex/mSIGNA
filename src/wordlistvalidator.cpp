///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// wordlistvalidator.cpp
//
// Copyright (c) 2015 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "wordlistvalidator.h"

#include <string>

WordlistValidator::WordlistValidator(int minlen, int maxlen, QObject* parent) :
    QValidator(parent), minlen_(minlen), maxlen_(maxlen)
{
}

QValidator::State WordlistValidator::validate(QString& input, int& pos) const
{
    if (input.isEmpty()) return Intermediate;
    input = input.toLower();

    // Backspace over spaces if we're not at the end.
    if ((pos < input.size()) && (input.size() + 1 == lastInput.size()) && (lastInput[pos] == ' '))
    {
        QString temp = input.left(pos) + ' ' + input.right(input.size() - pos);
        if (temp == lastInput) input = lastInput;
    }

    QString newInput;
    int newPos = pos;
    
    std::string wordlist = input.toStdString();
    bool final = true;
    QString lastWord;
    int i = 0;
    for (auto c: wordlist)
    {
        i++;
        if (c == ' ')
        {
            if (lastWord.isEmpty())
            {
                // Strip extra spaces
                if (newPos > newInput.size()) newPos--;
            }
            else if (lastWord.size() >= minlen_)
            {
                newInput += lastWord + ' ';
                lastWord.clear();
            }
            else if (i < (int)wordlist.size() && newPos > newInput.size())
            {
                // Remove words that are too short.
                newPos = newInput.size() - 1;
                lastWord.clear();
            }
            else
            {
                // Disallow adding a space that makes a word too short.
                return Invalid;
            }
        }
        else if (c >= 'a' && c <= 'z' && lastWord.size() < maxlen_)
        {
            lastWord += c;
        }
        else
        {
            return Invalid;
        }
    }

    newInput += lastWord;

    if (lastWord.size() < minlen_) final = false;

    input = newInput;
    pos = newPos;

    lastInput = input;
    
    return final ? Acceptable : Intermediate;
}

