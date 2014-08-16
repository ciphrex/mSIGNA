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
#include <string>

namespace stdutils
{

class custom_error : public std::runtime_error
{
public:
    explicit custom_error(const std::exception& e)
        : std::runtime_error(e.what()), has_code_(false), code_(0) { }
 
    explicit custom_error(const std::string& what)
        : std::runtime_error(what), has_code_(false), code_(0) { }

    explicit custom_error(const char* what)
        : std::runtime_error(what), has_code_(false), code_(0) { }

    custom_error(const std::string& what, int code)
        : std::runtime_error(what), has_code_(true), code_(code) { }

    custom_error(const char* what, int code)
        : std::runtime_error(what), has_code_(true), code_(code) { }

    virtual ~custom_error() throw() { }

    bool has_code() const { return has_code_; }
    int code() const { return code_; }

private:
    bool has_code_;
    int code_;
};

}
