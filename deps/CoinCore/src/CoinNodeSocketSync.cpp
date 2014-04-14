////////////////////////////////////////////////////////////////////////////////
//
// CoinNodeSocketSync.cpp
//
// Copyright (c) 2011-2013 Eric Lombrozo
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

//#define __DEBUG_OUT__
#define __SHOW_EXCEPTIONS__

#include <CoinNodeSocket.h>
#include <numericdata.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sstream>
#include <stdexcept>

#include <boost/thread/locks.hpp>

#define READ_BUFFER_SIZE 1024
#define MESSAGE_HEADER_SIZE 20
#define COMMAND_SIZE 12

using namespace Coin;
using namespace std;


void CoinNodeSocket::messageLoop()
{
    unsigned char read_buffer[READ_BUFFER_SIZE];
    unsigned char command[12];
    uchar_vector message;
    unsigned int payloadSize;
    unsigned int checksumSize;
    bool isVerack;
    boost::system::error_code ec;

    while (!bDisconnect) {
        try {
            int bytes_read = boost::asio::read(*pSocket, boost::asio::buffer(read_buffer, READ_BUFFER_SIZE), boost::asio::transfer_at_least(MIN_MESSAGE_HEADER_SIZE), ec);
            if (bDisconnect) break;
            if (ec) {
                boost::unique_lock<boost::mutex> lock(connectionMutex);
                if (!bDisconnect) {
                    bDisconnect = true;
                    if (pSocket) {
                        delete pSocket;
                        pSocket = NULL;
                    }
                } 
                break;
            }
            if (bytes_read == 0) {
                std::cout << "WARNING: CoinNodeSocket::messageLoop() - 0 bytes read." << std::endl;
                continue;
            }

            message += uchar_vector(read_buffer, bytes_read);

            while (message.size() >= MIN_MESSAGE_HEADER_SIZE) {
                // Find the first occurrence of the magic bytes, discard anything before it.
                // If magic bytes are not found, read new buffer. (TODO: detect misbehaving node and disconnect.)
                uchar_vector::iterator it = std::search(message.begin(), message.end(), magicBytes.begin(), magicBytes.end());
                if (it == message.end()) {
                    message.clear();
                    break;
                }
                message.assign(it, message.end());
                if (message.size() < MIN_MESSAGE_HEADER_SIZE) {
                    break;
                }

                // Get command
                uchar_vector(message.begin() + 4, message.begin() + 16).copyToArray(command);

                // Get payload size
                payloadSize = vch_to_uint<uint32_t>(uchar_vector(message.begin() + 16, message.begin() + 20), _BIG_ENDIAN);

                // Get checksum size
                isVerack = (strcmp((char*)command, "verack") == 0);
                checksumSize = isVerack ? 0 : 4;

                if (message.size() < MIN_MESSAGE_HEADER_SIZE  + payloadSize + checksumSize) {
                    break;
                }

                CoinNodeMessage nodeMessage(message);
                message.assign(message.begin() + MIN_MESSAGE_HEADER_SIZE + payloadSize + checksumSize, message.end());

                if (isVerack) {
                    // Signal completion of handshake
                    // TODO: Check verack, make sure peer satisfies our needs.
                    if (!bHandshakeComplete) {
                        boost::unique_lock<boost::mutex> lock(handshakeMutex);
                        // Double check
                        if (!bHandshakeComplete) {
                            bHandshakeComplete = true;
                            lock.unlock();
                            handshakeCond.notify_all();
                        }
                    }
                }
                else {
                    if (!nodeMessage.isChecksumValid()) {
                        throw std::runtime_error("CoinNodeSocket::messageLoop() - Error: Checksum does not match payload for message.");
                    }
                }

                if (coinMessageHandler) {
                    coinMessageHandler(this, pListener, nodeMessage);
                }
            }
        }
        catch (const std::exception& e) {
            message.clear();
            std::cout << "CoinNodeSocket:messageLoop() - Exception: " << e.what() << std::endl;
        }
    }

    if (socketClosedHandler) socketClosedHandler(this, pListener, ec.value());
}

void CoinNodeSocket::open(CoinMessageHandler callback, uint32_t magic, uint version, const char* hostname, uint port, SocketClosedHandler socketClosedHandler)
{
    boost::unique_lock<boost::mutex> lock(connectionMutex);
    if (pSocket) throw runtime_error("Connection already open.");

    this->coinMessageHandler = callback;
    this->magic = magic;
    this->magicBytes = uint_to_vch(magic, _BIG_ENDIAN);
    this->version = version;
    this->host = hostname;
    this->port = port;
    this->socketClosedHandler = socketClosedHandler;

    try {
        std::stringstream ss;
        ss << port;
        tcp::resolver resolver(io_service);
        tcp::resolver::query query(host, ss.str());
        tcp::resolver::iterator iter = resolver.resolve(query);
        this->endpoint = *iter;

        pSocket = new tcp::socket(io_service);
        boost::asio::connect(*pSocket, iter);
    }
    catch (const boost::system::error_code& ec) {
        if (pSocket) delete pSocket;
        pSocket = NULL;
        throw std::runtime_error(std::string("Boost error: ") + ec.message());
    }
    catch (const std::exception& e) {
        if (pSocket) delete pSocket;
        pSocket = NULL;
        throw std::runtime_error(e.what());
    }

    bDisconnect = false;
    bHandshakeComplete = false;

    messageLoopThread = boost::thread(&CoinNodeSocket::messageLoop, this);
}

void CoinNodeSocket::close()
{
    boost::unique_lock<boost::mutex> lock(connectionMutex);

    if (!pSocket) return;
    bDisconnect = true;
    delete pSocket;
    pSocket = NULL;
    messageLoopThread.join();
}

void CoinNodeSocket::doHandshake(
    int32_t version,
    uint64_t services,
    int64_t timestamp,
    const NetworkAddress& recipientAddress,
    const NetworkAddress& senderAddress,
    uint64_t nonce,
    const char* subVersion,
    int32_t startHeight
)
{
    boost::unique_lock<boost::mutex> lock(handshakeMutex);
    if (bHandshakeComplete) return;

    VersionMessage versionMessage(version, services, timestamp, recipientAddress, senderAddress, nonce, subVersion, startHeight);
    CoinNodeMessage messagePacket(magic, &versionMessage);
    this->sendMessage(messagePacket);
}

void CoinNodeSocket::waitOnHandshakeComplete()
{
    boost::system_time const timeout = boost::get_system_time() + boost::posix_time::milliseconds(5000);
    boost::unique_lock<boost::mutex> lock(handshakeMutex);
    while (!bHandshakeComplete) {
        if (!handshakeCond.timed_wait(lock, timeout)) {
            close();
            throw runtime_error("Handshake timed out.");
        }
    }
}

void CoinNodeSocket::sendMessage(const CoinNodeMessage& message)
{
    boost::lock_guard<boost::mutex> lock(connectionMutex);
    if (!pSocket) throw runtime_error("Socket is not open.");
 
    std::vector<unsigned char> rawData = message.getSerialized();

#ifdef __DEBUG_OUT__
    fprintf(stdout, "Sending message: %s\n", message.toString().c_str());
    fprintf(stdout, "Raw data:\n%s\n", uchar_vector(rawData).getHex().c_str());
#endif

    std::vector<uint8_t> buffer = rawData;
    boost::asio::write(*pSocket, boost::asio::buffer(rawData));
}

