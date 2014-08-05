///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_peer_io.cpp
//
// Copyright (c) 2012-2013 Eric Lombrozo
//
// All Rights Reserved.

#include "CoinQ_peer_io.h"

#include <logger/logger.h>

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

        if (bHandshakeComplete) return;
        boost::lock_guard<boost::mutex> lock(handshakeMutex);
        if (bHandshakeComplete) return;
    
        do_stop();
        notifyTimeout(*this);
    });
}

void Peer::do_read()
{
    boost::asio::async_read(socket_, boost::asio::buffer(read_buffer, READ_BUFFER_SIZE), boost::asio::transfer_at_least(MIN_MESSAGE_HEADER_SIZE),
    strand_.wrap([this](const boost::system::error_code& ec, std::size_t bytes_read) {
        if (!bRunning) return;
        LOGGER(trace) << "Peer read handler." << std::endl;

        if (ec)
        {
            read_message.clear();
            do_stop();

            std::stringstream err;
            err << "Read error: " << ec.value() << ": " << ec.message();
            notifyConnectionError(*this, err.str());
            return;
        }

        read_message += uchar_vector(read_buffer, bytes_read);

        while (read_message.size() >= MIN_MESSAGE_HEADER_SIZE)
        {
            // Find the first occurrence of the magic bytes, discard anything before it.
            // If magic bytes are not found, read new buffer. 
            // TODO: detect misbehaving node and disconnect.
            uchar_vector::iterator it = std::search(read_message.begin(), read_message.end(), magic_bytes_vector_.begin(), magic_bytes_vector_.end());
            if (it == read_message.end())
            {
                read_message.clear();
                break;
            }

            read_message.assign(it, read_message.end());
            if (read_message.size() < MIN_MESSAGE_HEADER_SIZE) break;

            // Get command
            unsigned char command[13];
            command[12] = 0;
            uchar_vector(read_message.begin() + 4, read_message.begin() + 16).copyToArray(command);

            // Get payload size
            unsigned int payloadSize = vch_to_uint<uint32_t>(uchar_vector(read_message.begin() + 16, read_message.begin() + 20), _BIG_ENDIAN);

            if (read_message.size() < MIN_MESSAGE_HEADER_SIZE  + payloadSize) break;

            try
            {
                Coin::CoinNodeMessage peerMessage(read_message);

                if (!peerMessage.isChecksumValid()) throw std::runtime_error("Invalid checksum.");

                std::string command = peerMessage.getCommand();
                if (command == "verack") {
                    LOGGER(trace) << "Peer verack received." << std::endl;
                    // Signal completion of handshake
                    if (bHandshakeComplete) throw std::runtime_error("Second verack received.");
                    boost::unique_lock<boost::mutex> lock(handshakeMutex);
                    if (bHandshakeComplete) throw std::runtime_error("Second verack received.");
                    timer_.cancel();
                    bHandshakeComplete = true;
                    lock.unlock();
                    bWriteReady = true;
                    notifyOpen(*this);
                }
                else if (command == "version")
                {
                    // TODO: Check version information
                    Coin::VerackMessage verackMessage;
                    Coin::CoinNodeMessage msg(magic_bytes_, &verackMessage);
                    do_send(msg);
                }
                else if (command == "inv")
                {
                    Coin::Inventory* pInventory = static_cast<Coin::Inventory*>(peerMessage.getPayload());
                    notifyInv(*this, *pInventory);
                }
                else if (command == "tx")
                {
                    Coin::Transaction* pTx = static_cast<Coin::Transaction*>(peerMessage.getPayload());
                    notifyTx(*this, *pTx);
                }
                else if (command == "block")
                {
                    Coin::CoinBlock* pBlock = static_cast<Coin::CoinBlock*>(peerMessage.getPayload());
                    notifyBlock(*this, *pBlock);
                }
                else if (command == "merkleblock")
                {
                    Coin::MerkleBlock* pMerkleBlock = static_cast<Coin::MerkleBlock*>(peerMessage.getPayload());
                    notifyMerkleBlock(*this, *pMerkleBlock);
                }
                else if (command == "addr")
                {
                    Coin::AddrMessage* pAddr = static_cast<Coin::AddrMessage*>(peerMessage.getPayload());
                    notifyAddr(*this, *pAddr);
                }
                else if (command == "headers")
                {
                    Coin::HeadersMessage* pHeaders = static_cast<Coin::HeadersMessage*>(peerMessage.getPayload());
                    notifyHeaders(*this, *pHeaders);
                }
                else
                {
                    std::stringstream err;
                    err << "Command type not implemented: " << command;
                    notifyProtocolError(*this, err.str());
                }

                notifyMessage(*this, peerMessage);
            }
            catch (const std::exception& e)
            {
                std::stringstream err;
                err << "Message decode error: " << e.what();
                notifyProtocolError(*this, err.str());
            }

            read_message.assign(read_message.begin() + MIN_MESSAGE_HEADER_SIZE + payloadSize, read_message.end());
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
            std::stringstream err;
            err << "Write error: " << ec.value() << ": " << ec.message();
            notifyConnectionError(*this, err.str());
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
            do_stop();

            std::stringstream err;
            err << "Connect error: " << ec.value() << ": " << ec.message();
            notifyConnectionError(*this, err.str());
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

            std::stringstream err;
            err << "Handshake error: " << ec.value() << ": " << ec.message();
            notifyConnectionError(*this, err.str());
        }
        catch (const std::exception& e)
        {
            do_stop();

            std::stringstream err;
            err << "Handshake error: " << e.what();
            notifyConnectionError(*this, err.str());
        }
    }));
}

void Peer::do_stop()
{
    do_clearSendQueue();
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

    tcp::resolver::query query(host_, port_);

    resolver_.async_resolve(query, [this](const boost::system::error_code& ec, tcp::resolver::iterator iterator) {
        if (!bRunning) return;
        LOGGER(trace) << "Peer resolve handler." << std::endl;

        if (ec)
        {
            do_stop();

            std::stringstream err;
            err << "Resolve error: " << ec.value() << ": " << ec.message();
            notifyConnectionError(*this, err.str());
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
        socket_.cancel();
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

