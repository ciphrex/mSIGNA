///////////////////////////////////////////////////////////////////////////////
//
// JsonRpc.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#include "JsonRpc.h"
#include "JsonExceptions.h"

using namespace JsonRpc;
using namespace json_spirit;

void Request::setJson(const std::string& json)
{
    Value value;
    read_string(json, value);
    if (value.type() != obj_type)
    {
        throw JsonInvalidException(json);
    }

    const Object& obj = value.get_obj();
    const Value& method = find_value(obj, "method");
    if (method.type() != str_type)
    {
        throw JsonMissingMethodException(json);
    }

    const Value& params = find_value(obj, "params");
    if (params.is_null())
    {
        m_params = Array();
    }
    else if (params.type() != array_type)
    {
        throw JsonInvalidParameterFormatException(json);
    }
    else
    {
        m_params = params.get_array();
    }

    m_method = method.get_str();
    m_id = find_value(obj, "id");
}

std::string Request::getJson() const
{
    Object req;
    req.push_back(Pair("method", m_method));
    req.push_back(Pair("params", m_params));
    req.push_back(Pair("id", m_id));
    return write_string<Value>(req);
}



void Response::setJson(const std::string& json)
{
    Value value;
    read_string(json, value);
    if (value.type() != obj_type) {
        throw JsonInvalidException(json);
    }
    const Object& obj = value.get_obj();
    m_result = find_value(obj, "result");
    m_error = find_value(obj, "error");
    m_id = find_value(obj, "id");
}

std::string Response::getJson() const
{
    Object res;
    res.push_back(Pair("result", m_result));
    res.push_back(Pair("error", m_error));
    res.push_back(Pair("id", m_id));
    return write_string<Value>(res);
}

void Response::setResult(const Value& result, const Value& id)
{
    m_result = result;
    m_error = Value();
    m_id = id;
}

void Response::setError(const Value& error, const Value& id)
{
    m_result = Value();
    m_error = error;
    m_id = id;
}

void Response::setError(const std::exception& e, const Value& id)
{
    m_result = Value();

    Object error;
    error.push_back(Pair("message", e.what()));
    error.push_back(Pair("code", Value()));    
    m_error = error;

    m_id = id;
}

void Response::setError(const stdutils::custom_error& e, const Value& id)
{
    m_result = Value();

    Object error;
    error.push_back(Pair("message", e.what()));
    if (e.has_code())   { error.push_back(Pair("code", e.code())); }
    else                { error.push_back(Pair("code", Value()));  }
    m_error = error;

    m_id = id;
}

