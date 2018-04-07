///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_txs.h 
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#ifndef _COINQ_TXS_H_
#define _COINQ_TXS_H_

#include "CoinQ_blocks.h"
#include "CoinQ_keys.h"
#include "CoinQ_signals.h"

#include <CoinCore/CoinNodeData.h>

#include <map>

class ChainTransaction : public Coin::Transaction
{
public:
    ChainHeader blockHeader;
    int index;
    // TODO: Merkle path

    ChainTransaction() : Coin::Transaction(), index(-1) { }
    ChainTransaction(const Coin::Transaction& tx) : Coin::Transaction(tx), index(-1) { }
    ChainTransaction(const Coin::Transaction& tx, const ChainHeader& _blockHeader, int _index) : Coin::Transaction(tx), blockHeader(_blockHeader), index(_index) { }
};

class ChainTxOut : public Coin::TxOut
{
public:
    uchar_vector txHash;
    int index;
    bool bSpent;

    ChainTxOut() : Coin::TxOut(), index(-1), bSpent(false) { }
    ChainTxOut(const Coin::TxOut& txOut) : Coin::TxOut(txOut), index(-1), bSpent(false) { }
    ChainTxOut(const Coin::TxOut& txOut, const uchar_vector& _txHash, int _index, bool _bSpent = false)
        : Coin::TxOut(txOut), txHash(_txHash), index(_index), bSpent(_bSpent) { }

//    std::string toJson() const;
};
/*
std::string ChainTxOut::toJson() const
{
    std::stringstream ss;
    ss << "{\"amount\":" << (value / 100000000ull) << "." << (value % 100000000ull) << ",\"amount_int\":" << value << ",\"script\":\"" << scriptPubKey.getHex()
       << "\",\"address\":\"" << getAddress() << "\",\"tx_hash\":\"" << txHash.getHex() << "\",\"index\":" << index << ",\"spent\":" << (bSpent ? "true" : "false") << "}";
    return ss.str();
}
*/
typedef std::function<void(const ChainTransaction&)>    chain_tx_slot_t;


class ICoinQTxStore
{
public:
    virtual bool insert(const ChainTransaction& tx) = 0;
    virtual bool deleteTx(const uchar_vector& txHash) = 0;
    virtual bool unconfirm(const uchar_vector& blockHash) = 0;

    virtual bool hasTx(const uchar_vector& txHash) const = 0;
    virtual bool getTx(const uchar_vector& txHash, ChainTransaction& tx) const = 0; 

    virtual uchar_vector getBestBlockHash(const uchar_vector& txHash) const = 0;
    virtual int getBestBlockHeight(const uchar_vector& txHash) const = 0;
    virtual std::vector<ChainTransaction> getConfirmedTxs(int minHeight, int maxHeight) const = 0;
    virtual std::vector<ChainTransaction> getUnconfirmedTxs() const = 0;

    enum Status { UNSPENT = 1, SPENT = 2, BOTH = 3 };
    virtual std::vector<ChainTxOut> getTxOuts(const CoinQ::Keys::AddressSet& addressSet, Status status, int minConf) const = 0;

    virtual void setBestHeight(int height) = 0;
    virtual int  getBestHeight() const = 0;

    virtual void subscribeInsert(chain_tx_slot_t slot) = 0;
    virtual void subscribeDelete(chain_tx_slot_t slot) = 0;
    virtual void subscribeConfirm(chain_tx_slot_t slot) = 0;
    virtual void subscribeUnconfirm(chain_tx_slot_t slot) = 0;
    virtual void subscribeNewBestHeight(std::function<void(int)> slot) = 0;
};

class ICoinQInputPicker
{
    virtual std::vector<ChainTxOut> pick() const = 0;
};

class CoinQSimpleInputPicker : public ICoinQInputPicker
{
private:
    const ICoinQTxStore& txStore;
    const CoinQ::Keys::AddressSet& addressSet;
    uint64_t minValue;
    int minConf;

public:
    CoinQSimpleInputPicker(const ICoinQTxStore& _txStore, const CoinQ::Keys::AddressSet& _addressSet, uint64_t _minValue, int _minConf = 1)
        : txStore(_txStore), addressSet(_addressSet), minValue(_minValue), minConf(_minConf) { }

    std::vector<ChainTxOut> pick() const;
};

#endif // _COINQ_TXS_H_
