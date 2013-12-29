////////////////////////////////////////////////////////////////////////////////
//
// CoinQ_errors.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef _COINQ_ERRORS_H
#define _COINQ_ERRORS_H

#include <string>
#include <stdexcept>

namespace CoinQ {

class Exception : public std::runtime_error
{
public:
    enum Error {
        NOT_FOUND,
        INVALID_PARAMETERS
    };

    Exception(const std::string& message, int error) : std::runtime_error(message), m_error(error) { }
    int error() const { return m_error; }

private:
    int m_error;
};

}

#endif // _COINQ_ERRORS_H
