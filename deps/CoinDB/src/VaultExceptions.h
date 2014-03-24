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

class KeychainException : public std::runtime_error
{
public:
    const std::string& keychain_name() const { return keychain_name_; }

protected:
    KeychainException(const std::string& what, const std::string& keychain_name) : std::runtime_error(what), keychain_name_(keychain_name) { }

    std::string keychain_name_;
};

class KeychainNotFoundException : public KeychainException
{
public:
    KeychainNotFoundException(const std::string& keychain_name) : KeychainException("Keychain not found.", keychain_name) { }
};

class AccountNotFoundException : public std::runtime_error
{
public:
    AccountNotFoundException(const std::string& account_name) : std::runtime_error("Account not found."), account_name_(account_name) { }

    const std::string& account_name() const { return account_name_; }

private:
    std::string account_name_;
};

class AccountBinException : public std::runtime_error
{
public:
    const std::string& account_name() const { return account_name_; }
    const std::string& bin_name() const { return bin_name_; }

protected:
    AccountBinException(const std::string& what) : std::runtime_error(what) { }

    std::string account_name_;
    std::string bin_name_;
};

class AccountBinNotFoundException : public AccountBinException
{
public:
    AccountBinNotFoundException(const std::string& account_name, const std::string& bin_name) : AccountBinException("Account bin not found.")
    {
        account_name_ = account_name;
        bin_name_ = bin_name;
    }
};

class AccountBinAlreadyExistsException : public AccountBinException
{
public:
    AccountBinAlreadyExistsException(const std::string& account_name, const std::string& bin_name) : AccountBinException("Account bin already exists.")
    {
        account_name_ = account_name;
        bin_name_ = bin_name;
    }
};

class InsufficientFundsException : public std::runtime_error
{
public:
    InsufficientFundsException() : std::runtime_error("Insufficient funds.") { }
};

}
