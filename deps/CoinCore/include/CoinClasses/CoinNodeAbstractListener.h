////////////////////////////////////////////////////////////////////////////////
//
// CoinNodeAbstractListener.h
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

#ifndef _COIN_NODE_ABSTRACT_LISTENER_H__
#define _COIN_NODE_ABSTRACT_LISTENER_H__

#include "CoinNodeSocket.h"

#include <pthread.h>

namespace Coin
{

const unsigned char DEFAULT_Ipv6[] = {0,0,0,0,0,0,0,0,0,0,255,255,127,0,0,1};

class CoinNodeAbstractListener
{
protected:
    CoinNodeSocket m_nodeSocket;

    uint32_t m_magic;
    uint32_t m_version;

    std::string m_peerHostname;
    uint16_t m_port;

    NetworkAddress m_peerAddress;
    NetworkAddress m_listenerAddress;

    bool m_syncMessages;
    pthread_mutex_t m_handlerLock;

public:
    // set syncMessages to false to allow subclass to handle thread synchronization itself.
    CoinNodeAbstractListener(uint32_t magic, uint32_t version, const std::string& peerHostname, uint16_t port, bool syncMessages = false,
        const unsigned char* peerIpAddress = DEFAULT_Ipv6, const unsigned char* listenerIpAddress = DEFAULT_Ipv6) :
        m_magic(magic),
        m_version(version),
        m_peerHostname(peerHostname),
        m_port(port),
        m_syncMessages(syncMessages)
    {
        m_listenerAddress.set(NODE_NETWORK, listenerIpAddress, port);
        m_peerAddress.set(NODE_NETWORK, peerIpAddress, port);
        m_nodeSocket.setListener(this);
        if (m_syncMessages)
            pthread_mutex_init(&this->m_handlerLock, NULL);
    }

    virtual ~CoinNodeAbstractListener() { this->stop(); }

    virtual int lockHandler() { return (m_syncMessages ? pthread_mutex_lock(&m_handlerLock) : 0); }
    virtual int unlockHandler() { return (m_syncMessages ? pthread_mutex_unlock(&m_handlerLock) : 0); }

    uint32_t getMagic() const { return m_magic; }
    uint32_t getVersion() const { return m_version; }
    const boost::asio::ip::tcp::endpoint& getEndpoint() const { return m_nodeSocket.getEndpoint(); }

    virtual void start();
    virtual void stop() { m_nodeSocket.close(); }
	
    virtual void sendMessage(const CoinNodeMessage& pMessage) { m_nodeSocket.sendMessage(pMessage); }

    virtual void askForBlock(const std::string& hash);
    virtual void getBlocks(const std::vector<std::string>& locatorHashes,
                           const std::string& hashStop = "0000000000000000000000000000000000000000000000000000000000000000");
    virtual void getBlocks(const std::vector<uchar_vector>& locatorHashes,
                           const uchar_vector& hashStop = g_zero32bytes);
    virtual void getHeaders(const std::vector<std::string>& locatorHashes,
                            const std::string& hashStop = "0000000000000000000000000000000000000000000000000000000000000000");
    virtual void getHeaders(const std::vector<uchar_vector>& locatorHashes,
                            const uchar_vector& hashStop = g_zero32bytes); 
    virtual void askForTx(const std::string& hash);
    virtual void askForPeers();
    virtual void askForMempool();

    // Implement the following methods in a derived subclass.
    virtual void onBlock(CoinBlock& block) { };
    virtual void onTx(Transaction& tx) { };
    virtual void onAddr(AddrMessage& addr) { };
    virtual void onHeaders(HeadersMessage& headers) { };

    virtual void onSocketClosed(int code) { };
};

}; // namespace Coin

#endif // _COIN_NODE_LISTENER_H__
