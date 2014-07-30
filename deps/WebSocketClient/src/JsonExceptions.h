///////////////////////////////////////////////////////////////////////////////
//
// JsonExceptions.h
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.
//

#pragma once

#include <stdutils/customerror.h>

namespace JsonRpc
{

enum ErrorCodes
{
    // JSON errors
    JSON_INVALID = 10001, // start numbering high so as not to clobber application errors
    JSON_MISSING_METHOD,
    JSON_INVALID_PARAMETER_FORMAT
};

// JSON EXCEPTIONS
class JsonException : public stdutils::custom_error
{
public:
    virtual ~JsonException() throw() { }
    const std::string& json() const { return json_; }

protected:
    explicit JsonException(const std::string& what, int code, const std::string& json) : stdutils::custom_error(what, code), json_(json) { }

    std::string json_;
};

class JsonInvalidException : public JsonException
{
public:
    explicit JsonInvalidException(const std::string& json) : JsonException("Invalid JSON.", JSON_INVALID, json) { }
};

class JsonMissingMethodException : public JsonException
{
public:
    explicit JsonMissingMethodException(const std::string& json) : JsonException("Missing method.", JSON_MISSING_METHOD, json) { }
};

class JsonInvalidParameterFormatException: public JsonException
{
public:
    explicit JsonInvalidParameterFormatException(const std::string& json) : JsonException("Invalid parameter format.", JSON_INVALID_PARAMETER_FORMAT, json) { }
};

}
