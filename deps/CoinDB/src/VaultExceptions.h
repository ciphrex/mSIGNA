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

// KEYCHAIN EXCEPTIONS
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

class KeychainAlreadyExistsException : public KeychainException
{
public:
    KeychainAlreadyExistsException(const std::string& keychain_name) : KeychainException("Keychain already exists.", keychain_name) { }
};

class KeychainPrivateKeyUnlockFailedException: public KeychainException
{
public:
    KeychainPrivateKeyUnlockFailedException(const std::string& keychain_name) : KeychainException("Failed to unlock private keys.", keychain_name) { }
};

// ACCOUNT EXCEPTIONS
class AccountException : public std::runtime_error
{
public:
    const std::string& account_name() const { return account_name_; }

protected:
    AccountException(const std::string& what, const std::string& account_name) : std::runtime_error(what), account_name_(account_name) { }

    std::string account_name_;
};

class AccountNotFoundException : public AccountException
{
public:
    AccountNotFoundException(const std::string& account_name) : AccountException("Account not found.", account_name) { }
};

class AccountAlreadyExistsException : public AccountException
{
public:
    AccountAlreadyExistsException(const std::string& account_name) : AccountException("Account already exists.", account_name) { }
};

class AccountInsufficientFundsException : public AccountException
{
public:
    AccountInsufficientFundsException(const std::string& account_name) : AccountException("Insufficient funds.", account_name) { }
};

class AccountChainCodeLockedException : public AccountException
{
public:
    AccountChainCodeLockedException(const std::string& account_name, const std::set<std::string>& locked_keychains) : AccountException("Chain code is locked.", account_name), locked_keychains_(locked_keychains) { }

    const std::set<std::string>& locked_keychains() const { return locked_keychains_; }

protected:
    std::set<std::string> locked_keychains_;
};

// ACCOUNT BIN EXCEPTIONS
class AccountBinException : public std::runtime_error
{
public:
    const std::string& account_name() const { return account_name_; }
    const std::string& bin_name() const { return bin_name_; }

protected:
    AccountBinException(const std::string& what, const std::string& account_name, const std::string& bin_name) : std::runtime_error(what), account_name_(account_name), bin_name_(bin_name) { }

    std::string account_name_;
    std::string bin_name_;
};

class AccountBinNotFoundException : public AccountBinException
{
public:
    AccountBinNotFoundException(const std::string& account_name, const std::string& bin_name) : AccountBinException("Account bin not found.", account_name, bin_name) { }
};

class AccountBinAlreadyExistsException : public AccountBinException
{
public:
    AccountBinAlreadyExistsException(const std::string& account_name, const std::string& bin_name) : AccountBinException("Account bin already exists.", account_name, bin_name) { }
};

class AccountBinOutOfScriptsException : public AccountBinException
{
public:
    AccountBinOutOfScriptsException(const std::string& account_name, const std::string& bin_name) : AccountBinException("Account bin out of scripts.", account_name, bin_name) { }
};

// TX EXCEPTIONS
class TxException : public std::runtime_error
{
public:
    const bytes_t& hash() const { return hash_; }

protected:
    TxException(const std::string& what, const bytes_t& hash) : std::runtime_error(what), hash_(hash) { }

    bytes_t hash_;
};

class TxNotFoundException : public TxException
{
public:
    TxNotFoundException(const bytes_t& hash) : TxException("Transaction not found.", hash) { }
};

}
