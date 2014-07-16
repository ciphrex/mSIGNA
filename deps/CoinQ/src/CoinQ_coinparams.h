///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_coinparams.h
//
// Copyright (c) 2012-2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

#include <CoinCore/CoinNodeData.h>

#include <vector>
#include <map>

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
        uint64_t currency_divisor,
        const char* currency_symbol,
        uint64_t currency_max,
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
    currency_divisor_(currency_divisor),
    currency_symbol_(currency_symbol),
    currency_max_(currency_max),
    block_header_hash_function_(block_header_hash_function),
    block_header_pow_hash_function_(block_header_pow_hash_function),
    genesis_block_(genesis_block)
    {
        currency_decimals_ = 0;
        uint64_t i = currency_divisor_;
        while (i > 1)
        {
            currency_decimals_++;
            i /= 10;
        }
    }

    uint32_t                        magic_bytes() const { return magic_bytes_; }
    uint32_t                        protocol_version() const { return protocol_version_; }
    const char*                     default_port() const { return default_port_; }
    uint8_t                         pay_to_pubkey_hash_version() const { return pay_to_pubkey_hash_version_; }
    uint8_t                         pay_to_script_hash_version() const { return pay_to_script_hash_version_; }
    const char*                     network_name() const { return network_name_; }
    const char*                     url_prefix() const { return url_prefix_; }
    uint64_t                        currency_divisor() const { return currency_divisor_; }
    const char*                     currency_symbol() const { return currency_symbol_; }
    uint64_t                        currency_max() const { return currency_max_; }
    unsigned int                    currency_decimals() const { return currency_decimals_; }
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
    uint64_t                currency_divisor_;
    const char*             currency_symbol_;
    uint64_t                currency_max_;
    unsigned int            currency_decimals_;
    Coin::hashfunc_t        block_header_hash_function_;
    Coin::hashfunc_t        block_header_pow_hash_function_;
    Coin::CoinBlockHeader   genesis_block_;
};

typedef std::pair<std::string, const CoinParams&> NetworkPair;
typedef std::map<std::string, const CoinParams&> NetworkMap;

class NetworkSelector
{
public:
    NetworkSelector();

    std::vector<std::string> getNetworkNames() const;
    const CoinParams& getCoinParams(const std::string& network_name) const;

private:
    NetworkMap network_map_;
};


// Individual accessors
const CoinParams& getBitcoinParams();
const CoinParams& getTestnet3Params();
const CoinParams& getLitecoinParams();
const CoinParams& getQuarkcoinParams();

}
