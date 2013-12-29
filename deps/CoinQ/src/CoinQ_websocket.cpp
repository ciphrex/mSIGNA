///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_websocket.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include "CoinQ_websocket.h"
#include "CoinQ_jsonrpc.h"
#include "CoinQ_coinjson.h"

#include <boost/lexical_cast.hpp>

using namespace CoinQ::WebSocket;

bool Server::onValidate(websocketpp::connection_hdl hdl)
{
    std::cout << "Server::onValidate()" << std::endl;
    ws_server_t::connection_ptr con = m_ws_server.get_con_from_hdl(hdl);
    std::string remote_endpoint = boost::lexical_cast<std::string>(con->get_remote_endpoint());
    std::cout << "Remote endpoint: " << remote_endpoint << std::endl;
    if (boost::regex_match(remote_endpoint, m_allow_ips_regex)) {
        std::cout << "Validation successful." << std::endl;
        return true;
    }
    else {
        std::cout << "Validation failed." << std::endl;
        return false;
    }
}

void Server::onOpen(websocketpp::connection_hdl hdl)
{
    std::cout << "Server::onOpen() called with hdl: " << hdl.lock().get() << std::endl;
    json_spirit::Object obj;
    obj.push_back(json_spirit::Pair("bestheader", getChainHeaderJsonObject(m_best_header)));
    m_ws_server.send(hdl, json_spirit::write_string<json_spirit::Value>(obj), websocketpp::frame::opcode::text);
}

void Server::onClose(websocketpp::connection_hdl hdl)
{
    std::cout << "Server::onClose() called with hdl: " << hdl.lock().get() << std::endl;
    boost::unique_lock<boost::mutex> lock(m_connectionMutex);
    m_tx_subscribers.erase(hdl);
    m_header_subscribers.erase(hdl);
    m_block_subscribers.erase(hdl);
}

void Server::onMessage(websocketpp::connection_hdl hdl, ws_server_t::message_ptr msg)
{
    std::cout << "Server::onMessage() called with hdl: " << hdl.lock().get()
              << " and message: " << msg->get_payload()
              << std::endl;

    std::stringstream err;

    try {
        JsonRpc::Request request(msg->get_payload());
        boost::unique_lock<boost::mutex> lock(m_requestMutex);
        m_requests.push(std::make_pair(hdl, request));
        lock.unlock();
        m_requestCond.notify_one();
    }
    catch (const std::exception& e) {
        JsonRpc::Response response;
        response.setError(e.what());
        m_ws_server.send(hdl, response.getJson(), msg->get_opcode());
    }
/*
        m_ws_server.send(hdl, msg->get_payload(), msg->get_opcode());
    } catch (const websocketpp::lib::error_code& e) {
        std::cout << "Echo failed because: " << e
                  << "(" << e.message() << ")" << std::endl;
    }*/
}

void Server::requestLoop()
{
    while (true) {
        boost::unique_lock<boost::mutex> lock(m_requestMutex);

        while (m_bRunning && m_requests.empty()) {
            m_requestCond.wait(lock);
        }

        if (!m_bRunning) break;

        client_request_t req = m_requests.front();
        m_requests.pop();

        lock.unlock();

        try {
            const std::string& method = req.second.getMethod();
            if (method == "subscribe") {
                const json_spirit::Value& params = req.second.getParams();
                if (params.type() != json_spirit::array_type) {
                    m_ws_server.send(req.first, "Invalid parameters.", websocketpp::frame::opcode::text);
                    continue;
                }
                const json_spirit::Array& streams = params.get_array();
                json_spirit::Array subscribedstreams;
                for (unsigned int i = 0; i < streams.size(); i++) {
                    if (streams[i].type() == json_spirit::str_type) {
                        std::string stream = streams[i].get_str();
                        if (stream == "tx") {
                            boost::unique_lock<boost::mutex> lock(m_connectionMutex);
                            m_tx_subscribers.insert(req.first);
                            subscribedstreams.push_back("tx");
                        }
                        else if (stream == "header") {
                            boost::unique_lock<boost::mutex> lock(m_connectionMutex);
                            m_header_subscribers.insert(req.first);
                            subscribedstreams.push_back("header");
                        }
                        else if (stream == "block") {
                            boost::unique_lock<boost::mutex> lock(m_connectionMutex);
                            m_block_subscribers.insert(req.first);
                            subscribedstreams.push_back("block");
                        }
                    }
                }
                json_spirit::Object result;
                result.push_back(json_spirit::Pair("subscribedstreams", subscribedstreams));
                JsonRpc::Response res;
                res.setResult(result, req.second.getId());
                m_ws_server.send(req.first, res.getJson(), websocketpp::frame::opcode::text);
            }
            else if (method == "unsubscribe") {
                const json_spirit::Value& params = req.second.getParams();
                if (params.type() != json_spirit::array_type) {
                    m_ws_server.send(req.first, "Invalid parameters.", websocketpp::frame::opcode::text);
                    continue;
                }
                const json_spirit::Array& streams = params.get_array();
                json_spirit::Array unsubscribedstreams;
                for (unsigned int i = 0; i < streams.size(); i++) {
                    if (streams[i].type() == json_spirit::str_type) {
                        std::string stream = streams[i].get_str();
                        if (stream == "tx") {
                            boost::unique_lock<boost::mutex> lock(m_connectionMutex);
                            m_tx_subscribers.erase(req.first);
                            unsubscribedstreams.push_back("tx");
                        }
                        else if (stream == "header") {
                            boost::unique_lock<boost::mutex> lock(m_connectionMutex);
                            m_header_subscribers.erase(req.first);
                            unsubscribedstreams.push_back("header");
                        }
                        else if (stream == "block") {
                            boost::unique_lock<boost::mutex> lock(m_connectionMutex);
                            m_block_subscribers.erase(req.first);
                            unsubscribedstreams.push_back("block");
                        }
                    }
                }
                json_spirit::Object result;
                result.push_back(json_spirit::Pair("unsubscribedstreams", unsubscribedstreams));
                JsonRpc::Response res;
                res.setResult(result, req.second.getId());
                m_ws_server.send(req.first, res.getJson(), websocketpp::frame::opcode::text);
            }
            else if (m_client_request_callback) {
                m_client_request_callback(req);
            }
            else {
                m_ws_server.send(req.first, "Invalid method.", websocketpp::frame::opcode::text);
            }
        }
        catch (const std::exception& e) {
            std::cout << "Server::requestLoop() - Error: " << e.what() << std::endl;
        }
    }
}

void Server::init(int port, const std::string& allow_ips)
{
    m_port = port;
    m_bRunning = false;
    m_client_request_callback = NULL;
    try {
        m_allow_ips_regex.assign(allow_ips);
    }
    catch (const boost::regex_error& e) {
        std::cout << std::endl << "WARNING: Invalid allowips regex. Allowing localhost only." << std::endl;
        m_allow_ips_regex.assign(DEFAULT_ALLOWED_IPS);
    }

    m_ws_server.set_access_channels(websocketpp::log::alevel::all);
    m_ws_server.clear_access_channels(websocketpp::log::alevel::frame_payload);

    m_ws_server.init_asio();

    m_ws_server.set_validate_handler(websocketpp::lib::bind(&Server::onValidate, this, websocketpp::lib::placeholders::_1));
    m_ws_server.set_open_handler(websocketpp::lib::bind(&Server::onOpen, this, websocketpp::lib::placeholders::_1));
    m_ws_server.set_close_handler(websocketpp::lib::bind(&Server::onClose, this, websocketpp::lib::placeholders::_1));
    m_ws_server.set_message_handler(websocketpp::lib::bind(&Server::onMessage, this, websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));
}

void Server::start()
{
    boost::unique_lock<boost::mutex> lock(m_startMutex);
    if (m_bRunning) {
        throw std::runtime_error("Server already started.");
    }

    m_bRunning = true;

    m_ws_server.listen(m_port);
    m_ws_server.start_accept();

    m_request_loop_thread   = boost::thread(websocketpp::lib::bind(&Server::requestLoop, this));
    m_io_service_thread     = boost::thread(websocketpp::lib::bind(&ws_server_t::run, &m_ws_server));
}

void Server::stop()
{
    boost::unique_lock<boost::mutex> lock(m_startMutex);
    m_bRunning = false;
    lock.unlock();

    std::cout << "Websocket server stopping request loop thread..." << std::flush;
    m_requestCond.notify_all();
    m_request_loop_thread.join();
    std::cout << "Done." << std::endl;

    std::cout << "Websocket server stopping io service thread..." << std::flush;
    m_ws_server.stop();
    m_io_service_thread.join();
    std::cout << "Done." << std::endl;
}

void Server::pushTx(const ChainTransaction& tx)
{
    boost::unique_lock<boost::mutex> lock(m_connectionMutex);
    for (auto hdl: m_tx_subscribers) {
        try {
            json_spirit::Object obj;
            obj.push_back(json_spirit::Pair("tx", CoinQ::getChainTransactionJsonObject(tx)));
            m_ws_server.send(hdl, json_spirit::write_string<json_spirit::Value>(obj), websocketpp::frame::opcode::text);
        }
        catch (const boost::system::error_code& ec) {
            std::cout << "Server::pushTx() - Boost error: (" << ec.value() << ") " << ec.message() << std::endl;
        }
        catch (const std::exception& e) {
            std::cout << "Server::pushTx() - STL Exception: " << e.what() << std::endl;
        }
        catch (...) {
            std::cout << "Server::pushTx() - Unknown error." << std::endl;
        }
    }
}

void Server::pushTx(websocketpp::connection_hdl hdl, const ChainTransaction& tx)
{
    json_spirit::Object obj;
    obj.push_back(json_spirit::Pair("tx", CoinQ::getChainTransactionJsonObject(tx)));
    m_ws_server.send(hdl, json_spirit::write_string<json_spirit::Value>(obj), websocketpp::frame::opcode::text);
}

void Server::pushHeader(const ChainHeader& header)
{
    boost::unique_lock<boost::mutex> lock(m_connectionMutex);
    for (auto hdl: m_header_subscribers) {
        try {
            json_spirit::Object obj;
            obj.push_back(json_spirit::Pair("header", CoinQ::getChainHeaderJsonObject(header)));
            m_ws_server.send(hdl, json_spirit::write_string<json_spirit::Value>(obj), websocketpp::frame::opcode::text);
        }
        catch (const boost::system::error_code& ec) {
            std::cout << "Server::pushHeader() - Boost error: (" << ec.value() << ") " << ec.message() << std::endl;
        }
        catch (const std::exception& e) {
            std::cout << "Server::pushHeader() - STL Exception: " << e.what() << std::endl;
        }
        catch (...) {
            std::cout << "Server::pushHeader() - Unknown error." << std::endl;
        }
    }
}

void Server::pushBlock(const ChainBlock& block, bool allFields)
{
    boost::unique_lock<boost::mutex> lock(m_connectionMutex);
    for (auto hdl: m_block_subscribers) {
        try {
            json_spirit::Object obj;
            obj.push_back(json_spirit::Pair("block", CoinQ::getChainBlockJsonObject(block, allFields)));
            m_ws_server.send(hdl, json_spirit::write_string<json_spirit::Value>(obj), websocketpp::frame::opcode::text);
        }
        catch (const boost::system::error_code& ec) {
            std::cout << "Server::pushBlock() - Boost error: (" << ec.value() << ") " << ec.message() << std::endl;
        }
        catch (const std::exception& e) {
            std::cout << "Server::pushBlock() - STL Exception: " << e.what() << std::endl;
        }
        catch (...) {
            std::cout << "Server::pushBlock() - Unknown error." << std::endl;
        }
    }
}

