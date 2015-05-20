////////////////////////////////////////////////////////////////////////////////
//
// bip39.h
//
// Copyright (c) 2015 Eric Lombrozo
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "hash.h"
#include "typedefs.h"

#include <stdexcept>

namespace Coin
{
namespace BIP39
{

enum Language { ENGLISH };

class InvalidWordException : public std::runtime_error
{
public:
    explicit InvalidWordException(const std::string& word) : std::runtime_error("Invalid word."), word_(word) { }

    const std::string& word() const { return word_; }

private:
    std::string word_;
};

class InvalidCharacterException : public std::runtime_error
{
public:
    explicit InvalidCharacterException(char c) : std::runtime_error("Invalid character."), c_(c) { }

    char c() const { return c_; }

private:
    char c_;
};

class InvalidWordlistLength : public std::runtime_error
{
public:
    explicit InvalidWordlistLength() : std::runtime_error("Invalid wordlist length.") { }
};

class InvalidChecksum : public std::runtime_error
{
public:
    explicit InvalidChecksum() : std::runtime_error("Invalid checksum.") { }
};

int minWordLen();
int maxWordLen();

std::string toWordlist(const secure_bytes_t& data, Language language = ENGLISH);
secure_bytes_t fromWordlist(const std::string& list, Language language = ENGLISH);

}
}
