///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_jsonrpc.h 
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#ifndef _COINQ_JSONRPC_H_
#define _COINQ_JSONRPC_H_

#include <json_spirit/json_spirit_reader_template.h>
#include <json_spirit/json_spirit_writer_template.h>
#include <json_spirit/json_spirit_utils.h>

#include <stdexcept>
#include <sstream>
#include <string>

namespace CoinQ {
namespace JsonRpc {

class Request
{
private:
    std::string m_method;
    json_spirit::Value m_params;
    json_spirit::Value m_id;

public:
    Request() { }
    Request(const Request& request) : m_method(request.m_method), m_params(request.m_params), m_id(request.m_id) { }
    Request(const std::string& method, const json_spirit::Object& params, const json_spirit::Value& id = json_spirit::Value())
        : m_method(method), m_params(params), m_id(id) { }
    Request(const std::string& json) { setJson(json); }

    void setJson(const std::string& json);
    std::string getJson() const;

    const std::string& getMethod() const { return m_method; }
    const json_spirit::Value& getParams() const { return m_params; }
    const json_spirit::Value& getId() const { return m_id; }
};


class Response
{
private:
    json_spirit::Value m_result;
    json_spirit::Value m_error;
    json_spirit::Value m_id;

public:
    Response() { }
    Response(const Response& response) : m_result(response.m_result), m_error(response.m_error), m_id(response.m_id) { }
    Response(const json_spirit::Value& result, const json_spirit::Value& error, const json_spirit::Value& id)
        : m_result(result), m_error(error), m_id(id) { }
    Response(const std::string& json) { setJson(json); }

    void setJson(const std::string& json);
    std::string getJson() const;

    void setResult(const json_spirit::Value& result, const json_spirit::Value& id = json_spirit::Value());
    void setError(const json_spirit::Value& error, const json_spirit::Value& id = json_spirit::Value());

    const json_spirit::Value& getResult() const { return m_result; }
    const json_spirit::Value& getError() const { return m_error; }
    const json_spirit::Value& getId() const { return m_id; }
};

}
}

#endif // _COINQ_JSONRPC_H_
