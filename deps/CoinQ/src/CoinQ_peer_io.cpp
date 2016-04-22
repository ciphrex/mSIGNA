///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_peer_io.cpp
//
// Copyright (c) 2012-2013 Eric Lombrozo
//
// All Rights Reserved.

#include "CoinQ_peer_io.h"

#include <sstream>

using namespace CoinQ;
using namespace std;

const unsigned char Peer::DEFAULT_Ipv6[] = {0,0,0,0,0,0,0,0,0,0,255,255,127,0,0,1};

void Peer::do_handshake()
{
    if (!bRunning) return;

    // Send version message
    Coin::NetworkAddress peerAddress;
    peerAddress.set(NODE_NETWORK, DEFAULT_Ipv6, strtoul(port_.c_str(), NULL, 0));
    Coin::VersionMessage versionMessage(protocol_version_, NODE_NETWORK, time(NULL), peerAddress, peerAddress, getRandomNonce64(), user_agent_.c_str(), start_height_, relay_);
    Coin::CoinNodeMessage msg(magic_bytes_, &versionMessage);

    LOGGER(trace) << "Sending version message." << endl;
    do_send(msg);

    // Give peer 5 seconds to respond
    timer_.expires_from_now(boost::posix_time::seconds(5));
    timer_.async_wait([this](const boost::system::error_code& ec) {
        if (!bRunning) return;
        LOGGER(trace) << "Peer timer handler" << std::endl;

        if (ec == boost::asio::error::operation_aborted) return;

        if (bHandshakeComplete) return;
        boost::lock_guard<boost::mutex> lock(handshakeMutex);
        if (bHandshakeComplete) return;
    
        do_stop();
        notifyTimeout(*this);
    });
}

void Peer::do_keepaliveCheck()
{
    if (!bRunning || idle_duration_before_ping_sent == 0) return;

    boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
    boost::posix_time::time_duration diff = now - last_data_received_at;
    long idle_time = diff.total_seconds();

    //LOGGER(trace) << "Keep Alive Handler - last data received " << idle_time << " seconds ago." << std::endl;

    if(idle_time >= idle_duration_before_ping_sent) {
        // If no response to ping we previously sent.. disconnect peer
        if(bExpectingPong) {
            LOGGER(trace) << "Keep Alive Handler - No response to ping." << std::endl;
            do_stop();
            notifyTimeout(*this);
            return;
        }

        // Send a ping after idle timeout is reached
        Coin::PingMessage pingMessage;
        Coin::CoinNodeMessage msg(magic_bytes_, &pingMessage);
        LOGGER(trace) << "Keep Alive Handler - Sending ping message." << std::endl;
        do_send(msg);

        bExpectingPong = true;
        keepalive_timer_.expires_from_now(boost::posix_time::seconds(max_ping_response_delay));

    } else {
        keepalive_timer_.expires_from_now(boost::posix_time::seconds(idle_duration_before_ping_sent - idle_time));
    }

    keepalive_timer_.async_wait([this](const boost::system::error_code& ec) {
        if (!bRunning) return;
        if (ec == boost::asio::error::operation_aborted) return;
        strand_.post(boost::bind(&Peer::do_keepaliveCheck, this));
    });
}

void Peer::do_read()
{
    LOGGER(trace) << "Peer::do_read() - waiting for " << min_read_bytes << " bytes..." << endl;
    boost::asio::async_read(socket_, boost::asio::buffer(read_buffer, READ_BUFFER_SIZE),
        transfer_at_least_t(min_read_bytes, &last_data_received_at),
    strand_.wrap([this](const boost::system::error_code& ec, std::size_t bytes_read) {
        if (!bRunning) return;

        if (ec)
        {
            if (ec == boost::asio::error::operation_aborted) return;

            read_message.clear();
            do_stop();

            stringstream err;
            err << "Peer read error: " << ec.message();
            LOGGER(debug) << "Peer::do_read() handler - " << err.str() << std::endl;
            notifyConnectionError(*this, err.str(), ec.value());
            return;
        }

        read_message += uchar_vector(read_buffer, bytes_read);

        while (true)
        {
            if (read_message.size() < MIN_MESSAGE_HEADER_SIZE)
            {
                min_read_bytes = MIN_MESSAGE_HEADER_SIZE - read_message.size();
                break;
            }

            // Find the first occurrence of the magic bytes, discard anything before it.
            // If magic bytes are not found, read new buffer. 
            // TODO: detect misbehaving node and disconnect.
            uchar_vector::iterator it = std::search(read_message.begin(), read_message.end(), magic_bytes_vector_.begin(), magic_bytes_vector_.end());
            if (it == read_message.end())
            {
                read_message.clear();
                min_read_bytes = MIN_MESSAGE_HEADER_SIZE;
                break;
            }

            read_message.assign(it, read_message.end());
            if (read_message.size() < MIN_MESSAGE_HEADER_SIZE)
            {
                min_read_bytes = MIN_MESSAGE_HEADER_SIZE - read_message.size();
                break;
            }

            // Get command
            unsigned char command[13];
            command[12] = 0;
            uchar_vector(read_message.begin() + 4, read_message.begin() + 16).copyToArray(command);
            LOGGER(debug) << "Peer read handler - command: " << command << endl;

            // Get payload size
            unsigned int payloadSize = vch_to_uint<uint32_t>(uchar_vector(read_message.begin() + 16, read_message.begin() + 20), LITTLE_ENDIAN_);
            LOGGER(debug) << "Peer read handler - payload size: " << payloadSize << endl;
            LOGGER(debug) << "Peer read handler - read_message size: " << read_message.size() << endl;

            if (read_message.size() < MIN_MESSAGE_HEADER_SIZE  + payloadSize)
            {
                min_read_bytes = MIN_MESSAGE_HEADER_SIZE + payloadSize - read_message.size();
                break;
            }

            try
            {
                Coin::CoinNodeMessage peerMessage(read_message);

                if (!peerMessage.isChecksumValid()) throw std::runtime_error("Invalid checksum.");

                std::string command = peerMessage.getCommand();
                if (command == "verack") {
                    LOGGER(trace) << "Peer read handler - VERACK" << std::endl;

                    // Signal completion of handshake
                    if (bHandshakeComplete) throw std::runtime_error("Second verack received.");
                    boost::unique_lock<boost::mutex> lock(handshakeMutex);
                    if (bHandshakeComplete) throw std::runtime_error("Second verack received.");
                    timer_.cancel();
                    bHandshakeComplete = true;
                    lock.unlock();
                    bWriteReady = true;
                    notifyOpen(*this);
                    bExpectingPong = false;
                    do_keepaliveCheck();
                }
                else if (command == "version")
                {
                    LOGGER(trace) << "Peer read handler - VERSION" << std::endl;

                    // TODO: Check version information
                    Coin::VerackMessage verackMessage;
                    Coin::CoinNodeMessage msg(magic_bytes_, &verackMessage);
                    do_send(msg);
                }
                else if (command == "inv")
                {
                    LOGGER(trace) << "Peer read handler - INV" << std::endl;

                    Coin::Inventory* pInventory = static_cast<Coin::Inventory*>(peerMessage.getPayload());
                    notifyInv(*this, *pInventory);
                }
                else if (command == "tx")
                {
                    LOGGER(trace) << "Peer read handler - TX" << std::endl;

                    Coin::Transaction* pTx = static_cast<Coin::Transaction*>(peerMessage.getPayload());
                    notifyTx(*this, *pTx);
                }
                else if (command == "block")
                {
                    LOGGER(trace) << "Peer read handler - BLOCK" << std::endl;

                    Coin::CoinBlock* pBlock = static_cast<Coin::CoinBlock*>(peerMessage.getPayload());
                    notifyBlock(*this, *pBlock);
                }
                else if (command == "merkleblock")
                {
                    LOGGER(trace) << "Peer read handler - MERKLEBLOCK" << std::endl;

                    Coin::MerkleBlock* pMerkleBlock = static_cast<Coin::MerkleBlock*>(peerMessage.getPayload());
                    notifyMerkleBlock(*this, *pMerkleBlock);
                }
                else if (command == "addr")
                {
                    LOGGER(trace) << "Peer read handler - ADDR" << std::endl;

                    Coin::AddrMessage* pAddr = static_cast<Coin::AddrMessage*>(peerMessage.getPayload());
                    notifyAddr(*this, *pAddr);
                }
                else if (command == "headers")
                {
                    LOGGER(trace) << "Peer read handler - HEADERS" << std::endl;

                    Coin::HeadersMessage* pHeaders = static_cast<Coin::HeadersMessage*>(peerMessage.getPayload());
                    notifyHeaders(*this, *pHeaders);
                }
                else if (command == "ping")
                {
                    LOGGER(trace) << "Peer read handler - PING" << std::endl;

                    Coin::PingMessage* pPing = static_cast<Coin::PingMessage*>(peerMessage.getPayload());
                    Coin::PongMessage pongMessage(pPing->nonce);
                    Coin::CoinNodeMessage msg(magic_bytes_, &pongMessage);
                    do_send(msg);
                }
                else if (command == "pong")
                {
                    LOGGER(trace) << "Peer read handler - PONG" << std::endl;
                    bExpectingPong = false;
                }
                else
                {
                    LOGGER(error) << "Peer read handler - command not implemented: " << command << std::endl;

                    std::stringstream err;
                    err << "Command type not implemented: " << command;
                    notifyProtocolError(*this, err.str(), -1);
                }

                notifyMessage(*this, peerMessage);
            }
            catch (const std::exception& e)
            {
                std::stringstream err;
                err << "Message decode error: " << e.what();
                LOGGER(error) << "Peer read handler error: " << err.str() << std::endl;
                notifyProtocolError(*this, err.str(), -1);
                min_read_bytes = MIN_MESSAGE_HEADER_SIZE;
            }

            read_message.assign(read_message.begin() + MIN_MESSAGE_HEADER_SIZE + payloadSize, read_message.end());
            LOGGER(debug) << "Peer read handler - remaining message bytes: " << read_message.size() << endl;
        }

        do_read();
    }));
}

void Peer::do_write(boost::shared_ptr<uchar_vector> data)
{
    if (!bRunning) return;

    boost::asio::async_write(socket_, boost::asio::buffer(*data), boost::asio::transfer_all(),
    strand_.wrap([this](const boost::system::error_code& ec, std::size_t bytes_written) {
        if (!bRunning) return;
        LOGGER(trace) << "Peer write handler." << std::endl;

        if (ec)
        {
            if (ec == boost::asio::error::operation_aborted) return;

            stringstream err;
            err << "Peer write error: " << ec.message();
            notifyConnectionError(*this, err.str(), ec.value());
            return;
        }

        boost::lock_guard<boost::mutex> sendLock(sendMutex);
        sendQueue.pop();
        if (!sendQueue.empty()) { do_write(sendQueue.front()); }
    }));
}

void Peer::do_send(const Coin::CoinNodeMessage& message)
{
    boost::shared_ptr<uchar_vector> data(new uchar_vector(message.getSerialized()));
    boost::lock_guard<boost::mutex> sendLock(sendMutex);
    sendQueue.push(data);
    if (sendQueue.size() == 1) { strand_.post(boost::bind(&Peer::do_write, this, data)); }
}

void Peer::do_connect(tcp::resolver::iterator iter)
{
    boost::asio::async_connect(socket_, iter, strand_.wrap([this](const boost::system::error_code& ec, tcp::resolver::iterator) {
        if (!bRunning) return;
        LOGGER(trace) << "Peer connect handler." << std::endl;

        if (ec)
        {
            if (ec == boost::asio::error::operation_aborted) return;

            do_stop();

            stringstream err;
            err << "Peer connect error: " << ec.message();
            notifyConnectionError(*this, err.str(), ec.value());
            return;
        }

        try
        {
            do_read();
            do_handshake();
        }
        catch (const boost::system::error_code& ec)
        {
            do_stop();
            notifyConnectionError(*this, ec.message(), ec.value());
        }
        catch (const std::exception& e)
        {
            do_stop();
            notifyConnectionError(*this, e.what(), -1);
        }
    }));
}

void Peer::do_stop()
{
    bRunning = false;
    bHandshakeComplete = false;
    bWriteReady = false;
    notifyClose(*this);
    notifyStop(*this);
}



void Peer::start()

{
    if (bRunning) throw std::runtime_error("Peer already started.");
    boost::unique_lock<boost::shared_mutex> lock(mutex);
    if (bRunning) throw std::runtime_error("Peer already started.");

    bRunning = true;
    bHandshakeComplete = false;
    bWriteReady = false;
    read_message.clear();
    min_read_bytes = MIN_MESSAGE_HEADER_SIZE;

    tcp::resolver::query query(host_, port_);

    resolver_.async_resolve(query, [this](const boost::system::error_code& ec, tcp::resolver::iterator iterator) {
        if (!bRunning) return;
        LOGGER(trace) << "Peer resolve handler." << std::endl;

        if (ec)
        {
            if (ec == boost::asio::error::operation_aborted) return;

            do_stop();

            stringstream err;
            err << "Peer resolve error: " << ec.message();
            notifyConnectionError(*this, err.str(), ec.value());
            return;
        }

        endpoint_ = *iterator;
        strand_.post(boost::bind(&Peer::do_connect, this, iterator)); 
    }); 
}

void Peer::stop()
{
    {
        if (!bRunning) return;
        boost::unique_lock<boost::shared_mutex> lock(mutex);
        if (!bRunning) return;

        bRunning = false;
        keepalive_timer_.cancel();

        boost::system::error_code ec;
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        if (ec)
        {
            stringstream err;
            err << "Peer shutdown error: " << ec.message();
            notifyConnectionError(*this, err.str(), ec.value());
        }

        socket_.close();
        do_clearSendQueue();
        bHandshakeComplete = false;
        bWriteReady = false;
    }

    notifyClose(*this);
    notifyStop(*this);
}

bool Peer::send(Coin::CoinNodeStructure& message)
{
    boost::shared_lock<boost::shared_mutex> runLock(mutex);
    if (!bRunning || !bWriteReady) return false;

    Coin::CoinNodeMessage wrappedMessage(magic_bytes_, &message);
    do_send(wrappedMessage);
    return true;
}

void Peer::do_clearSendQueue()
{
    boost::lock_guard<boost::mutex> sendLock(sendMutex);
    while (!sendQueue.empty()) { sendQueue.pop(); }
}

