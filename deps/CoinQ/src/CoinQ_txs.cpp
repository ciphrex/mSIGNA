///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_txs.cpp 
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "CoinQ_txs.h"

std::vector<ChainTxOut> CoinQSimpleInputPicker::pick() const
{
    std::vector<ChainTxOut> pickedTxOuts;

    uint64_t total = 0;
    std::vector<ChainTxOut> txOuts = txStore.getTxOuts(addressSet, ICoinQTxStore::Status::UNSPENT, minConf);
    for (auto& txOut: txOuts) {
        pickedTxOuts.push_back(txOut);
        total += txOut.value;
        if (total >= minValue) return pickedTxOuts;
    }

    throw std::runtime_error("Insufficient funds.");
}
