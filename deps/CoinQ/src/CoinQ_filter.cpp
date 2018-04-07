////////////////////////////////////////////////////////////////////////////////
//
// CoinQ_filter.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "CoinQ_filter.h"

bool CoinQBestChainBlockFilter::push(const Coin::CoinBlock& block) const
{
    if (!pBlockTree) return false;

    uchar_vector hash = block.hash();
    if (!pBlockTree->hasHeader(hash)) return false;

    const ChainHeader& header = pBlockTree->getHeader(hash);
    if (!header.inBestChain) return false;

    ChainBlock chainBlock(block, true, header.height, header.chainWork);
    notify(chainBlock);
    return true;
}



bool CoinQTxAddressFilter::push(const ChainTransaction& tx) const
{
    if (!pAddressSet) return false;

    if (mode & Mode::RECEIVE) {
        for (unsigned int i = 0; i < tx.outputs.size(); i++) {
            if (pAddressSet->getAddresses().count(tx.outputs[i].getAddress()) != 0) {
                notify(tx);
                return true;
            }
        }
    }

    if (mode & Mode::SEND) {
        for (unsigned int i = 0; i < tx.inputs.size(); i++) {
            if (pAddressSet->getAddresses().count(tx.inputs[i].getAddress()) != 0) {
                notify(tx);
                return true;
            }
        }
    }

    return false;
}

