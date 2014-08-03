///////////////////////////////////////////////////////////////////////////////
//
// VaultExceptions.h
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.
//

#pragma once

#include <stdutils/customerror.h>

namespace CoinDB
{

enum ErrorCodes
{
    // Keychain errors
    KEYCHAIN_NOT_FOUND = 101,
    KEYCHAIN_ALREADY_EXISTS,
    KEYCHAIN_CHAIN_CODE_LOCKED,
    KEYCHAIN_CHAIN_CODE_UNLOCK_FAILED,
    KEYCHAIN_PRIVATE_KEY_LOCKED,
    KEYCHAIN_PRIVATE_KEY_UNLOCK_FAILED,
    KEYCHAIN_IS_NOT_PRIVATE,
    KEYCHAIN_INVALID_PRIVATE_KEY,

    // Account errors
    ACCOUNT_NOT_FOUND = 201,
    ACCOUNT_ALREADY_EXISTS,
    ACCOUNT_INSUFFICIENT_FUNDS,
    ACCOUNT_CANNOT_ISSUE_CHANGE_SCRIPT,

    // Account bin errors
    ACCOUNTBIN_NOT_FOUND = 301,
    ACCOUNTBIN_ALREADY_EXISTS,
    ACCOUNTBIN_OUT_OF_SCRIPTS,

    // Tx errors
    TX_NOT_FOUND = 401,
    TX_INVALID_INPUTS,
    TX_OUTPUTS_EXCEED_INPUTS,
    TX_OUTPUT_NOT_FOUND,

    // Block header errors
    BLOCKHEADER_NOT_FOUND = 501,

    // Merkle block errors
    MERKLEBLOCK_INVALID = 602,

    // Chain code errors
    CHAINCODE_LOCKED = 702,
    CHAINCODE_UNLOCK_FAILED,
    CHAINCODE_SET_UNLOCK_KEY_FAILED
};

// KEYCHAIN EXCEPTIONS
class KeychainException : public stdutils::custom_error
{
public:
    virtual ~KeychainException() throw() { }
    const std::string& keychain_name() const { return keychain_name_; }

protected:
    explicit KeychainException(const std::string& what, int code, const std::string& keychain_name) : stdutils::custom_error(what, code), keychain_name_(keychain_name) { }

    std::string keychain_name_;
};

class KeychainNotFoundException : public KeychainException
{
public:
    explicit KeychainNotFoundException(const std::string& keychain_name) : KeychainException("Keychain not found.", KEYCHAIN_NOT_FOUND, keychain_name) { }
};

class KeychainAlreadyExistsException : public KeychainException
{
public:
    explicit KeychainAlreadyExistsException(const std::string& keychain_name) : KeychainException("Keychain already exists.", KEYCHAIN_ALREADY_EXISTS, keychain_name) { }
};

class KeychainChainCodeLockedException: public KeychainException
{
public:
    explicit KeychainChainCodeLockedException(const std::string& keychain_name) : KeychainException("Keychain chain code is locked.", KEYCHAIN_CHAIN_CODE_LOCKED, keychain_name) { }
};

class KeychainChainCodeUnlockFailedException: public KeychainException
{
public:
    explicit KeychainChainCodeUnlockFailedException(const std::string& keychain_name) : KeychainException("Keychain chain code could not be unlocked.", KEYCHAIN_CHAIN_CODE_UNLOCK_FAILED, keychain_name) { }
};

class KeychainPrivateKeyLockedException: public KeychainException
{
public:
    explicit KeychainPrivateKeyLockedException(const std::string& keychain_name) : KeychainException("Keychain private keys are locked.", KEYCHAIN_PRIVATE_KEY_LOCKED, keychain_name) { }
};

class KeychainPrivateKeyUnlockFailedException: public KeychainException
{
public:
    explicit KeychainPrivateKeyUnlockFailedException(const std::string& keychain_name) : KeychainException("Keychain private keys could not be unlocked.", KEYCHAIN_PRIVATE_KEY_UNLOCK_FAILED, keychain_name) { }
};

class KeychainIsNotPrivateException: public KeychainException
{
public:
    explicit KeychainIsNotPrivateException(const std::string& keychain_name) : KeychainException("Keychain is not private.", KEYCHAIN_IS_NOT_PRIVATE, keychain_name) { }
};

class KeychainInvalidPrivateKeyException: public KeychainException
{
public:
    explicit KeychainInvalidPrivateKeyException(const std::string& keychain_name, const bytes_t& pubkey) : KeychainException("Invalid private key for public key.", KEYCHAIN_INVALID_PRIVATE_KEY, keychain_name), pubkey_(pubkey) { }
    virtual ~KeychainInvalidPrivateKeyException() throw() { }
    const bytes_t& pubkey() const { return pubkey_; }

private:
    bytes_t pubkey_;
};

// ACCOUNT EXCEPTIONS
class AccountException : public stdutils::custom_error
{
public:
    virtual ~AccountException() throw() { }
    const std::string& account_name() const { return account_name_; }

protected:
    explicit AccountException(const std::string& what, int code, const std::string& account_name) : stdutils::custom_error(what, code), account_name_(account_name) { }
    std::string account_name_;
};

class AccountNotFoundException : public AccountException
{
public:
    explicit AccountNotFoundException(const std::string& account_name) : AccountException("Account not found.", ACCOUNT_NOT_FOUND, account_name) { }
};

class AccountAlreadyExistsException : public AccountException
{
public:
    explicit AccountAlreadyExistsException(const std::string& account_name) : AccountException("Account already exists.", ACCOUNT_ALREADY_EXISTS, account_name) { }
};

class AccountInsufficientFundsException : public AccountException
{
public:
    explicit AccountInsufficientFundsException(const std::string& account_name) : AccountException("Insufficient funds.", ACCOUNT_INSUFFICIENT_FUNDS, account_name) { }
};

class AccountCannotIssueChangeScriptException : public AccountException
{
public:
    explicit AccountCannotIssueChangeScriptException(const std::string& account_name) : AccountException("Account cannot issue change script", ACCOUNT_CANNOT_ISSUE_CHANGE_SCRIPT, account_name) { }
};

// ACCOUNT BIN EXCEPTIONS
class AccountBinException : public stdutils::custom_error
{
public:
    virtual ~AccountBinException() throw() { }
    const std::string& account_name() const { return account_name_; }
    const std::string& bin_name() const { return bin_name_; }

protected:
    explicit AccountBinException(const std::string& what, int code, const std::string& account_name, const std::string& bin_name) : stdutils::custom_error(what, code), account_name_(account_name), bin_name_(bin_name) { }
    std::string account_name_;
    std::string bin_name_;
};

class AccountBinNotFoundException : public AccountBinException
{
public:
    explicit AccountBinNotFoundException(const std::string& account_name, const std::string& bin_name) : AccountBinException("Account bin not found.", ACCOUNTBIN_NOT_FOUND, account_name, bin_name) { }
};

class AccountBinAlreadyExistsException : public AccountBinException
{
public:
    explicit AccountBinAlreadyExistsException(const std::string& account_name, const std::string& bin_name) : AccountBinException("Account bin already exists.", ACCOUNTBIN_ALREADY_EXISTS, account_name, bin_name) { }
};

class AccountBinOutOfScriptsException : public AccountBinException
{
public:
    explicit AccountBinOutOfScriptsException(const std::string& account_name, const std::string& bin_name) : AccountBinException("Account bin out of scripts.", ACCOUNTBIN_OUT_OF_SCRIPTS, account_name, bin_name) { }
};

// TX EXCEPTIONS
class TxException : public stdutils::custom_error
{
public:
    virtual ~TxException() throw() { }
    const bytes_t& hash() const { return hash_; }

protected:
    explicit TxException(const std::string& what, int code, const bytes_t& hash) : stdutils::custom_error(what, code), hash_(hash) { }
    bytes_t hash_;
};

class TxNotFoundException : public TxException
{
public:
    explicit TxNotFoundException(const bytes_t& hash = bytes_t()) : TxException("Transaction not found.", TX_NOT_FOUND, hash) { }
};

class TxInvalidInputsException : public TxException
{
public:
    explicit TxInvalidInputsException(const bytes_t& hash = bytes_t()) : TxException("Transaction inputs are invalid.", TX_INVALID_INPUTS, hash) { }
};

class TxOutputsExceedInputsException : public TxException
{
public:
    explicit TxOutputsExceedInputsException(const bytes_t& hash = bytes_t()) : TxException("Transaction outputs exceed inputs.", TX_OUTPUTS_EXCEED_INPUTS, hash) { }
};

class TxOutputNotFoundException : public TxException
{
public:
    explicit TxOutputNotFoundException(const bytes_t& outhash = bytes_t(), int outindex = -1) : TxException("Transaction output not found.", TX_OUTPUT_NOT_FOUND, outhash),  outindex_(outindex) { }

    int outindex() const { return outindex_; }

private:
    int outindex_;
};

// BLOCK HEADER EXCEPTIONS
class BlockHeaderException : public stdutils::custom_error
{
public:
    virtual ~BlockHeaderException() throw() { }
    const bytes_t& hash() const { return hash_; }
    uint32_t height() const { return height_; }

protected:
    explicit BlockHeaderException(const std::string& what, int code, const bytes_t& hash, uint32_t height) : stdutils::custom_error(what, code), hash_(hash), height_(height) { }
    explicit BlockHeaderException(const std::string& what, int code, const bytes_t& hash) : stdutils::custom_error(what, code), hash_(hash), height_(0) { }
    explicit BlockHeaderException(const std::string& what, int code, uint32_t height) : stdutils::custom_error(what, code), height_(height) { }
    bytes_t hash_;
    uint32_t height_;
};

class BlockHeaderNotFoundException : public BlockHeaderException
{
public:
    explicit BlockHeaderNotFoundException(const bytes_t& hash, uint32_t height) : BlockHeaderException("Block header not found.", BLOCKHEADER_NOT_FOUND, hash, height) { }
    explicit BlockHeaderNotFoundException(const bytes_t& hash) : BlockHeaderException("Block header not found.", BLOCKHEADER_NOT_FOUND, hash) { }
    explicit BlockHeaderNotFoundException(uint32_t height) : BlockHeaderException("Block header not found.", BLOCKHEADER_NOT_FOUND, height) { }
};

// MERKLE BLOCK EXCEPTIONS
class MerkleBlockException : public stdutils::custom_error
{
public:
    virtual ~MerkleBlockException() throw() { }
    const bytes_t& hash() const { return hash_; }
    uint32_t height() const { return height_; }

protected:
    explicit MerkleBlockException(const std::string& what, int code, const bytes_t& hash, uint32_t height) : stdutils::custom_error(what, code), hash_(hash), height_(height) { }
    bytes_t hash_;
    uint32_t height_;
};

class MerkleBlockInvalidException : public MerkleBlockException
{
public:
    explicit MerkleBlockInvalidException(const bytes_t& hash, uint32_t height) : MerkleBlockException("Merkle block is invalid.", MERKLEBLOCK_INVALID, hash, height) { }
};

// CHAIN CODE EXCEPTIONS
class ChainCodeException : public stdutils::custom_error
{
public:
    virtual ~ChainCodeException() throw() { }

protected:
    explicit ChainCodeException(const std::string& what, int code) : stdutils::custom_error(what, code) { }
};

class ChainCodesAreLockedException : public ChainCodeException
{
public:
    explicit ChainCodesAreLockedException() : ChainCodeException("Chain codes are locked.", CHAINCODE_LOCKED) { }
};

class ChainCodeUnlockFailedForKeychainException : public ChainCodeException
{
public:
    explicit ChainCodeUnlockFailedForKeychainException(const std::string& keychain_name) : ChainCodeException("Chain code is locked for a keychain.", CHAINCODE_UNLOCK_FAILED), keychain_name_(keychain_name) { }
    virtual ~ChainCodeUnlockFailedForKeychainException() throw() { }
    const std::string& keychain_name() const { return keychain_name_; }

protected:
    std::string keychain_name_;
};

class ChainCodeSetUnlockKeyFailedForKeychainException : public ChainCodeException
{
public:
    explicit ChainCodeSetUnlockKeyFailedForKeychainException(const std::string& keychain_name) : ChainCodeException("Failed to set unlock key for keychain chain code.", CHAINCODE_SET_UNLOCK_KEY_FAILED), keychain_name_(keychain_name) { }
    virtual ~ChainCodeSetUnlockKeyFailedForKeychainException() throw() { }
    const std::string& keychain_name() const { return keychain_name_; }

protected:
    std::string keychain_name_;
};

}
