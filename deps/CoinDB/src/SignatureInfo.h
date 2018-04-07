///////////////////////////////////////////////////////////////////////////////
//
// SignatureInfo.h
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

class SigningKeychain
{
public:
    SigningKeychain(const std::string& name, const bytes_t& hash, bool hasSigned, bool isPrivate = false) :
        name_(name), hash_(hash), hasSigned_(hasSigned), isPrivate_(isPrivate) { }

    const std::string&  name() const        { return name_; }
    const bytes_t&      hash() const        { return hash_; }
    bool                hasSigned() const   { return hasSigned_; }
    bool                isPrivate() const   { return isPrivate_; }

private:
    std::string name_;
    bytes_t     hash_;
    bool        hasSigned_;
    bool        isPrivate_;
};

struct SigningKeychainCompare
{
    bool operator() (const SigningKeychain& lhs, const SigningKeychain& rhs) const
    {
        if (lhs.hash() == rhs.hash()) return false;
        return lhs.name() < rhs.name();
    }
};

typedef std::set<SigningKeychain, SigningKeychainCompare> SigningKeychainSet;

class SignatureInfo
{
public:
    SignatureInfo() : sigsNeeded_(0) { }
    SignatureInfo(unsigned int sigsNeeded, const SigningKeychainSet& signingKeychains) :
        sigsNeeded_(sigsNeeded), signingKeychains_(signingKeychains) { }

    unsigned int                sigsNeeded() const          { return sigsNeeded_; }
    const SigningKeychainSet&   signingKeychains() const    { return signingKeychains_; }

private:
    unsigned int sigsNeeded_;
    SigningKeychainSet signingKeychains_;
};

}
