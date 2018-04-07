///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_coinparams.h
//
// Copyright (c) 2012-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include "CoinQ_exceptions.h"

#include <CoinCore/CoinNodeData.h>

#include <vector>
#include <map>

namespace CoinQ {

class CoinParams
{
public:
    CoinParams() { }

    CoinParams(
        uint32_t magic_bytes,
        uint32_t protocol_version,
        const char* default_port,
        uint8_t pay_to_pubkey_hash_version,
        uint8_t pay_to_script_hash_version,
        uint8_t old_pay_to_script_hash_version,
        uint8_t pay_to_witness_pubkey_hash_version,
        uint8_t pay_to_witness_script_hash_version,
        uint8_t privkey_version,
        const char* network_name,
        const char* url_prefix,
        uint64_t currency_divisor,
        const char* currency_symbol,
        uint64_t currency_max,
        uint64_t default_fee,
        Coin::hashfunc_t block_header_hash_function,
        Coin::hashfunc_t block_header_pow_hash_function,
        const Coin::CoinBlockHeader& genesis_block,
        bool segwit_enabled = false) :
    magic_bytes_(magic_bytes),
    protocol_version_(protocol_version),
    default_port_(default_port),
    pay_to_pubkey_hash_version_(pay_to_pubkey_hash_version),
    pay_to_script_hash_version_(pay_to_script_hash_version),
    old_pay_to_script_hash_version_(old_pay_to_script_hash_version),
    pay_to_witness_pubkey_hash_version_(pay_to_witness_pubkey_hash_version),
    pay_to_witness_script_hash_version_(pay_to_witness_script_hash_version),
    privkey_version_(privkey_version),
    network_name_(network_name),
    url_prefix_(url_prefix),
    currency_divisor_(currency_divisor),
    currency_symbol_(currency_symbol),
    currency_max_(currency_max),
    default_fee_(default_fee),
    block_header_hash_function_(block_header_hash_function),
    block_header_pow_hash_function_(block_header_pow_hash_function),
    genesis_block_(genesis_block),
    segwit_enabled_(segwit_enabled)
    {
        address_versions_[0] = pay_to_pubkey_hash_version_;
        address_versions_[1] = pay_to_script_hash_version_;

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
    uint8_t                         old_pay_to_script_hash_version() const { return old_pay_to_script_hash_version_; }
    uint8_t                         pay_to_witness_pubkey_hash_version() const { return pay_to_witness_pubkey_hash_version_; }
    uint8_t                         pay_to_witness_script_hash_version() const { return pay_to_witness_script_hash_version_; }
    uint8_t                         privkey_version() const { return privkey_version_; }
    const unsigned char*            address_versions() const { return address_versions_; }
    const char*                     network_name() const { return network_name_; }
    const char*                     url_prefix() const { return url_prefix_; }
    uint64_t                        currency_divisor() const { return currency_divisor_; }
    const char*                     currency_symbol() const { return currency_symbol_; }
    uint64_t                        currency_max() const { return currency_max_; }
    uint64_t                        default_fee() const { return default_fee_; }
    unsigned int                    currency_decimals() const { return currency_decimals_; }
    Coin::hashfunc_t                block_header_hash_function() const { return block_header_hash_function_; }
    Coin::hashfunc_t                block_header_pow_hash_function() const { return block_header_pow_hash_function_; }
    const Coin::CoinBlockHeader&    genesis_block() const { return genesis_block_; }
    bool                            segwit_enabled() const { return segwit_enabled_; }

private:
    uint32_t                magic_bytes_;
    uint32_t                protocol_version_;
    const char*             default_port_;
    uint8_t                 pay_to_pubkey_hash_version_;
    uint8_t                 pay_to_script_hash_version_;
    uint8_t                 old_pay_to_script_hash_version_;
    uint8_t                 pay_to_witness_pubkey_hash_version_;
    uint8_t                 pay_to_witness_script_hash_version_;
    uint8_t                 privkey_version_;
    unsigned char           address_versions_[2];
    const char*             network_name_;
    const char*             url_prefix_;
    uint64_t                currency_divisor_;
    const char*             currency_symbol_;
    uint64_t                currency_max_;
    uint64_t                default_fee_;
    unsigned int            currency_decimals_;
    Coin::hashfunc_t        block_header_hash_function_;
    Coin::hashfunc_t        block_header_pow_hash_function_;
    Coin::CoinBlockHeader   genesis_block_;
    bool                    segwit_enabled_;
};

typedef std::pair<std::string, const CoinParams&> NetworkPair;
typedef std::map<std::string, const CoinParams&> NetworkMap;

class NetworkSelector
{
public:
    explicit NetworkSelector(const std::string& network_name = "");

    std::vector<std::string> getNetworkNames() const;
    const CoinParams& getCoinParams(const std::string& network_name = "") const;

    void select(const std::string& network_name);
    const std::string& getSelected() const { return selected_; }

private:
    NetworkMap network_map_;
    std::string selected_;
};


// Individual accessors
const CoinParams& getBitcoinParams();
const CoinParams& getTestnet3Params();
const CoinParams& getLitecoinParams();
const CoinParams& getLtcTestnet4Params();
const CoinParams& getQuarkcoinParams();

}
