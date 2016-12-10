///////////////////////////////////////////////////////////////////////////////
//
// VaultExceptions.h
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include "Schema.h"

#include <stdutils/customerror.h>

namespace CoinDB
{

enum ErrorCodes
{
    // Vault errors
    VAULT_WRONG_SCHEMA_VERSION = 101,
    VAULT_WRONG_NETWORK,
    VAULT_FAILED_TO_OPEN_DATABASE,
    VAULT_MISSING_TXS,
    VAULT_NEEDS_SCHEMA_MIGRATION,

    // Chain code errors
    CHAINCODE_LOCKED = 201,
    CHAINCODE_UNLOCK_FAILED,
    CHAINCODE_SET_UNLOCK_KEY_FAILED,

    // Keychain errors
    KEYCHAIN_NOT_FOUND = 301,
    KEYCHAIN_ALREADY_EXISTS,
    KEYCHAIN_CHAIN_CODE_LOCKED,
    KEYCHAIN_CHAIN_CODE_UNLOCK_FAILED,
    KEYCHAIN_PRIVATE_KEY_LOCKED,
    KEYCHAIN_PRIVATE_KEY_UNLOCK_FAILED,
    KEYCHAIN_IS_NOT_PRIVATE,
    KEYCHAIN_INVALID_PRIVATE_KEY,

    // Account errors
    ACCOUNT_NOT_FOUND = 401,
    ACCOUNT_ALREADY_EXISTS,
    ACCOUNT_INSUFFICIENT_FUNDS,
    ACCOUNT_CANNOT_ISSUE_CHANGE_SCRIPT,

    // Account bin errors
    ACCOUNTBIN_NOT_FOUND = 501,
    ACCOUNTBIN_ALREADY_EXISTS,
    ACCOUNTBIN_OUT_OF_SCRIPTS,

    // Tx errors
    TX_NOT_FOUND = 601,
    TX_INVALID_INPUTS,
    TX_OUTPUTS_EXCEED_INPUTS,
    TX_OUTPUT_NOT_FOUND,
    TX_MISMATCH,
    TX_NOT_SIGNED,
    TX_INVALID_OUTPUTS,
    TX_OUTPUT_SCRIPT_NOT_IN_USER_WHITELIST,

    // Block header errors
    BLOCKHEADER_NOT_FOUND = 701,

    // Merkle block errors
    MERKLEBLOCK_INVALID = 801,

    // MerkleTx errors
    MERKLETX_BAD_INSERTION_ORDER = 901,
    MERKLETX_MISMATCH,
    MERKLETX_FAILED_TO_CONNECT,
    MERKLETX_INVALID_HEIGHT,

    // SigningScript errors
    SIGNINGSCRIPT_NOT_FOUND = 1001,

    // User errors
    USER_NOT_FOUND = 1101,
    USER_ALREADY_EXISTS,
    USER_INVALID_USERNAME,

    // Contact errors
    CONTACT_NOT_FOUND = 1201,
    CONTACT_ALREADY_EXISTS,
    CONTACT_INVALID_USERNAME
};

// VAULT EXCEPTIONS
class VaultException : public stdutils::custom_error
{
public:
    virtual ~VaultException() throw() { }
    const std::string& vault_name() const { return vault_name_; }

protected:
    explicit VaultException(const std::string& what, int code, const std::string& vault_name) : stdutils::custom_error(what, code), vault_name_(vault_name) { }

    std::string vault_name_;
};
 
class VaultWrongSchemaVersionException : public VaultException
{
public:
    explicit VaultWrongSchemaVersionException(const std::string& vault_name, uint32_t schema_version) : VaultException("Wrong database schema version.", VAULT_WRONG_SCHEMA_VERSION, vault_name), schema_version_(schema_version) { }

    uint32_t schema_version() const { return schema_version_; }

private:
    uint32_t schema_version_;
};

class VaultWrongNetworkException : public VaultException
{
public:
    explicit VaultWrongNetworkException(const std::string& vault_name, const std::string& network) : VaultException("Wrong network.", VAULT_WRONG_NETWORK, vault_name), network_(network) { }

    const std::string&  network() const { return network_; }

private:
    std::string network_;
};

class VaultFailedToOpenDatabaseException : public VaultException
{
public:
    explicit VaultFailedToOpenDatabaseException(const std::string& vault_name, const std::string& dberror) : VaultException("Failed to open database.", VAULT_FAILED_TO_OPEN_DATABASE, vault_name), dberror_(dberror) { }

    const std::string&  dberror() const { return dberror_; }

private:
    std::string dberror_;
};

class VaultMissingTxsException : public VaultException
{
public:
    explicit VaultMissingTxsException(const std::string& vault_name, const hashvector_t& txhashes) : VaultException("Vault is missing transactions.", VAULT_MISSING_TXS, vault_name), txhashes_(txhashes) { }

    const hashvector_t& txhashes() const { return txhashes_; }

private:
    hashvector_t txhashes_;
};

class VaultNeedsSchemaMigrationException : public VaultException
{
public:
    explicit VaultNeedsSchemaMigrationException(const std::string& vault_name, uint32_t schema_version, uint32_t current_version) : VaultException("Schema migration required.", VAULT_NEEDS_SCHEMA_MIGRATION, vault_name), schema_version_(schema_version), current_version_(current_version) { }

    uint32_t schema_version() const { return schema_version_; }
    uint32_t current_version() const { return current_version_; }

private:
    uint32_t schema_version_;
    uint32_t current_version_;
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
    explicit AccountInsufficientFundsException(const std::string& account_name, uint64_t requested, uint64_t available, const std::string& username = std::string()) : AccountException("Insufficient funds.", ACCOUNT_INSUFFICIENT_FUNDS, account_name), requested_(requested), available_(available), username_(username) { }
    uint64_t requested() const { return requested_; }
    uint64_t available() const { return available_; }
    const std::string& username() const { return username_; }
    void username(const std::string& username) { username_ = username; }

private:
    uint64_t requested_;
    uint64_t available_;
    std::string username_;
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
    explicit TxOutputNotFoundException(const bytes_t& outhash = bytes_t(), int outindex = -1) : TxException("Transaction output not found.", TX_OUTPUT_NOT_FOUND, outhash), outindex_(outindex) { }

    int outindex() const { return outindex_; }

private:
    int outindex_;
};

class TxMismatchException : public TxException
{
public:
    explicit TxMismatchException(const bytes_t& hash = bytes_t()) : TxException("Transaction mismatch.", TX_MISMATCH, hash) { }
};

class TxNotSignedException : public TxException
{
public:
    explicit TxNotSignedException(const bytes_t& hash = bytes_t()) : TxException("Transaction is not signed.", TX_NOT_SIGNED, hash) { }
};

class TxInvalidOutputsException : public TxException
{
public:
    explicit TxInvalidOutputsException() : TxException("Transaction outputs are invalid.", TX_INVALID_OUTPUTS, bytes_t()) { }
};

class TxOutputScriptNotInUserWhitelistException : public TxException
{
public:
    explicit TxOutputScriptNotInUserWhitelistException(const std::string& username, const bytes_t& txoutscript) : TxException("Transaction output script is not in user whitelist.", TX_OUTPUT_SCRIPT_NOT_IN_USER_WHITELIST, bytes_t()), username_(username), txoutscript_(txoutscript) { }
    const std::string& username() const { return username_; }
    const bytes_t& txoutscript() const { return txoutscript_; }

protected:
    std::string username_;
    bytes_t txoutscript_;
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
    explicit BlockHeaderNotFoundException(uint32_t height = 0) : BlockHeaderException("Block header not found.", BLOCKHEADER_NOT_FOUND, height) { }
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

// MERKLETX EXCEPTIONS
class MerkleTxException : public stdutils::custom_error
{
public:
    virtual ~MerkleTxException() throw() { }

    const bytes_t& blockhash() const { return blockhash_; }
    uint32_t height() const { return height_; }

    const bytes_t& txhash() const { return txhash_; }

    unsigned int txindex() const { return txindex_; }
    unsigned int txtotal() const { return txtotal_; }

protected:
    explicit MerkleTxException(const std::string& what, int code, const bytes_t& blockhash, uint32_t height, const bytes_t& txhash, unsigned int txindex, unsigned int txtotal) : stdutils::custom_error(what, code), blockhash_(blockhash), height_(height), txhash_(txhash), txindex_(txindex), txtotal_(txtotal) { }

    bytes_t blockhash_;
    uint32_t height_;
    bytes_t txhash_;
    unsigned int txindex_;
    unsigned int txtotal_;
};

class MerkleTxBadInsertionOrderException : public MerkleTxException
{
public:
    explicit MerkleTxBadInsertionOrderException(const bytes_t& blockhash, uint32_t height, const bytes_t& txhash, unsigned int txindex, unsigned int txtotal) : MerkleTxException("Merkle transaction was inserted out of order.", MERKLETX_BAD_INSERTION_ORDER, blockhash, height, txhash, txindex, txtotal) { }
};

class MerkleTxMismatchException : public MerkleTxException
{
public:
    explicit MerkleTxMismatchException(const bytes_t& blockhash, uint32_t height, const bytes_t& txhash, unsigned int txindex, unsigned int txtotal) : MerkleTxException("Stored transaction does not match inserted transaction.", MERKLETX_MISMATCH, blockhash, height, txhash, txindex, txtotal) { }
};

class MerkleTxFailedToConnectException : public MerkleTxException
{
public:
    explicit MerkleTxFailedToConnectException(const bytes_t& blockhash, uint32_t height, const bytes_t& txhash, unsigned int txindex, unsigned int txtotal) : MerkleTxException("Merkleblock cannot connect to chain.", MERKLETX_FAILED_TO_CONNECT, blockhash, height, txhash, txindex, txtotal) { }
};

class MerkleTxInvalidHeightException : public MerkleTxException
{
public:
    explicit MerkleTxInvalidHeightException(const bytes_t& blockhash, uint32_t height, const bytes_t& txhash, unsigned int txindex, unsigned int txtotal) : MerkleTxException("Merkleblock has invalid height.", MERKLETX_INVALID_HEIGHT, blockhash, height, txhash, txindex, txtotal) { }
};

// SIGNING SCRIPT EXCEPTIONS
class SigningScriptException : public stdutils::custom_error
{
public:
    virtual ~SigningScriptException() throw() { }

protected:
    explicit SigningScriptException(const std::string& what, int code) : stdutils::custom_error(what, code) { }
};

class SigningScriptNotFoundException : public SigningScriptException
{
public:
    explicit SigningScriptNotFoundException() : SigningScriptException("Signing script not found.", SIGNINGSCRIPT_NOT_FOUND) { }
};

// USER EXCEPTIONS
class UserException : public stdutils::custom_error
{
public:
    virtual ~UserException() throw() { }
    const std::string& username() const { return username_; }

protected:
    explicit UserException(const std::string& what, int code, const std::string& username) : stdutils::custom_error(what, code), username_(username) { }

    std::string username_;
};

class UserNotFoundException : public UserException
{
public:
    explicit UserNotFoundException(const std::string& username) : UserException("User not found.", USER_NOT_FOUND, username) { }
};

class UserAlreadyExistsException : public UserException
{
public:
    explicit UserAlreadyExistsException(const std::string& username) : UserException("User already exists.", USER_ALREADY_EXISTS, username) { }
};

class UserInvalidUsernameException : public UserException
{
public:
    explicit UserInvalidUsernameException(const std::string& username) : UserException("Invalid user username.", USER_INVALID_USERNAME, username) { }
};

// CONTACT EXCEPTIONS
class ContactException : public stdutils::custom_error
{
public:
    virtual ~ContactException() throw() { }
    const std::string& username() const { return username_; }

protected:
    explicit ContactException(const std::string& what, int code, const std::string& username) : stdutils::custom_error(what, code), username_(username) { }

    std::string username_;
};

class ContactNotFoundException : public ContactException
{
public:
    explicit ContactNotFoundException(const std::string& username) : ContactException("Contact not found.", CONTACT_NOT_FOUND, username) { }
};

class ContactAlreadyExistsException : public ContactException
{
public:
    explicit ContactAlreadyExistsException(const std::string& username) : ContactException("Contact already exists.", CONTACT_ALREADY_EXISTS, username) { }
};

class ContactInvalidUsernameException : public ContactException
{
public:
    explicit ContactInvalidUsernameException(const std::string& username) : ContactException("Invalid contact username.", CONTACT_INVALID_USERNAME, username) { }
};

}
