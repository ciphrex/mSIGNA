///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_coinparams.cpp
//
// Copyright (c) 2012-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "CoinQ_coinparams.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>

using namespace std;

namespace CoinQ
{

NetworkSelector::NetworkSelector(const std::string& network_name)
{
    network_map_.insert(NetworkPair("bitcoin", getBitcoinParams()));
    network_map_.insert(NetworkPair("testnet3", getTestnet3Params()));
    network_map_.insert(NetworkPair("litecoin", getLitecoinParams()));
    network_map_.insert(NetworkPair("ltctestnet4", getLtcTestnet4Params()));
    network_map_.insert(NetworkPair("quarkcoin", getQuarkcoinParams()));

    if (!network_name.empty()) { select(network_name); }
}

vector<string> NetworkSelector::getNetworkNames() const
{
    vector<string> names;
    for (const auto& item: network_map_) { names.push_back(item.first); }
    return names;
}

const CoinParams& NetworkSelector::getCoinParams(const std::string& network_name) const
{
    string lower_network_name;

    if (network_name.empty())   { lower_network_name = selected_;    }
    else                        { lower_network_name = network_name; }

    if (lower_network_name.empty()) throw NetworkSelectorNoNetworkSelectedException();

    transform(lower_network_name.begin(), lower_network_name.end(), lower_network_name.begin(), ::tolower);

    const auto& it = network_map_.find(lower_network_name);
    if (it == network_map_.end()) throw NetworkSelectorNetworkNotRecognizedException(lower_network_name);

    return it->second;
}

void NetworkSelector::select(const std::string& network_name)
{
    string lower_network_name(network_name);
    transform(lower_network_name.begin(), lower_network_name.end(), lower_network_name.begin(), ::tolower);
    if (!network_map_.count(lower_network_name)) throw NetworkSelectorNetworkNotRecognizedException(lower_network_name);

    selected_ = lower_network_name;
}


// Coins can be added here
const CoinParams bitcoinParams(
    0xd9b4bef9ul,
    70001,
    "8333",
    0,
    5,
    5,
    4,
    10,
    128,
    "Bitcoin",
    "bitcoin",
    100000000,
    "BTC",
    21000000,
    100000,
    &sha256_2,
    &sha256_2,
    Coin::CoinBlockHeader(
        1,
        1231006505,
        486604799,
        2083236893,
        uchar_vector(32, 0),
        uchar_vector("4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b")
    ),
    true
);
const CoinParams& getBitcoinParams() { return bitcoinParams; }

const CoinParams testnet3Params(
    0x0709110bul,
    70001,
    "18333",
    0x6f,
    0xc4,
    0xc4,
    6,
    40,
    239,
    "Testnet3",
    "testnet3",
    100000000,
    "tBTC",
    21000000,
    0,
    &sha256_2,
    &sha256_2,
    Coin::CoinBlockHeader(
        1,
        1296688602,
        486604799,
        414098458,
        uchar_vector(32, 0),
        uchar_vector("4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b")
    ),
    true
);
const CoinParams& getTestnet3Params() { return testnet3Params; }

const CoinParams litecoinParams(
    0xdbb6c0fbul,
    70002,
    "9333",
    48,
    50,
    5,
    4,
    10,
    176,
    "Litecoin",
    "litecoin",
    100000000,
    "LTC",
    84000000,
    100000,
    &sha256_2,
    &scrypt_1024_1_1_256,
    Coin::CoinBlockHeader(
        1,
        1317972665,
        0x1e0ffff0,
        2084524493,
        uchar_vector(32, 0),
        uchar_vector("97ddfbbae6be97fd6cdf3e7ca13232a3afff2353e29badfab7f73011edd4ced9")
    ),
    true
);
const CoinParams& getLitecoinParams() { return litecoinParams; }

const CoinParams ltcTestnet4Params(
    0xf1c8d2fdul,
    70002,
    "19335",
    111,
    58,
    196,
    4,
    10,
    239,
    "LtcTestnet4",
    "ltctestnet4",
    100000000,
    "tLTC",
    84000000,
    100000,
    &sha256_2,
    &scrypt_1024_1_1_256,
    Coin::CoinBlockHeader(
        1,
        1486949366,
        0x1e0ffff0,
        293345,
        uchar_vector(32, 0),
        uchar_vector("97ddfbbae6be97fd6cdf3e7ca13232a3afff2353e29badfab7f73011edd4ced9")
    )
);
const CoinParams& getLtcTestnet4Params() { return ltcTestnet4Params; }

const CoinParams quarkcoinParams(
    0xdd03a5feul,
    70001,
    "11973",
    0x3a,
    0x09,
    0x09,
    4,
    10,
    128,
    "Quarkcoin",
    "quarkcoin",
    100000,
    "QRK",
    0xffffffffffffffffull / 100000,
    0,
    &hash9,
    &hash9,
    Coin::CoinBlockHeader(
        112,
        1374408079,
        0x1e0fffff,
        12058113,
        uchar_vector(32, 0),
        uchar_vector("868b2fb28cb1a0b881480cc85eb207e29e6ae75cdd6d26688ed34c2d2d23c776")
    )
);
const CoinParams& getQuarkcoinParams() { return quarkcoinParams; }

/*
const CoinParams bluecoinParams(
    0xaaabf5fe, // main.cpp     pchmessageStart
    71001,      // protocol.h   PROTOCOL_VERSION
    "27104",    // 
    0x1a,
    0x1c,       // base58.h     CBitcoinAddress
    "Bluecoin",
    "bluecoin",
    1000000,    // util.h       COIN
    "BLU",
    0xffffffffffffffffull / 100000, // TODO: fix this
    &hash9,     // main.h       CBlock::GetHash() - there's a time cutoff before which scrypt_1024_1_1_256 is used
    &hash9,     //              The time cutoff is X11_CUTOFF_TIME = 1403395200 defined in main.h
    Coin::CoinBlockHeader(
        1,
        1398205046,
        0x1e0fffff, // main.cpp     bnProofOfWorkLimit
        380217,
        uchar(32, 0),
        uchar_vector("9867907dca9d8dd34cb5424091f5ef374e36407ad196fb28f6cd628fdf4e6220") // main.cpp  LoadBlockIndex
    )
); 
const CoinParams& getBluecoinParams() { return bluecoinParams; }
*/
    
}
