///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_coinjson.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "CoinQ_coinjson.h"

namespace CoinQ {

json_spirit::Object getTxInJsonObject(const Coin::TxIn& txIn)
{
    json_spirit::Object obj;
    obj.push_back(json_spirit::Pair("outhash", txIn.getOutpointHash().getHex()));
    obj.push_back(json_spirit::Pair("outindex", (uint64_t)txIn.getOutpointIndex()));
    obj.push_back(json_spirit::Pair("script", txIn.scriptSig.getHex()));
    obj.push_back(json_spirit::Pair("address", txIn.getAddress()));
    obj.push_back(json_spirit::Pair("sequence", (uint64_t)txIn.sequence));
    return obj;
}

json_spirit::Object getTxOutJsonObject(const Coin::TxOut& txOut)
{
    json_spirit::Object obj;
    std::stringstream ss;
    ss << txOut.value;
    obj.push_back(json_spirit::Pair("amount_int", ss.str()));
    obj.push_back(json_spirit::Pair("script", txOut.scriptPubKey.getHex()));
    obj.push_back(json_spirit::Pair("address", txOut.getAddress()));
    return obj;
}

json_spirit::Object getTransactionJsonObject(const Coin::Transaction& tx)
{
    json_spirit::Object obj;
    obj.push_back(json_spirit::Pair("hash", tx.getHashLittleEndian().getHex()));
    obj.push_back(json_spirit::Pair("version", (uint64_t)tx.version));
    json_spirit::Array inputs;
    for (unsigned int i = 0; i < tx.inputs.size(); i++) {
        inputs.push_back(getTxInJsonObject(tx.inputs[i]));
    }
    obj.push_back(json_spirit::Pair("inputs", inputs));
    json_spirit::Array outputs;
    for (unsigned int i = 0; i < tx.outputs.size(); i++) {
        outputs.push_back(getTxOutJsonObject(tx.outputs[i]));
    }
    obj.push_back(json_spirit::Pair("outputs", outputs));
    obj.push_back(json_spirit::Pair("locktime", (uint64_t)tx.lockTime));
    return obj;
}

json_spirit::Object getHeaderJsonObject(const Coin::CoinBlockHeader& header)
{
    json_spirit::Object obj;
    obj.push_back(json_spirit::Pair("hash", header.getHashLittleEndian().getHex()));
    obj.push_back(json_spirit::Pair("version", (uint64_t)header.version));
    obj.push_back(json_spirit::Pair("prevblockhash", header.prevBlockHash.getHex()));
    obj.push_back(json_spirit::Pair("merkleroot", header.merkleRoot.getHex()));
    obj.push_back(json_spirit::Pair("timestamp", (uint64_t)header.timestamp));
    obj.push_back(json_spirit::Pair("bits", (uint64_t)header.bits));
    obj.push_back(json_spirit::Pair("nonce", (uint64_t)header.nonce));
    return obj;
}

json_spirit::Object getBlockJsonObject(const Coin::CoinBlock& block, bool allFields)
{
    json_spirit::Object obj = getHeaderJsonObject(block.blockHeader);
    if (allFields) {
        obj.push_back(json_spirit::Pair("size", block.getSize()));
        obj.push_back(json_spirit::Pair("sent", block.getTotalSent()));
    }
    json_spirit::Array txs;
    for (unsigned int i = 0; i < block.txs.size(); i++) {
        txs.push_back(getTransactionJsonObject(block.txs[i]));
    }
    obj.push_back(json_spirit::Pair("txs", txs));
    return obj;
}

json_spirit::Object getChainHeaderJsonObject(const ChainHeader& header)
{
    json_spirit::Object obj = getHeaderJsonObject(header);
    obj.push_back(json_spirit::Pair("inbestchain", header.inBestChain));
    obj.push_back(json_spirit::Pair("height", header.height));
    obj.push_back(json_spirit::Pair("chainwork", header.chainWork.getDec()));
    return obj;
}

json_spirit::Object getChainBlockJsonObject(const ChainBlock& block, bool allFields)
{
    json_spirit::Object obj = getBlockJsonObject(block, allFields);
    obj.push_back(json_spirit::Pair("inbestchain", block.inBestChain));
    obj.push_back(json_spirit::Pair("height", block.height));
    obj.push_back(json_spirit::Pair("chainwork", block.chainWork.getDec()));
    return obj;
}

json_spirit::Object getChainTransactionJsonObject(const ChainTransaction& tx)
{
    json_spirit::Object obj = getTransactionJsonObject(tx);
    if (tx.blockHeader.height > -1) {
        json_spirit::Object header = getChainHeaderJsonObject(tx.blockHeader);
        obj.push_back(json_spirit::Pair("header", header));
        obj.push_back(json_spirit::Pair("index", tx.index));
    }
    return obj;
}

}
