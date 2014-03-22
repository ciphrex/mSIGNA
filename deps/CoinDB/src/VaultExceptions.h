///////////////////////////////////////////////////////////////////////////////
//
// VaultExceptions.h
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.
//

#pragma once

#include <stdexcept>

namespace CoinDB
{

class AccountNotFoundException : public std::runtime_error
{
public:
    AccountNotFoundException(const std::string& account_name = "") : std::runtime_error("Account not found."), account_name_(account_name)  { }

    const std::string& account_name() const { return account_name_; }

private:
    std::string account_name_;
};

}
