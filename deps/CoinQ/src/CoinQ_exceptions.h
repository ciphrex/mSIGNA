///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_exceptions.h
//
// Copyright (c) 2012-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include <stdutils/customerror.h>

#include <string>

namespace CoinQ {

enum ErrorCodes
{
    // NetworkSelector errors
    NETWORKSELECTOR_NO_NETWORK_SELECTED = 10000,
    NETWORKSELECTOR_NETWORK_NOT_RECOGNIZED,

    // BlockTree errors
    BLOCKTREE_FILE_NOT_FOUND = 10100,
    BLOCKTREE_INVALID_FILE_TYPE,
    BLOCKTREE_INVALID_FILE_LENGTH,
    BLOCKTREE_FAILED_TO_OPEN_FILE_FOR_READ,
    BLOCKTREE_FILE_READ_FAILURE,
    BLOCKTREE_FILE_WRITE_FAILURE,
    BLOCKTREE_CHECKSUM_ERROR,
    BLOCKTREE_LOAD_INTERRUPTED,
    BLOCKTREE_UNEXPECTED_END_OF_FILE,
    BLOCKTREE_SWAPFILE_ALREADY_EXISTS
};

// NETWORK SELECTOR EXCEPTIONS
class NetworkSelectorException : public stdutils::custom_error
{
public:
    virtual ~NetworkSelectorException() throw() { }

protected:
    explicit NetworkSelectorException(const std::string& what, int code) : stdutils::custom_error(what, code) { }
};

class NetworkSelectorNoNetworkSelectedException : public NetworkSelectorException
{
public:
    explicit NetworkSelectorNoNetworkSelectedException() : NetworkSelectorException("No network selected.", NETWORKSELECTOR_NO_NETWORK_SELECTED) {}
};

class NetworkSelectorNetworkNotRecognizedException : public NetworkSelectorException
{
public:
    explicit NetworkSelectorNetworkNotRecognizedException(const std::string& network) : NetworkSelectorException("Network not recognized.", NETWORKSELECTOR_NETWORK_NOT_RECOGNIZED), network_(network) {}

    const std::string& network() const { return network_; }

private:
    std::string network_;
};

// BLOCK TREE EXCEPTIONS
class BlockTreeException : public stdutils::custom_error
{
public:
    virtual ~BlockTreeException() throw() { }

protected:
    explicit BlockTreeException(const std::string& what, int code) : stdutils::custom_error(what, code) { }
};

class BlockTreeFileNotFoundException : public BlockTreeException
{
public:
    explicit BlockTreeFileNotFoundException() : BlockTreeException("Blocktree file not found.", BLOCKTREE_FILE_NOT_FOUND) { }
};

class BlockTreeInvalidFileTypeException : public BlockTreeException
{
public:
    explicit BlockTreeInvalidFileTypeException() : BlockTreeException("Blocktree invalid file type.", BLOCKTREE_INVALID_FILE_TYPE) { }
};

class BlockTreeInvalidFileLengthException : public BlockTreeException
{
public:
    explicit BlockTreeInvalidFileLengthException() : BlockTreeException("Blocktree invalid file length.", BLOCKTREE_INVALID_FILE_LENGTH) { }
};

class BlockTreeFailedToOpenFileForReadException : public BlockTreeException
{
public:
    explicit BlockTreeFailedToOpenFileForReadException() : BlockTreeException("Blocktree failed to open file for read.", BLOCKTREE_FAILED_TO_OPEN_FILE_FOR_READ) { }
};

class BlockTreeFileReadFailureException : public BlockTreeException
{
public:
    explicit BlockTreeFileReadFailureException() : BlockTreeException("Blocktree file read failure.", BLOCKTREE_FILE_READ_FAILURE) { }
};

class BlockTreeFileWriteFailureException : public BlockTreeException
{
public:
    explicit BlockTreeFileWriteFailureException() : BlockTreeException("Blocktree file write failure.", BLOCKTREE_FILE_WRITE_FAILURE) { }
};

class BlockTreeChecksumErrorException : public BlockTreeException
{
public:
    explicit BlockTreeChecksumErrorException() : BlockTreeException("Blocktree checksum error.", BLOCKTREE_CHECKSUM_ERROR) { }
};

class BlockTreeLoadInterruptedException : public BlockTreeException
{
public:
    explicit BlockTreeLoadInterruptedException() : BlockTreeException("Blocktree load interrupted.", BLOCKTREE_LOAD_INTERRUPTED) { }
};

class BlockTreeUnexpectedEndOfFileException : public BlockTreeException
{
public:
    explicit BlockTreeUnexpectedEndOfFileException() : BlockTreeException("Blocktree unexpected end of file.", BLOCKTREE_UNEXPECTED_END_OF_FILE) { }
};

class BlockTreeSwapfileAlreadyExistsException : public BlockTreeException
{
public:
    explicit BlockTreeSwapfileAlreadyExistsException() : BlockTreeException("Blocktree swapfile already exists.", BLOCKTREE_SWAPFILE_ALREADY_EXISTS) { }
};

}

