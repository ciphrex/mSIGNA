///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_jsonrpc.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "CoinQ_jsonrpc.h"

using namespace CoinQ::JsonRpc;

void Request::setJson(const std::string& json)
{
    json_spirit::Value value;
    json_spirit::read_string(json, value);
    if (value.type() != json_spirit::obj_type) {
        throw std::runtime_error("Invalid JSON.");
    }

    const json_spirit::Object& obj = value.get_obj();
    const json_spirit::Value& method = json_spirit::find_value(obj, "method");
    if (method.type() != json_spirit::str_type) {
        throw std::runtime_error("Missing method.");
    }
    m_method = method.get_str();
    m_params = json_spirit::find_value(obj, "params");
    m_id = json_spirit::find_value(obj, "id");
}

std::string Request::getJson() const
{
    json_spirit::Object req;
    req.push_back(json_spirit::Pair("method", m_method));
    req.push_back(json_spirit::Pair("params", m_params));
    req.push_back(json_spirit::Pair("id", m_id));
    return json_spirit::write_string<json_spirit::Value>(req);
}



void Response::setJson(const std::string& json)
{
    json_spirit::Value value;
    json_spirit::read_string(json, value);
    if (value.type() != json_spirit::obj_type) {
        throw std::runtime_error("Invalid JSON.");
    }
    const json_spirit::Object& obj = value.get_obj();
    m_result = json_spirit::find_value(obj, "result");
    m_error = json_spirit::find_value(obj, "error");
    m_id = json_spirit::find_value(obj, "id");
}

std::string Response::getJson() const
{
    json_spirit::Object res;
    res.push_back(json_spirit::Pair("result", m_result));
    res.push_back(json_spirit::Pair("error", m_error));
    res.push_back(json_spirit::Pair("id", m_id));
    return json_spirit::write_string<json_spirit::Value>(res);
}

void Response::setResult(const json_spirit::Value& result, const json_spirit::Value& id)
{
    m_result = result;
    m_error = json_spirit::Value();
    m_id = id;
}

void Response::setError(const json_spirit::Value& error, const json_spirit::Value& id)
{
    m_result = json_spirit::Value();
    m_error = error;
    m_id = id;
}

