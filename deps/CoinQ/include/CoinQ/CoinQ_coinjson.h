///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_coinjson.h 
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef _COINQ_COINJSON_H_
#define _COINQ_COINJSON_H_

#include "CoinQ_blocks.h"
#include "CoinQ_txs.h"

#include <json_spirit/json_spirit_reader_template.h>
#include <json_spirit/json_spirit_writer_template.h>
#include <json_spirit/json_spirit_utils.h>

#include <sstream>

namespace CoinQ {

json_spirit::Object getTxInJsonObject(const Coin::TxIn& txIn);
json_spirit::Object getTxOutJsonObject(const Coin::TxOut& txOut);
json_spirit::Object getTransactionJsonObject(const Coin::Transaction& tx);
json_spirit::Object getHeaderJsonObject(const Coin::CoinBlockHeader& header);
json_spirit::Object getBlockJsonObject(const Coin::CoinBlock& block, bool allFields = false);
json_spirit::Object getChainHeaderJsonObject(const ChainHeader& header);
json_spirit::Object getChainBlockJsonObject(const ChainBlock& block, bool allFields = false);
json_spirit::Object getChainTransactionJsonObject(const ChainTransaction& tx);

}

#endif // _COINQ_COINJSON_H_
