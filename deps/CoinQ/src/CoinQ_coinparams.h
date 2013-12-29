///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_coinparams.h
//
// Copyright (c) 2012-2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef COINQ_COINPARAMS_H
#define COINQ_COINPARAMS_H

#include <CoinNodeData.h>

#if defined(USE_BITCOIN)
namespace BITCOIN_PARAMS
{
    const uint32_t              MAGIC_BYTES = 0xd9b4bef9ul;
    const uint32_t              PROTOCOL_VERSION = 70001;
    const char*                 DEFAULT_PORT = "8333";
    const uint8_t               PAY_TO_PUBKEY_HASH_VERSION = 0x00;
    const uint8_t               PAY_TO_SCRIPT_HASH_VERSION = 0x05;
    const char*                 NETWORK_NAME = "Bitcoin";
    const Coin::hashfunc_t      BLOCK_HEADER_HASH_FUNCTION = &sha256_2;
    const Coin::CoinBlockHeader GENESIS_BLOCK(1, 1231006505, 486604799, 2083236893, bytes_t(32, 0), uchar_vector("4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));
}
#endif

#if defined(USE_LITECOIN)
namespace LITECOIN_PARAMS
{
    const uint32_t              MAGIC_BYTES = 0xdbb6c0fbul;
    const uint32_t              PROTOCOL_VERSION = 60002;
    const char*                 DEFAULT_PORT = "9333";
    const uint8_t               PAY_TO_PUBKEY_HASH_VERSION = 0x30;
    const uint8_t               PAY_TO_SCRIPT_HASH_VERSION = 0x05;
    const char*                 NETWORK_NAME = "Litecoin";
    // TODO: Add correct block header hash function and genesis block.
}
#endif

#if defined(USE_QUARKCOIN)
namespace QUARKCOIN_PARAMS
{
    const uint32_t              MAGIC_BYTES = 0xdd03a5feul;
    const uint32_t              PROTOCOL_VERSION = 70001;
    const char*                 DEFAULT_PORT = "11973";
    const char*                 DEFAULT_PORT_TESTNET = "21973";
    const uint8_t               PAY_TO_PUBKEY_HASH_VERSION = 0x3a;
    const uint8_t               PAY_TO_SCRIPT_HASH_VERSION = 0x09;
    const char*                 NETWORK_NAME = "Quarkcoin";
    const Coin::hashfunc_t      BLOCK_HEADER_HASH_FUNCTION = &hash9;
}
#endif

// The generic namespace COIN_PARAMS can be used instead by defining the appropriate constant
namespace COIN_PARAMS
{
#if defined(USE_BITCOIN)
    const uint32_t              MAGIC_BYTES                 = BITCOIN_PARAMS::MAGIC_BYTES;
    const uint32_t              PROTOCOL_VERSION            = BITCOIN_PARAMS::PROTOCOL_VERSION;
    const char*                 DEFAULT_PORT                = BITCOIN_PARAMS::DEFAULT_PORT;
    const uint8_t               PAY_TO_PUBKEY_HASH_VERSION  = BITCOIN_PARAMS::PAY_TO_PUBKEY_HASH_VERSION;
    const uint8_t               PAY_TO_SCRIPT_HASH_VERSION  = BITCOIN_PARAMS::PAY_TO_SCRIPT_HASH_VERSION;
    const char*                 NETWORK_NAME                = BITCOIN_PARAMS::NETWORK_NAME;
    const Coin::hashfunc_t      BLOCK_HEADER_HASH_FUNCTION  = BITCOIN_PARAMS::BLOCK_HEADER_HASH_FUNCTION;
    const Coin::CoinBlockHeader GENESIS_BLOCK(BITCOIN_PARAMS::GENESIS_BLOCK);
#elif defined(USE_LITECOIN)
    const uint32_t              MAGIC_BYTES                 = LITECOIN_PARAMS::MAGIC_BYTES;
    const uint32_t              PROTOCOL_VERSION            = LITECOIN_PARAMS::PROTOCOL_VERSION;
    const char*                 DEFAULT_PORT                = LITECOIN_PARAMS::DEFAULT_PORT;
    const uint8_t               PAY_TO_PUBKEY_HASH_VERSION  = LITECOIN_PARAMS::PAY_TO_PUBKEY_HASH_VERSION;
    const uint8_t               PAY_TO_SCRIPT_HASH_VERSION  = LITECOIN_PARAMS::PAY_TO_SCRIPT_HASH_VERSION;
    const char*                 NETWORK_NAME                = LITECOIN_PARAMS::NETWORK_NAME;
//    const Coin::hashfunc_t      BLOCK_HEADER_HASH_FUNCTION  = LITECOIN_PARAMS::BLOCK_HEADER_HASH_FUNCTION;
//    const Coin::CoinBlockHeader GENESIS_BLOCK(LITECOIN_PARAMS::GENESIS_BLOCK);
#elif defined(USE_QUARKCOIN)
    const uint32_t              MAGIC_BYTES                 = QUARKCOIN_PARAMS::MAGIC_BYTES;
    const uint32_t              PROTOCOL_VERSION            = QUARKCOIN_PARAMS::PROTOCOL_VERSION;
    const char*                 DEFAULT_PORT                = QUARKCOIN_PARAMS::DEFAULT_PORT;
    const uint8_t               PAY_TO_PUBKEY_HASH_VERSION  = QUARKCOIN_PARAMS::PAY_TO_PUBKEY_HASH_VERSION;
    const uint8_t               PAY_TO_SCRIPT_HASH_VERSION  = QUARKCOIN_PARAMS::PAY_TO_SCRIPT_HASH_VERSION;
    const char*                 NETWORK_NAME                = QUARKCOIN_PARAMS::NETWORK_NAME;
    const Coin::hashfunc_t      BLOCK_HEADER_HASH_FUNCTION  = QUARKCOIN_PARAMS::BLOCK_HEADER_HASH_FUNCTION;
//    const Coin::CoinBlockHeader GENESIS_BLOCK(QUARKCOIN_PARAMS::GENESIS_BLOCK);
#endif
}

#endif // COINQ_COINPARAMS_H
