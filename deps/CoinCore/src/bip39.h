////////////////////////////////////////////////////////////////////////////////
//
// bip39.h
//
// Copyright (c) 2015 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
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
