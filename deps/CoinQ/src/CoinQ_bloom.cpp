///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_bloom.cpp
//
// Copyright (c) 2012-2013 Eric Lombrozo
//
// All Rights Reserved.

#include "CoinQ_bloom.h"
#include "typedefs.h"

#include <CoinNodeData.h>
#include <Base58Check.h>

#include <boost/program_options.hpp>

#include <fstream>

namespace po = boost::program_options;

using namespace CoinQ;

void BloomFilterTextFile::load(const std::string& filename, const char* base58chars)
{
    uint32_t nElements;
    double falsePositiveRate;
    uint32_t nTweak;
    uint8_t nFlags;
    std::vector<std::string> hexes;
    std::vector<std::string> outpoints;
    std::vector<std::string> base58s;

    po::options_description options("Options");
    options.add_options()
        ("elements", po::value<uint32_t>(&nElements)->default_value(1000), "number of elements")
        ("falsepositiverate", po::value<double>(&falsePositiveRate)->default_value(0.0001), "false positive rate")
        ("tweak", po::value<uint32_t>(&nTweak)->default_value(0), "tweak")
        ("flags", po::value<uint8_t>(&nFlags)->default_value(0), "flags")
        ("hex", po::value<std::vector<std::string>>(&hexes), "hex-encoded data")
        ("outpoint", po::value<std::vector<std::string>>(&outpoints), "outpoints")
        ("base58", po::value<std::vector<std::string>>(&base58s), "base58 encoded data")
    ;

    po::variables_map vm;
    std::ifstream f(filename);
    po::store(po::parse_config_file(f, options), vm);
    f.close();
    po::notify(vm);

    std::cout << "nElements: " << nElements << std::endl
              << "falsePositiveRate: " << falsePositiveRate << std::endl
              << "nTweak: " << nTweak << std::endl
              << "nFlags: " << (int)nFlags << std::endl;

    set(nElements, falsePositiveRate, nTweak, nFlags);

    std::cout << std::endl << "---hexes---" << std::endl;
    for (auto& hex: hexes) {
        uchar_vector data(hex);
        insert(data);
        std::cout << data.getHex() << std::endl;
    }
 
    std::cout << std::endl << "---outpoints---" << std::endl;
    for (auto& outpoint: outpoints) {
        if (outpoint.find_first_of(':') != 64 || outpoint.size() < 66) continue;
        uchar_vector outhash(outpoint.substr(0, 64));
        uint32_t outindex = strtoul(outpoint.substr(65).c_str(), NULL, 10);
        Coin::OutPoint point(outhash, outindex);
        uchar_vector data = point.getSerialized();
        insert(data);
        std::cout << point.toDelimited(":") << std::endl;
    }

    std::cout << std::endl << "---base58s---" << std::endl;
    for (auto& base58: base58s) {
        uchar_vector data;
        unsigned int version;
        if (!fromBase58Check(base58, data, version, base58chars)) continue;
        insert(data);
        std::cout << version << " " << data.getHex() << std::endl;
    }
}
