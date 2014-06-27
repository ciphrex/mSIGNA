///////////////////////////////////////////////////////////////////////////////
//
// customerror.h
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.
//

#pragma once

#include <stdexcept>

namespace stdutils
{

class custom_error : public std::runtime_error
{
public:
    explicit custom_error(const std::string& what, int code = 0)
        : std::runtime_error(what), code_(code) { }

    explicit custom_error(const char* what, int code = 0)
        : std::runtime_error(what), code_(code) { }

    virtual ~custom_error() throw() { }

    int code() const { return code_; }

private:
    int code_;
};

}
