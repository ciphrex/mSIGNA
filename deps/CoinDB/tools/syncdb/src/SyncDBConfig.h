///////////////////////////////////////////////////////////////////////////////
//
// SyncDBConfig.h
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include <CoinDBConfig.h>

const double DEFAULT_FILTER_FALSE_POSITIVE_RATE = 0.001;
const uint32_t DEFAULT_FILTER_TWEAK = 0;
const uint8_t DEFAULT_FILTER_FLAGS = 0;

class SyncDBConfig : public CoinDBConfig
{
public:
    SyncDBConfig();

    bool parseParams(int argc, char* argv[]);

    double getFilterFalsePositiveRate() const { return m_filterFalsePositiveRate; }
    uint32_t getFilterTweak() const { return m_filterTweak; }
    uint8_t getFilterFlags() const { return m_filterFlags; }

protected:
    double m_filterFalsePositiveRate;
    uint32_t m_filterTweak;
    uint8_t m_filterFlags;
};

inline SyncDBConfig::SyncDBConfig() : CoinDBConfig()
{
    namespace po = boost::program_options;

    m_options.add_options()
        ("filterfpr", po::value<double>(&m_filterFalsePositiveRate), "filter false positive rate")
        ("filtertweak", po::value<uint32_t>(&m_filterTweak), "filter tweak")
        ("filterflags", po::value<uint8_t>(&m_filterFlags), "filter flags")
    ;
}

inline bool SyncDBConfig::parseParams(int argc, char* argv[])
{
    if (!CoinDBConfig::parseParams(argc, argv)) return false;

    if (!m_vm.count("filterfpr"))   { m_filterFalsePositiveRate = DEFAULT_FILTER_FALSE_POSITIVE_RATE; }
    if (!m_vm.count("filtertweak")) { m_filterTweak = DEFAULT_FILTER_TWEAK; }
    if (!m_vm.count("filterflags")) { m_filterFlags = DEFAULT_FILTER_FLAGS; }

    return true;
}

