////////////////////////////////////////////////////////////////////////////////
//
// CoinNodeSocket.cpp
//
// Copyright (c) 2011-2012 Eric Lombrozo
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

#define SOCKET_BUFFER_SIZE 1024
#define MESSAGE_HEADER_SIZE 20
#define COMMAND_SIZE 12

using namespace Coin;
using namespace std;


class recv_exception : public runtime_error
{
private:
    int code;
    
public:
    recv_exception(const std::string& description, int _code) : runtime_error(description), code(_code) { }
    int getCode() const { return code; }
};

int _recv(boost::asio::ip::tcp::socket* pSocket, unsigned char* buf, size_t len, int flags)
{
    boost::system::error_code ec;
    int bytesRecv = boost::asio::read(*pSocket, boost::asio::buffer(buf, len), boost::asio::transfer_at_least(24), ec);
    if (ec) {
        throw recv_exception(ec.message(), ec.value());
    }
    return bytesRecv;
}

void CoinNodeSocket::messageLoop()
{
    try {
#ifdef __DEBUG_OUT__
        fprintf(stdout, "Starting message loop.\n\n");
#endif
        CoinNodeSocket* pNodeSocket = this;
        uchar_vector message;
        uchar_vector magicBytes = uint_to_vch(magic, _BIG_ENDIAN);
#ifdef __DEBUG_OUT__
        fprintf(stdout, "Magic Bytes: %s\n", magicBytes.getHex().c_str());
#endif
        unsigned char command[12];
        uchar_vector payload;
        uint payloadLength = 0;
        uchar_vector checksum;
        uint checksumLength = 0;
        unsigned char receivedData[SOCKET_BUFFER_SIZE];
        uint bytesBuffered = 0;
        while (!bDisconnect) {
            try {  
                // Find magic bytes. all magic bytes must exist in a single frame to be recognized.
                uchar_vector::iterator it;
                while ((it = search(message.begin(), message.end(), magicBytes.begin(), magicBytes.end())) == message.end()) {
                    bytesBuffered = _recv(pSocket, receivedData, SOCKET_BUFFER_SIZE, 0);
                    message = uchar_vector(receivedData, bytesBuffered);
                }
                message.assign(it, message.end()); // remove everything before magic bytes

                // get rest of header
                while (message.size() < MIN_MESSAGE_HEADER_SIZE) {
                    bytesBuffered = _recv(pSocket, receivedData, SOCKET_BUFFER_SIZE, 0);
                    message += uchar_vector(receivedData, bytesBuffered);
                }
                // get command
                uchar_vector(message.begin() + 4, message.begin() + 16).copyToArray(command);

                // get payload length
                payloadLength = vch_to_uint<uint32_t>(uchar_vector(message.begin() + 16, message.begin() + 20), _BIG_ENDIAN);

                // version and verack messages have no checksum - as of Feb 20, 2012, version messages do have a checksum
                /*checksumLength = ((strcmp((char*)command, "version") == 0) ||
                                    (strcmp((char*)command, "verack") == 0)) ? 0 : 4;*/
                // VERSION_CHECKSUM_CHANGE
                checksumLength = (strcmp((char*)command, "verack") == 0) ? 0 : 4;

                // get checksum and payload
                while (message.size() < MIN_MESSAGE_HEADER_SIZE + checksumLength + payloadLength) {
                    bytesBuffered = _recv(pSocket, receivedData, SOCKET_BUFFER_SIZE, 0);
                    message += uchar_vector(receivedData, bytesBuffered);
                }

                CoinNodeMessage nodeMessage(message);

#ifdef __DEBUG_OUT__
                if (bytesBuffered > message.size())
                    fprintf(stdout, "Received extra bytes. Buffer dump:\n%s\n", uchar_vector(receivedData, bytesBuffered).getHex().c_str());
#endif

                if (nodeMessage.isChecksumValid()) {
                    // if it's a verack, signal the completion of the handshake
                    if (string((char*)command) == "verack") {
                        if (!bHandshakeComplete) {
                            boost::unique_lock<boost::mutex> lock(handshakeMutex);
                            bHandshakeComplete = true;
                            lock.unlock();
                            handshakeCond.notify_all();
                        } 
                        //pthread_cond_signal(&pNodeSocket->m_handshakeComplete);
                    }

                    // send the message to callback function.
                    if (coinMessageHandler) {
                        coinMessageHandler(this, pListener, nodeMessage);
                    }
                }
                else
                    throw runtime_error("Checksum does not match payload for message of type.");

                // shift message frame over
                message.assign(message.begin() + MESSAGE_HEADER_SIZE + checksumLength + payloadLength, message.end());
            }
            catch (const recv_exception& e)
            {
#ifdef __SHOW_EXCEPTIONS__
                if (e.getCode() != boost::system::errc::bad_file_descriptor) fprintf(stdout, "recv_exception: %s\n", e.what());
#endif
                close();
                if (socketClosedHandler) socketClosedHandler(pNodeSocket, pListener, e.getCode());
                return;
            }
            catch (const exception& e) {
                message.assign(message.begin() + MESSAGE_HEADER_SIZE + checksumLength + payloadLength, message.end());
#ifdef __SHOW_EXCEPTIONS__
                fprintf(stdout, "Exception: %s\n", e.what());
#endif
            }
        }
    }
    catch (const exception& e) {
#ifdef __SHOW_EXCEPTIONS__
        fprintf(stdout, "Exception: %s\n", e.what());
#endif
    }
}

void CoinNodeSocket::open(CoinMessageHandler callback, uint32_t magic, uint version, const char* hostname, uint port, SocketClosedHandler socketClosedHandler)
{
    boost::unique_lock<boost::mutex> lock(connectionMutex);
    if (pSocket) throw runtime_error("Connection already open.");

    this->coinMessageHandler = callback;
    this->magic = magic;
    this->version = version;
    this->host = hostname;
    this->port = port;
    this->socketClosedHandler = socketClosedHandler;

    std::stringstream ss;
    ss << port;
    tcp::resolver resolver(io_service);
    tcp::resolver::query query(host, ss.str());

    try {
        pSocket = new tcp::socket(io_service);
        boost::asio::connect(*pSocket, resolver.resolve(query));
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
    messageLoopThread.interrupt();
    delete pSocket;
    pSocket = NULL;
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

