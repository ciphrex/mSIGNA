////////////////////////////////////////////////////////////////////////////////
//
// WebSocketAPI
//
// Client.cpp
//
// Copyright (c) 2014 Eric Lombrozo, all rights reserved
//

#include "Client.h"

//#define REPORT_LOW_LEVEL

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

using namespace std;
using namespace json_spirit;
using namespace WebSocket;

/// Public Methods
Client::Client(const string& event_field, const string& data_field)
    : result_field("result"), error_field("error"), id_field("id"), bReturnFullResponse(false)
{
    bConnected = false;
    sequence = 0;

    this->event_field = event_field;
    this->data_field = data_field;

#if defined(REPORT_LOW_LEVEL)
    client.set_access_channels(websocketpp::log::alevel::all);
    client.set_error_channels(websocketpp::log::elevel::all);
#else
    client.clear_access_channels(websocketpp::log::alevel::all);
    client.clear_error_channels(websocketpp::log::elevel::all);
#endif

    client.init_asio();

    client.set_open_handler(bind(&Client::onOpen, this, ::_1));
    client.set_close_handler(bind(&Client::onClose, this, ::_1));
    client.set_fail_handler(bind(&Client::onFail, this, ::_1));
    client.set_message_handler(bind(&Client::onMessage, this, ::_1, ::_2));
}

Client::~Client()
{
    if (bConnected)
    {
        pConnection->close(websocketpp::close::status::going_away, "");
        bConnected = false;
    }
}

void Client::start(const string& serverUrl, OpenHandler on_open, CloseHandler on_close, LogHandler on_log, ErrorHandler on_error)
{
    if (bConnected) throw runtime_error("Already connected.");

    error_code_t error_code;
    pConnection = client.get_connection(serverUrl, error_code);
    if (error_code)
    {
#if defined(REPORT_LOW_LEVEL)
        client.get_alog().write(websocketpp::log::alevel::app, error_code.message());
#endif
        throw runtime_error(error_code.message());
    }

    sequence = 0; 
    this->on_open = on_open;
    this->on_close = on_close;
    this->on_log = on_log;
    this->on_error = on_error;
    client.connect(pConnection);
    client.run();

    client.reset();
    bConnected = false;
}

void Client::stop()
{
    if (bConnected)
    {
        pConnection->close(websocketpp::close::status::going_away, "");
    }
}

void Client::send(const Object& cmd, ResultCallback resultCallback, ErrorCallback errorCallback)
{
    Object seqCmd(cmd);
    seqCmd.push_back(Pair(id_field, sequence));
    if (resultCallback || errorCallback)
    {
        callback_map[sequence] = CallbackPair(resultCallback, errorCallback);
    }
    sequence++;
    string cmdStr = write_string<Value>(seqCmd, false);
    if (on_log) on_log(string("Sending command: ") + cmdStr);
    pConnection->send(cmdStr);
}

void Client::send(const JsonRpc::Request& request, ResultCallback resultCallback, ErrorCallback errorCallback)
{
    JsonRpc::Request seqRequest(request);
    seqRequest.setId((uint64_t)sequence);
    if (resultCallback || errorCallback)
    {
        callback_map[sequence] = CallbackPair(resultCallback, errorCallback);
    }
    sequence++;
    string cmdStr = seqRequest.getJson();
    if (on_log) on_log(string("Sending command: ") + cmdStr);
    pConnection->send(cmdStr);
}

Client& Client::on(const string& eventType, EventHandler handler)
{
    event_handler_map[eventType] = handler;
    return *this;
}

/// Protected Methods
void Client::onOpen(connection_hdl_t hdl)
{
    bConnected = true;
    if (on_log) on_log("Connection opened.");
    if (on_open) on_open();
}

void Client::onClose(connection_hdl_t hdl)
{
    bConnected = false;
    if (on_log) on_log("Connection closed.");
    if (on_close) on_close();
}

void Client::onFail(connection_hdl_t hdl)
{
    bConnected = false;
    if (on_error) on_error("Connection failed.");
}

void Client::onMessage(connection_hdl_t hdl, message_ptr_t msg)
{
    string json = msg->get_payload();

    try
    {
        if (on_log)
        {
            stringstream ss;
            ss << "Received message from server: " << json;
            on_log(ss.str());
        }

        Value value;
        read_string(json, value);
        const Object& obj = value.get_obj();
        const Value& id = find_value(obj, id_field);

        const Value& result = find_value(obj, result_field);
        if (result.type() != null_type && id.type() == int_type)
        {
            if (bReturnFullResponse)    { onResult(obj, id.get_uint64());    }
            else                        { onResult(result, id.get_uint64()); }
            return;
        }

        const Value& error = find_value(obj, error_field);
        if (error.type() != null_type && id.type() == int_type)
        {
            if (bReturnFullResponse)    { onError(obj, id.get_uint64());     }
            else                        { onError(error, id.get_uint64());   }
            return;
        }

        const Value& event = find_value(obj, event_field);
        if (event.type() == str_type)
        {
            auto it = event_handler_map.find(event.get_str());
            if (it != event_handler_map.end())
            {
                if (!data_field.empty())
                {
                    const Value& data = find_value(obj, data_field);
                    it->second(data);
                }
                else
                {
                    it->second(obj);
                }
            }
            return;
        }

        if (on_error)
        {
            stringstream ss;
            ss << "Invalid server message: " << json;
            on_error(ss.str());
        }
    }
    catch (const exception& e)
    {
        if (on_error)
        {
            stringstream ss;
            ss << "Server message parse error: " << json << " - " << e.what();
            on_error(ss.str());
        }
    }
}

void Client::onResult(const Value& result, uint64_t id)
{
    if (on_log)
    {
        string json = write_string<Value>(result, true);
        stringstream ss;
        ss << "Received result for id " << id << ": " << json;
        on_log(ss.str());
    }

    try
    {
        auto it = callback_map.find(id);
        if (it != callback_map.end() && it->second.first)
        {
            it->second.first(result);
            callback_map.erase(id);
        }
    }
    catch (const exception& e)
    {
        if (on_error)
        {
            stringstream ss;
            ss << "Server result parse error for id " << id << ": " << e.what();
            on_error(ss.str());
        }
    }
}

void Client::onError(const Value& error, uint64_t id)
{
    if (on_log)
    {
        string json = write_string<Value>(error, true);
        stringstream ss;
        ss << "Received error for id " << id << ": " << json;
        on_log(ss.str());
    }

    try
    {
        auto it = callback_map.find(id);
        if (it != callback_map.end() && it->second.second)
        {
            it->second.second(error);
            callback_map.erase(id);
        }
    }
    catch (const exception& e)
    {
        if (on_error)
        {
            stringstream ss;
            ss << "Server error parse error for id " << id << ": " << e.what();
            on_error(ss.str());
        }
    }
}

