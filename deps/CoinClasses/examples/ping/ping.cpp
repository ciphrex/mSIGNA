///////////////////////////////////////////////////////////////////////////////
//
// ping.cpp
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

#include <CoinNodeSocket.h>

#include <iostream>
#include <string>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/condition_variable.hpp>

using namespace Coin;
using namespace std;

// Change these to use a different network
namespace listener_network
{
    const uint32_t MAGIC_BYTES = 0xd9b4bef9ul;
    const uint32_t PROTOCOL_VERSION = 60002;
    const uint8_t ADDRESS_VERSION = 0x00;
    const uint8_t MULTISIG_ADDRESS_VERSION = 0x05;
};

const unsigned char DEFAULT_Ipv6[] = {0,0,0,0,0,0,0,0,0,0,255,255,127,0,0,1};

boost::mutex mutex;
boost::condition_variable gotVersion;
bool bGotVersion = false;

void MessageHandler(CoinNodeSocket* pNodeSocket, const CoinNodeMessage& message)
{
    if (string(message.getCommand()) == "version") {
        cout << message.toIndentedString() << endl;
        boost::unique_lock<boost::mutex> lock(mutex);
        bGotVersion = true;
        gotVersion.notify_all();
    }
}

int main(int argc, char* argv[])
{
    if (argc < 3) {
        cout << "Usage: " << argv[0] << " <host> <port> [magic_bytes=0xd9b4bef9ul]" << endl
             << "  Pings a bitcoin-like node and gets the version information." << endl;
        return 0;
    }
    uint16_t port = strtoul(argv[2], NULL, 10);

    NetworkAddress address(NODE_NETWORK, DEFAULT_Ipv6, port);

    CoinNodeSocket nodeSocket;

    try {
        nodeSocket.open(MessageHandler, listener_network::MAGIC_BYTES, listener_network::PROTOCOL_VERSION, argv[1], port);
        nodeSocket.doHandshake(listener_network::PROTOCOL_VERSION, NODE_NETWORK, time(NULL), address, address, 0, "", 0);
        nodeSocket.waitOnHandshakeComplete();

        boost::unique_lock<boost::mutex> lock(mutex);
        while (!bGotVersion) gotVersion.wait(lock);
    }
    catch (const exception& e) {
        cout << e.what() << endl;
    }
}