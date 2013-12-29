///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_txouts.h 
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef _COINQ_TXOUTS_H_
#define _COINQ_TXOUTS_H_

#include <CoinNodeData.h>

class ChainTxOut : public Coin::TxOut
{
public:
    enum Status {
        UNKNOWN = 0,
        UNSPENT,
        CLAIMED,
        SPENT
    };

    uchar_vector txHash;
    int index;

    uchar_vector spendTxHash;
    int spendIndex;

    ChainTxOut() : index(-1), spendIndex(-1) { }
    ChainTxOut(const Coin::TxOut& txOut, const uchar_vector& _txHash, int _index) : Coin::TxOut(txOut), txHash(_txHash), index(_index) { }

    void spend(const uchar_vector& _spendTxHash, int _spendIndex)
    {
        spendTxHash = _spendTxHash;
        spendIndex = _spendIndex;
    }
};

class ICoinQTxOutStore
{
public:
    virtual bool insert(const ChainTxOut& txOut, const uchar_vector& ) = 0;
    virtual bool spend(const uchar_vector& txHash, int index) = 0;
};

#endif // _COINQ_TXOUTS_H_
