///////////////////////////////////////////////////////////////////////////////
//
// SignatureInfo.h
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.
//

#pragma once

#include <CoinQ/CoinQ_typedefs.h>

#include <set>

namespace CoinDB
{

class SignatureInfo
{
public:
    typedef std::pair<std::string, bytes_t> keychain_info_t;
    SignatureInfo() : sigs_needed_(0) { }
    SignatureInfo(unsigned int sigs_needed, const std::set<keychain_info_t>& present_signers, const std::set<keychain_info_t>& missing_signers) :
        sigs_needed_(sigs_needed), present_signers_(present_signers), missing_signers_(missing_signers) { }

    unsigned int sigs_needed() const { return sigs_needed_; }
    const std::set<keychain_info_t>& present_signers() const { return present_signers_; }
    const std::set<keychain_info_t>& missing_signers() const { return missing_signers_; }

private:
    unsigned int sigs_needed_;
    std::set<keychain_info_t> present_signers_;
    std::set<keychain_info_t> missing_signers_;
};

}
