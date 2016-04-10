///////////////////////////////////////////////////////////////////////////////
//
// JsonRpc.h 
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

#include <stdutils/customerror.h>

#include <json_spirit/json_spirit_reader_template.h>
#include <json_spirit/json_spirit_writer_template.h>
#include <json_spirit/json_spirit_utils.h>

#include <sstream>
#include <string>

namespace JsonRpc {

class Request
{
public:
    Request() { }
    explicit Request(const Request& request) : m_method(request.m_method), m_params(request.m_params), m_id(request.m_id) { }
    Request(const std::string& method, const json_spirit::Array& params = json_spirit::Array(), const json_spirit::Value& id = json_spirit::Value())
        : m_method(method), m_params(params), m_id(id) { }

    void setJson(const std::string& json);
    std::string getJson() const;

    void setMethod(const std::string& method) { m_method = method; }
    const std::string& getMethod() const { return m_method; }

    void setParams(const json_spirit::Array& params) { m_params = params; }
    const json_spirit::Array& getParams() const { return m_params; }

    void setId(const json_spirit::Value& id) { m_id = id; }
    const json_spirit::Value& getId() const { return m_id; }

private:
    std::string m_method;
    json_spirit::Array m_params;
    json_spirit::Value m_id;
};


class Response
{
public:
    Response() { }
    explicit Response(const Response& response) : m_result(response.m_result), m_error(response.m_error), m_id(response.m_id) { }
    Response(const json_spirit::Value& result, const json_spirit::Value& error, const json_spirit::Value& id)
        : m_result(result), m_error(error), m_id(id) { }

    void setJson(const std::string& json);
    std::string getJson() const;

    void setResult(const json_spirit::Value& result, const json_spirit::Value& id = json_spirit::Value());
    void setError(const json_spirit::Value& error, const json_spirit::Value& id = json_spirit::Value());
    void setError(const std::exception& e, const json_spirit::Value& id = json_spirit::Value());
    void setError(const stdutils::custom_error& e, const json_spirit::Value& id = json_spirit::Value());

    const json_spirit::Value& getResult() const { return m_result; }
    const json_spirit::Value& getError() const { return m_error; }
    const json_spirit::Value& getId() const { return m_id; }

private:
    json_spirit::Value m_result;
    json_spirit::Value m_error;
    json_spirit::Value m_id;
};

}
