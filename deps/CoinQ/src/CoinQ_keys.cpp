////////////////////////////////////////////////////////////////////////////////
//
// CoinQ_keys.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "CoinQ_keys.h"

#include <CoinCore/Base58Check.h>

#include <fstream>

#include <boost/filesystem.hpp>

using namespace CoinQ::Keys;

void AddressSet::insert(const std::string& address)
{
    // TODO: validate address
    addresses.insert(address);
}

void AddressSet::insert(const std::vector<uchar_vector>& hashes, unsigned char version, const char* base58chars)
{
    for (auto& hash: hashes) {
        addresses.insert(toBase58Check(hash, version, base58chars));
    }
}

void AddressSet::loadFromFile(const std::string& filename)
{
    boost::filesystem::path p(filename);
    if (!boost::filesystem::exists(p)) {
        throw std::runtime_error("File not found.");
    }

    if (!boost::filesystem::is_regular_file(p)) {
        throw std::runtime_error("Invalid file type.");
    }

    std::string address;
#ifndef _WIN32
    std::ifstream fs(p.native());
#else
    std::ifstream fs(filename);
#endif
    getline(fs, address);
    while (fs.good()) {
#ifdef COINQKEYS_LOG
        std::cout << "Added address: " << address << std::endl;
#endif
        insert(address);
        getline(fs, address);
    }
    if (fs.bad()) {
        throw std::runtime_error("Error reading from file.");
    }
}

void AddressSet::flushToFile(const std::string& filename)
{
    boost::filesystem::path swapfile(filename + ".swp");
    if (boost::filesystem::exists(swapfile)) {
        throw std::runtime_error("Swapfile already exists.");
    }

    {
#ifndef _WIN32
        std::ofstream fs(swapfile.native());
#else
        std::ofstream fs(filename + ".swp");
#endif
        for (auto&  address: addresses) {
            if (fs.bad()) {
                throw std::runtime_error("Error writing to file.");
            }
            fs << address << std::endl;
        }
    }

    boost::system::error_code ec;
    boost::filesystem::path p(filename);
    boost::filesystem::rename(swapfile, p, ec);
    if (!!ec) {
        throw std::runtime_error(ec.message());
    }
}

