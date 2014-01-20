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

namespace CoinQ {

class CoinParams
{
public:
    CoinParams(
        uint32_t magic_bytes,
        uint32_t protocol_version,
        const char* default_port,
        uint8_t pay_to_pubkey_hash_version,
        uint8_t pay_to_script_hash_version,
        const char* network_name,
        const char* url_prefix,
        Coin::hashfunc_t block_header_hash_function,
        Coin::hashfunc_t block_header_pow_hash_function,
        const Coin::CoinBlockHeader& genesis_block) :
    magic_bytes_(magic_bytes),
    protocol_version_(protocol_version),
    default_port_(default_port),
    pay_to_pubkey_hash_version_(pay_to_pubkey_hash_version),
    pay_to_script_hash_version_(pay_to_script_hash_version),
    network_name_(network_name),
    url_prefix_(url_prefix),
    block_header_hash_function_(block_header_hash_function),
    block_header_pow_hash_function_(block_header_pow_hash_function),
    genesis_block_(genesis_block) { }

    uint32_t                        magic_bytes() const { return magic_bytes_; }
    uint32_t                        protocol_version() const { return protocol_version_; }
    const char*                     default_port() const { return default_port_; }
    uint8_t                         pay_to_pubkey_hash_version() const { return pay_to_pubkey_hash_version_; }
    uint8_t                         pay_to_script_hash_version() const { return pay_to_script_hash_version_; }
    const char*                     network_name() const { return network_name_; }
    const char*                     url_prefix() const { return url_prefix_; }
    Coin::hashfunc_t                block_header_hash_function() const { return block_header_hash_function_; }
    Coin::hashfunc_t                block_header_pow_hash_function() const { return block_header_pow_hash_function_; }
    const Coin::CoinBlockHeader&    genesis_block() const { return genesis_block_; }

private:
    uint32_t                magic_bytes_;
    uint32_t                protocol_version_;
    const char*             default_port_;
    uint8_t                 pay_to_pubkey_hash_version_;
    uint8_t                 pay_to_script_hash_version_;
    const char*             network_name_;
    const char*             url_prefix_;
    Coin::hashfunc_t        block_header_hash_function_;
    Coin::hashfunc_t        block_header_pow_hash_function_;
    Coin::CoinBlockHeader   genesis_block_;
};

inline CoinParams getBitcoinParams()
{
    return CoinParams(0xd9b4bef9ul, 70001, "8333", 0x00, 0x05, "Bitcoin", "bitcoin", &sha256_2, &sha256_2,
        Coin::CoinBlockHeader(1, 1231006505, 486604799, 2083236893, uchar_vector(32, 0), uchar_vector("4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b")));
}

inline CoinParams getLitecoinParams()
{
    return CoinParams(0xdbb6c0fbul, 70002, "9333", 0x30, 0x05, "Litecoin", "litecoin", &sha256_2, &scrypt_1024_1_1_256,
        Coin::CoinBlockHeader(1, 1317972665, 0x1e0ffff0, 2084524493, uchar_vector(32, 0), uchar_vector("97ddfbbae6be97fd6cdf3e7ca13232a3afff2353e29badfab7f73011edd4ced9")));
}

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
    const Coin::hashfunc_t      BLOCK_HEADER_POW_HASH_FUNCTION = &sha256_2;
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
    const Coin::hashfunc_t      BLOCK_HEADER_HASH_FUNCTION = &sha256_2;
    const Coin::hashfunc_t      BLOCK_HEADER_POW_HASH_FUNCTION = &scrypt;
    const Coin::CoinBlockHeader GENESIS_BLOCK(1, 1317972665, 0x1e0ffff0, 2084524493, bytes_t(32, 0), uchar_vector("97ddfbbae6be97fd6cdf3e7ca13232a3afff2353e29badfab7f73011edd4ced9"));
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
    const Coin::hashfunc_t      BLOCK_HEADER_POW_HASH_FUNCTION = &hash9;
}
#endif

// The generic namespace COIN_PARAMS can be used instead by defining the appropriate constant
namespace COIN_PARAMS
{
#if defined(USE_BITCOIN)
    const uint32_t              MAGIC_BYTES                     = BITCOIN_PARAMS::MAGIC_BYTES;
    const uint32_t              PROTOCOL_VERSION                = BITCOIN_PARAMS::PROTOCOL_VERSION;
    const char*                 DEFAULT_PORT                    = BITCOIN_PARAMS::DEFAULT_PORT;
    const uint8_t               PAY_TO_PUBKEY_HASH_VERSION      = BITCOIN_PARAMS::PAY_TO_PUBKEY_HASH_VERSION;
    const uint8_t               PAY_TO_SCRIPT_HASH_VERSION      = BITCOIN_PARAMS::PAY_TO_SCRIPT_HASH_VERSION;
    const char*                 NETWORK_NAME                    = BITCOIN_PARAMS::NETWORK_NAME;
    const Coin::hashfunc_t      BLOCK_HEADER_HASH_FUNCTION      = BITCOIN_PARAMS::BLOCK_HEADER_HASH_FUNCTION;
    const Coin::hashfunc_t      BLOCK_HEADER_POW_HASH_FUNCTION  = BITCOIN_PARAMS::BLOCK_HEADER_POW_HASH_FUNCTION;
    const Coin::CoinBlockHeader GENESIS_BLOCK(BITCOIN_PARAMS::GENESIS_BLOCK);
#elif defined(USE_LITECOIN)
    const uint32_t              MAGIC_BYTES                     = LITECOIN_PARAMS::MAGIC_BYTES;
    const uint32_t              PROTOCOL_VERSION                = LITECOIN_PARAMS::PROTOCOL_VERSION;
    const char*                 DEFAULT_PORT                    = LITECOIN_PARAMS::DEFAULT_PORT;
    const uint8_t               PAY_TO_PUBKEY_HASH_VERSION      = LITECOIN_PARAMS::PAY_TO_PUBKEY_HASH_VERSION;
    const uint8_t               PAY_TO_SCRIPT_HASH_VERSION      = LITECOIN_PARAMS::PAY_TO_SCRIPT_HASH_VERSION;
    const char*                 NETWORK_NAME                    = LITECOIN_PARAMS::NETWORK_NAME;
    const Coin::hashfunc_t      BLOCK_HEADER_HASH_FUNCTION      = LITECOIN_PARAMS::BLOCK_HEADER_HASH_FUNCTION;
    const Coin::hashfunc_t      BLOCK_HEADER_POW_HASH_FUNCTION  = LITECOIN_PARAMS::BLOCK_HEADER_POW_HASH_FUNCTION;
    const Coin::CoinBlockHeader GENESIS_BLOCK(LITECOIN_PARAMS::GENESIS_BLOCK);
#elif defined(USE_QUARKCOIN)
    const uint32_t              MAGIC_BYTES                     = QUARKCOIN_PARAMS::MAGIC_BYTES;
    const uint32_t              PROTOCOL_VERSION                = QUARKCOIN_PARAMS::PROTOCOL_VERSION;
    const char*                 DEFAULT_PORT                    = QUARKCOIN_PARAMS::DEFAULT_PORT;
    const uint8_t               PAY_TO_PUBKEY_HASH_VERSION      = QUARKCOIN_PARAMS::PAY_TO_PUBKEY_HASH_VERSION;
    const uint8_t               PAY_TO_SCRIPT_HASH_VERSION      = QUARKCOIN_PARAMS::PAY_TO_SCRIPT_HASH_VERSION;
    const char*                 NETWORK_NAME                    = QUARKCOIN_PARAMS::NETWORK_NAME;
    const Coin::hashfunc_t      BLOCK_HEADER_HASH_FUNCTION      = QUARKCOIN_PARAMS::BLOCK_HEADER_HASH_FUNCTION;
    const Coin::hashfunc_t      BLOCK_HEADER_POW_HASH_FUNCTION  = QUARKCOIN_PARAMS::BLOCK_HEADER_HASH_FUNCTION;
//    const Coin::CoinBlockHeader GENESIS_BLOCK(QUARKCOIN_PARAMS::GENESIS_BLOCK);
#endif
}

/*
inline CoinParams getDefaultCoinParams()
{
    return CoinParams(
        COIN_PARAMS::MAGIC_BYTES,
        COIN_PARAMS::PROTOCOL_VERSION,
        COIN_PARAMS::DEFAULT_PORT,
        COIN_PARAMS::PAY_TO_PUBKEY_HASH_VERSION,
        COIN_PARAMS::PAY_TO_SCRIPT_HASH_VERSION,
        COIN_PARAMS::NETWORK_NAME,
        COIN_PARAMS::BLOCK_HEADER_HASH_FUNCTION,
        COIN_PARAMS::BLOCK_HEADER_POW_HASH_FUNCTION,
        COIN_PARAMS::GENESIS_BLOCK);
};
*/

}

#endif // COINQ_COINPARAMS_H
