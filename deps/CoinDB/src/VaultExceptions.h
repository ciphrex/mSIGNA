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
    virtual ~KeychainException() throw() { }
    const std::string& keychain_name() const { return keychain_name_; }

protected:
    explicit KeychainException(const std::string& what, const std::string& keychain_name) : std::runtime_error(what), keychain_name_(keychain_name) { }

    std::string keychain_name_;
};

class KeychainNotFoundException : public KeychainException
{
public:
    explicit KeychainNotFoundException(const std::string& keychain_name) : KeychainException("Keychain not found.", keychain_name) { }
};

class KeychainAlreadyExistsException : public KeychainException
{
public:
    explicit KeychainAlreadyExistsException(const std::string& keychain_name) : KeychainException("Keychain already exists.", keychain_name) { }
};

class KeychainChainCodeLockedException: public KeychainException
{
public:
    explicit KeychainChainCodeLockedException(const std::string& keychain_name) : KeychainException("Keychain chain code is locked.", keychain_name) { }
};

class KeychainChainCodeUnlockFailedException: public KeychainException
{
public:
    explicit KeychainChainCodeUnlockFailedException(const std::string& keychain_name) : KeychainException("Keychain chain code could not be unlocked.", keychain_name) { }
};

class KeychainPrivateKeyLockedException: public KeychainException
{
public:
    explicit KeychainPrivateKeyLockedException(const std::string& keychain_name) : KeychainException("Keychain private keys are locked.", keychain_name) { }
};

class KeychainPrivateKeyUnlockFailedException: public KeychainException
{
public:
    explicit KeychainPrivateKeyUnlockFailedException(const std::string& keychain_name) : KeychainException("Keychain private keys could not be unlocked.", keychain_name) { }
};

class KeychainIsNotPrivateException: public KeychainException
{
public:
    explicit KeychainIsNotPrivateException(const std::string& keychain_name) : KeychainException("Keychain is not private.", keychain_name) { }
};

class KeychainInvalidPrivateKeyException: public KeychainException
{
public:
    explicit KeychainInvalidPrivateKeyException(const std::string& keychain_name, const bytes_t& pubkey) : KeychainException("Invalid private key for public key.", keychain_name), pubkey_(pubkey) { }
    virtual ~KeychainInvalidPrivateKeyException() throw() { }
    const bytes_t& pubkey() const { return pubkey_; }

private:
    bytes_t pubkey_;
};

// ACCOUNT EXCEPTIONS
class AccountException : public std::runtime_error
{
public:
    virtual ~AccountException() throw() { }
    const std::string& account_name() const { return account_name_; }

protected:
    explicit AccountException(const std::string& what, const std::string& account_name) : std::runtime_error(what), account_name_(account_name) { }
    std::string account_name_;
};

class AccountNotFoundException : public AccountException
{
public:
    explicit AccountNotFoundException(const std::string& account_name) : AccountException("Account not found.", account_name) { }
};

class AccountAlreadyExistsException : public AccountException
{
public:
    explicit AccountAlreadyExistsException(const std::string& account_name) : AccountException("Account already exists.", account_name) { }
};

class AccountInsufficientFundsException : public AccountException
{
public:
    explicit AccountInsufficientFundsException(const std::string& account_name) : AccountException("Insufficient funds.", account_name) { }
};

class AccountCannotIssueChangeScriptException : public AccountException
{
public:
    explicit AccountCannotIssueChangeScriptException(const std::string& account_name) : AccountException("Account cannot issue change script", account_name) { }
};

// ACCOUNT BIN EXCEPTIONS
class AccountBinException : public std::runtime_error
{
public:
    virtual ~AccountBinException() throw() { }
    const std::string& account_name() const { return account_name_; }
    const std::string& bin_name() const { return bin_name_; }

protected:
    explicit AccountBinException(const std::string& what, const std::string& account_name, const std::string& bin_name) : std::runtime_error(what), account_name_(account_name), bin_name_(bin_name) { }
    std::string account_name_;
    std::string bin_name_;
};

class AccountBinNotFoundException : public AccountBinException
{
public:
    explicit AccountBinNotFoundException(const std::string& account_name, const std::string& bin_name) : AccountBinException("Account bin not found.", account_name, bin_name) { }
};

class AccountBinAlreadyExistsException : public AccountBinException
{
public:
    explicit AccountBinAlreadyExistsException(const std::string& account_name, const std::string& bin_name) : AccountBinException("Account bin already exists.", account_name, bin_name) { }
};

class AccountBinOutOfScriptsException : public AccountBinException
{
public:
    explicit AccountBinOutOfScriptsException(const std::string& account_name, const std::string& bin_name) : AccountBinException("Account bin out of scripts.", account_name, bin_name) { }
};

// TX EXCEPTIONS
class TxException : public std::runtime_error
{
public:
    virtual ~TxException() throw() { }
    const bytes_t& hash() const { return hash_; }

protected:
    explicit TxException(const std::string& what, const bytes_t& hash) : std::runtime_error(what), hash_(hash) { }
    bytes_t hash_;
};

class TxNotFoundException : public TxException
{
public:
    explicit TxNotFoundException(const bytes_t& hash = bytes_t()) : TxException("Transaction not found.", hash) { }
};

// BLOCK HEADER EXCEPTIONS
class BlockHeaderException : public std::runtime_error
{
public:
    virtual ~BlockHeaderException() throw() { }
    const bytes_t& hash() const { return hash_; }
    uint32_t height() const { return height_; }

protected:
    explicit BlockHeaderException(const std::string& what, const bytes_t& hash, uint32_t height) : std::runtime_error(what), hash_(hash), height_(height) { }
    explicit BlockHeaderException(const std::string& what, const bytes_t& hash) : std::runtime_error(what), hash_(hash), height_(0) { }
    explicit BlockHeaderException(const std::string& what, uint32_t height) : std::runtime_error(what), height_(height) { }
    bytes_t hash_;
    uint32_t height_;
};

class BlockHeaderNotFoundException : public BlockHeaderException
{
public:
    explicit BlockHeaderNotFoundException(const bytes_t& hash, uint32_t height) : BlockHeaderException("Block header not found.", hash, height) { }
    explicit BlockHeaderNotFoundException(const bytes_t& hash) : BlockHeaderException("Block header not found.", hash) { }
    explicit BlockHeaderNotFoundException(uint32_t height) : BlockHeaderException("Block header not found.", height) { }
};

// MERKLE BLOCK EXCEPTIONS
class MerkleBlockException : public std::runtime_error
{
public:
    virtual ~MerkleBlockException() throw() { }
    const bytes_t& hash() const { return hash_; }
    uint32_t height() const { return height_; }

protected:
    explicit MerkleBlockException(const std::string& what, const bytes_t& hash, uint32_t height) : std::runtime_error(what), hash_(hash), height_(height) { }
    bytes_t hash_;
    uint32_t height_;
};

class MerkleBlockInvalidException : public MerkleBlockException
{
public:
    explicit MerkleBlockInvalidException(const bytes_t& hash, uint32_t height) : MerkleBlockException("Merkle block is invalid.", hash, height) { }
};

// CHAIN CODE EXCEPTIONS
class ChainCodeException : public std::runtime_error
{
public:
    virtual ~ChainCodeException() throw() { }

protected:
    explicit ChainCodeException(const std::string& what) : std::runtime_error(what) { }
};

class ChainCodesAreLockedException : public ChainCodeException
{
public:
    explicit ChainCodesAreLockedException() : ChainCodeException("Chain codes are locked.") { }
};

class ChainCodeUnlockFailedForKeychainException : public ChainCodeException
{
public:
    explicit ChainCodeUnlockFailedForKeychainException(const std::string& keychain_name) : ChainCodeException("Chain code is locked for a keychain."), keychain_name_(keychain_name) { }
    virtual ~ChainCodeUnlockFailedForKeychainException() throw() { }
    const std::string& keychain_name() const { return keychain_name_; }

protected:
    std::string keychain_name_;
};

class ChainCodeSetUnlockKeyFailedForKeychainException : public ChainCodeException
{
public:
    explicit ChainCodeSetUnlockKeyFailedForKeychainException(const std::string& keychain_name) : ChainCodeException("Failed to set unlock key for keychain chain code."), keychain_name_(keychain_name) { }
    virtual ~ChainCodeSetUnlockKeyFailedForKeychainException() throw() { }
    const std::string& keychain_name() const { return keychain_name_; }

protected:
    std::string keychain_name_;
};

}
