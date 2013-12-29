////////////////////////////////////////////////////////////////////////////////
//
// CoinNodeSocket.h
//
// Copyright (c) 2011 Eric Lombrozo
//

#ifndef COINNODESOCKET_H__
#define COINNODESOCKET_H__

#include "CoinNodeData.h"

#include <string>
#include <iostream>

#include <boost/thread.hpp>
#include <boost/asio.hpp>

namespace Coin {

class CoinNodeSocket;
class CoinNodeAbstractListener;

typedef void (*CoinMessageHandler)(CoinNodeSocket*, CoinNodeAbstractListener*, const Coin::CoinNodeMessage&);
typedef void (*SocketClosedHandler)(CoinNodeSocket*, CoinNodeAbstractListener*, int);

class CoinNodeSocket
{
private:
    // ASIO stuff
    typedef boost::asio::ip::tcp tcp;
    boost::asio::io_service io_service;
    tcp::socket* pSocket;
    tcp::endpoint endpoint;

    // Host and network information
    std::string host;
    uint port;
    uint32_t magic;
    uchar_vector magicBytes;
    uint32_t version;

    // Synchronization objects
    boost::mutex connectionMutex;
    boost::mutex handshakeMutex;
    boost::condition_variable handshakeCond;
    bool bHandshakeComplete;
    bool bDisconnect;

    // Read loop
    boost::thread messageLoopThread;
    void messageLoop();

    // Handlers
    CoinMessageHandler coinMessageHandler;
    SocketClosedHandler socketClosedHandler;

    // Listener for callbacks
    CoinNodeAbstractListener* pListener;

public:
    CoinNodeSocket() : pSocket(NULL) { }
    ~CoinNodeSocket() { this->close(); }

    void setListener(CoinNodeAbstractListener* _pListener) { pListener = _pListener; }
    uint32_t getMagic() const { return magic; }
    SocketClosedHandler getSocketClosedHandler() const { return socketClosedHandler; }

    void open(CoinMessageHandler callback, uint32_t magic, uint32_t version, const char* hostname = "127.0.0.1", uint port = 8333,
              SocketClosedHandler socketClosedHandler = NULL);
    void close();

    void doHandshake(
        int32_t version,
        uint64_t services,
        int64_t timestamp,
        const Coin::NetworkAddress& recipientAddress,
        const Coin::NetworkAddress& senderAddress,
        uint64_t nonce,
        const char* subVersion,
        int32_t startHeight
    );

    void waitOnHandshakeComplete();

    void sendMessage(unsigned char* command, const std::vector<unsigned char>& payload);
    void sendMessage(const Coin::CoinNodeMessage& pMessage);

    const tcp::endpoint& getEndpoint() const { return endpoint; }
};

}; // namespace Coin
#endif
