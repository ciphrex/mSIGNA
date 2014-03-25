///////////////////////////////////////////////////////////////////////////////
//
// SigningRequest.h
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.
//

#pragma once

#include <CoinQ_typedefs.h>

#include <set>

namespace CoinDB
{

class SigningRequest
{
public:
    SigningRequest(unsigned int sigs_needed, const std::set<bytes_t>& keychain_hashes, const bytes_t& rawtx = bytes_t()) :
        sigs_needed_(sigs_needed), keychain_hashes_(keychain_hashes), rawtx_(rawtx) { }

    unsigned int sigs_needed() const { return sigs_needed_; }
    const std::set<bytes_t>& keychain_hashes() const { return keychain_hashes_; }
    const bytes_t& rawtx() const { return rawtx_; }

private:
    unsigned int sigs_needed_;
    std::set<bytes_t> keychain_hashes_;
    bytes_t rawtx_;
};

}
