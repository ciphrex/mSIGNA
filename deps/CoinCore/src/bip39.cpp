////////////////////////////////////////////////////////////////////////////////
//
// bip39.cpp
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

#include "bip39.h"
#include "hash.h"
#include "wordlists/english.h"

#include <map>

using namespace Coin::BIP39;
using namespace std;

int Coin::BIP39::minWordLen()
{
    return MIN_WORDLEN;
}

int Coin::BIP39::maxWordLen()
{
    return MAX_WORDLEN;
}

secure_string_t Coin::BIP39::toWordlist(const secure_bytes_t& data, Language language)
{
    const char** WORDLIST;
    switch (language)
    {
    case ENGLISH:
        WORDLIST = ENGLISH_WORDLIST;
        break;

    default:
        throw runtime_error("Coin::BIP39::toWordList() - unsupported language.");
    }

    int nDataBits = data.size() << 3; // * 8
    if (nDataBits % 32) throw runtime_error("Coin::BIP39::toWordList() - data has invalid length.");

    int nChecksumBits = nDataBits / 32;
    if (nChecksumBits > 256) throw runtime_error("Coin::BIP39::toWordList() - data is too long.");

    int nChecksumBytes = (nChecksumBits + 7) / 8;
    secure_bytes_t checksum = sha256(data);
    secure_bytes_t bytes(data);
    for (int i = 0; i < nChecksumBytes; i++) { bytes.push_back(checksum[i]); }

    int nBits = nDataBits + nChecksumBits; // will be a multiple of 11

    secure_string_t rval;
    bool addSpace = false;
    int iBit = 0;
    int iWord = 0;
    for (auto byte: bytes)
    {
        for (int k = 7; k >= 0; k--)
        {
            iWord <<= 1;
            iWord += (byte >> k) & 1;
            iBit++;
            if (iBit % 11 == 0)
            {
                if (addSpace)   { rval += " "; }
                else            { addSpace = true; }

                rval += WORDLIST[iWord];
                iWord = 0;
            }

            if (iBit == nBits) break;
        }
    }

    return rval;
}

secure_bytes_t Coin::BIP39::fromWordlist(const secure_string_t& list, Language language)
{
    const char** WORDLIST;
    int WORDCOUNT;    
    switch (language)
    {
    case ENGLISH:
        WORDLIST = ENGLISH_WORDLIST;
        WORDCOUNT = ENGLISH_WORDCOUNT;
        break;

    default:
        throw runtime_error("Coin::BIP39::fromWordList() - unsupported language.");
    }

    typedef map<secure_string_t, int> wordmap_t;
    wordmap_t words;
    for (int i = 0; i < WORDCOUNT; i++) { words[WORDLIST[i]] = i; }

    secure_string_t word;
    secure_ints_t wordValues;
    for (auto c: list)
    {
        if (c == ' ')
        {
            if (word.empty()) continue;

            auto it = words.find(word);
            if (it == words.end()) throw InvalidWordException(word);

            wordValues.push_back(it->second);
            word.clear();
            continue;
        }

        if (c >= 'A' && c <= 'Z') { c += 'a' - 'A'; }
        if (c < 'a' || c > 'z') throw InvalidCharacterException(c);

        word += c;
    }

    if (!word.empty())
    {
        auto it = words.find(word);
        if (it == words.end()) throw InvalidWordException(word);

        wordValues.push_back(it->second);
    }

    if (wordValues.size() < 3 || wordValues.size() > 3*256 || wordValues.size() % 3) throw InvalidWordlistLength();
    int nChecksumBits = wordValues.size() / 3;
    int nDataBytes = nChecksumBits << 2; // * 4

    int iBit = 0;
    unsigned char byte = 0;
    secure_bytes_t bytes;
    for (auto wordValue: wordValues)
    {
        for (int k = 10; k >= 0; k--)
        {
            byte <<= 1;
            byte += (char)(wordValue >> k) & 1;
            iBit++;
            if (iBit == 8)
            {
                bytes.push_back(byte);
                byte = 0;
                iBit = 0;
            }
        }
    }

    if (iBit) { bytes.push_back(byte << (8 - iBit)); }

    secure_bytes_t rval(bytes.begin(), bytes.begin() + nDataBytes);
    secure_bytes_t checksum1(bytes.begin() + nDataBytes, bytes.end());
    secure_bytes_t checksum2 = sha256(rval);

    // TODO: Clean up this implementation
    iBit = 0;
    for (int i = 0; i < (int)checksum1.size(); i++)
    {
        char byte1 = checksum1[i];
        char byte2 = checksum2[i];
        for (int k = 7; k >= 0; k--)
        {
            if (((byte1 >> k) & 1) != ((byte2 >> k) & 1)) throw InvalidChecksum();
            iBit++;
            if (iBit == nChecksumBits) break;
        } 
    }

    return rval;
}
