////////////////////////////////////////////////////////////////////////////////
//
// TransactionSigner.h
//
// Copyright (c) 2011-2012 Eric Lombrozo
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

#ifndef TRANSACTIONSIGNER_H
#define TRANSACTIONSIGNER_H

#include "CoinKey.h"
#include "CoinNodeData.h"

class BasicInput
{
public:
    string_secure walletImport; /// TODO: remove explicit walletImport and privKey...use coinKey instead.
    uchar_vector_secure privKey;
    CoinKey coinKey;
    Coin::OutPoint outPoint;
    uint32_t sequence;

    BasicInput() { }

    BasicInput(const string_secure& _walletImport, const Coin::OutPoint& _outPoint, uint32_t _sequence = 0xffffffff)
        : walletImport(_walletImport), outPoint(_outPoint), sequence(_sequence) { }
    BasicInput(const uchar_vector_secure& _privKey, const Coin::OutPoint& _outPoint, uint32_t _sequence = 0xffffffff)
        : privKey(_privKey), outPoint(_outPoint), sequence(_sequence) { }
    BasicInput(const CoinKey& coinKey, const Coin::OutPoint& outPoint, uint32_t sequence = 0xffffffff)
    {
        this->coinKey = coinKey;
        this->walletImport = coinKey.getWalletImport();
        this->privKey = coinKey.getPrivateKey();
        this->outPoint = outPoint;
        this->sequence = sequence;
    }
    BasicInput(const BasicInput& basicInput)
    {
        this->walletImport = basicInput.walletImport;
        this->privKey = basicInput.privKey;
        this->outPoint = basicInput.outPoint;
        this->sequence = basicInput.sequence;
    }
};

class BasicOutput
{
public:
    std::string toAddress;
    uint64_t amount;

    BasicOutput() { }

    BasicOutput(const std::string& _toAddress, uint64_t _amount)
        : toAddress(_toAddress), amount(_amount) { }
    BasicOutput(const BasicOutput& basicOutput)
        : toAddress(basicOutput.toAddress), amount(basicOutput.amount) { }
};

Coin::Transaction CreateTransaction(
    const std::vector<BasicInput>& claims,
    const std::vector<BasicOutput>& payments,
    uint32_t lockTime = 0,
    uint8_t addressVersion = 0x00
);

void SignTransaction(const std::vector<BasicInput>& claims, Coin::Transaction& tx);

#endif // TRANSACTIONSIGNER_H
