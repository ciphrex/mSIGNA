///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_websocket.h 
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef _COINQ_WEBSOCKET_H_
#define _COINQ_WEBSOCKET_H_

#include "CoinQ_blocks.h"
#include "CoinQ_txs.h"
#include "CoinQ_coinjson.h"

#include "CoinQ_jsonrpc.h"

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <boost/thread.hpp>
#include <boost/regex.hpp>

#include <memory>
#include <queue>
#include <set>

namespace CoinQ {
namespace WebSocket {

const std::string DEFAULT_ALLOWED_IPS = "^\\[(::1|::ffff:127\\.0\\.0\\.1)\\].*";

class Server
{
public:
    typedef std::pair<websocketpp::connection_hdl, JsonRpc::Request> client_request_t;
    typedef std::function<void(const client_request_t&)> client_request_callback_t;

private:
    typedef websocketpp::server<websocketpp::config::asio> ws_server_t;
    ws_server_t m_ws_server;

    typedef std::set<websocketpp::connection_hdl> connection_set_t;
    connection_set_t m_connections;

    int m_port;
    boost::regex m_allow_ips_regex;

    typedef std::queue<client_request_t> request_queue_t;
    request_queue_t m_requests;

    client_request_callback_t m_client_request_callback;

    typedef std::set<websocketpp::connection_hdl> subscribers_t;
    subscribers_t m_tx_subscribers;
    subscribers_t m_header_subscribers;
    subscribers_t m_block_subscribers;

    boost::mutex m_requestMutex;
    boost::condition_variable m_requestCond;

    boost::mutex m_responseMutex;
    boost::condition_variable m_responseCond;

    boost::mutex m_connectionMutex;

    ChainHeader m_best_header;

    bool onValidate(websocketpp::connection_hdl hdl);
    void onOpen(websocketpp::connection_hdl hdl);
    void onClose(websocketpp::connection_hdl hdl);
    void onMessage(websocketpp::connection_hdl hdl, ws_server_t::message_ptr msg);

    void requestLoop();

    bool m_bRunning;

    boost::mutex m_startMutex;

    boost::thread m_request_loop_thread;
    boost::thread m_io_service_thread;

    void init(int port, const std::string& allow_ips);

public:
    Server(int port, const std::string& allow_ips = DEFAULT_ALLOWED_IPS) { init(port, allow_ips); }
    Server(const std::string& port, const std::string& allow_ips = DEFAULT_ALLOWED_IPS) { init(strtoul(port.c_str(), NULL, 10), allow_ips); }

    void start();
    void stop();
    void send(websocketpp::connection_hdl hdl, const JsonRpc::Response& res) { m_ws_server.send(hdl, res.getJson(), websocketpp::frame::opcode::text); }

    void pushTx(const ChainTransaction& tx);
    void pushHeader(const ChainHeader& header);
    void pushBlock(const ChainBlock& block, bool allFields = false);

    void pushTx(websocketpp::connection_hdl hdl, const ChainTransaction& tx);

    void setBestHeader(const ChainHeader& header) { m_best_header = header; }

    void setClientRequestCallback(client_request_callback_t callback) { m_client_request_callback = callback; }
};

}
}

#endif // _COINQ_WEBSOCKET_H_
