///////////////////////////////////////////////////////////////////////////////
//
// SigningRequest.h
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include <CoinQ/CoinQ_typedefs.h>

#include <set>

namespace CoinDB
{

class SigningRequest
{
public:
    typedef std::pair<std::string, bytes_t> keychain_info_t;
    SigningRequest() : sigs_needed_(0) { }
    SigningRequest(const bytes_t& hash, unsigned int sigs_needed, const std::set<keychain_info_t>& keychain_info, const bytes_t& rawtx = bytes_t()) :
        hash_(hash), sigs_needed_(sigs_needed), keychain_info_(keychain_info), rawtx_(rawtx) { }

    const bytes_t& hash() const { return hash_; }
    unsigned int sigs_needed() const { return sigs_needed_; }
    const std::set<keychain_info_t>& keychain_info() const { return keychain_info_; }
    const bytes_t& rawtx() const { return rawtx_; }

private:
    bytes_t hash_;
    unsigned int sigs_needed_;
    std::set<keychain_info_t> keychain_info_;
    bytes_t rawtx_;
};

}
