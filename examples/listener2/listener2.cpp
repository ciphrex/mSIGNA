///////////////////////////////////////////////////////////////////////////////
//
// listener2.cpp
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

#include <CoinNodeAbstractListener.h>

#include <iostream>

using namespace std;
using namespace Coin;

// Select the network

#define USE_BITCOIN_NETWORK
//#define USE_LITECOIN_NETWORK

namespace listener_network
{
#if defined(USE_BITCOIN_NETWORK)
    const uint32_t MAGIC_BYTES = 0xd9b4bef9ul;
    const uint32_t PROTOCOL_VERSION = 60003;
    const uint32_t DEFAULT_PORT = 8333;
    const uint8_t  ADDRESS_VERSION = 0x00;
    const uint8_t  MULTISIG_ADDRESS_VERSION = 0x05;
    const char*    NETWORK_NAME = "Bitcoin";
#elif defined(USE_LITECOIN_NETWORK)
    const uint32_t MAGIC_BYTES = 0xdbb6c0fbul;
    const uint32_t PROTOCOL_VERSION = 60002;
    const uint32_t DEFAULT_PORT = 9333;
    const uint8_t  ADDRESS_VERSION = 0x30;
    const uint8_t  MULTISIG_ADDRESS_VERSION = 0x05;
    const char*    NETWORK_NAME = "Litecoin";
#endif
};

class SimpleListener : public CoinNodeAbstractListener
{
private:
    bool bRunning;

public:
    SimpleListener(const string& hostname, uint16_t port)
        : CoinNodeAbstractListener(listener_network::MAGIC_BYTES, listener_network::PROTOCOL_VERSION, hostname, port), bRunning(false) { }
    
    virtual void start() { bRunning = true; CoinNodeAbstractListener::start(); }
    bool isRunning() const { return bRunning; }
    
    virtual void onBlock(CoinBlock& block);
    virtual void onTx(Transaction& tx);
    virtual void onAddr(AddrMessage& addr);
    virtual void onHeaders(HeadersMessage& headers);
    
    virtual void onSocketClosed(int code);
};

void SimpleListener::onBlock(CoinBlock& block)
{
    cout << "-----------------------------------------------------------------------------" << endl
         << "--New Block: " << block.blockHeader.getHashLittleEndian().getHex() << endl
         << "-----------------------------------------------------------------------------" << endl
         << block.toIndentedString() << endl << endl;
}

void SimpleListener::onTx(Transaction& tx)
{
    cout << "--------------------------------------------------------------------------" << endl
         << "--New Tx: " << tx.getHashLittleEndian().getHex() << endl
         << "--------------------------------------------------------------------------" << endl
         << tx.toIndentedString() << endl << endl;
}

void SimpleListener::onAddr(AddrMessage& addr)
{
    cout << "----------------------------------------------------------------------------" << endl
         << "--New Addr: " << addr.getHashLittleEndian().getHex() << endl
         << "----------------------------------------------------------------------------" << endl
         << addr.toIndentedString() << endl << endl;
}

void SimpleListener::onHeaders(HeadersMessage& headers)
{
    cout << "----------------------------------------------------------------------------" << endl
         << "--New Headers: " << headers.getHashLittleEndian().getHex() << endl
         << "----------------------------------------------------------------------------" << endl
         << headers.toIndentedString() << endl << endl;
}

void SimpleListener::onSocketClosed(int code)
{
    cout << "-------------------------------" << endl
         << "--Socket closed with code: " << code << endl
         << "-------------------------------" << endl
         << endl;
    bRunning = false;
}

int main(int argc, char* argv[])
{
    if (argc < 3) {
        cout << "Configured for " << listener_network::NETWORK_NAME << endl
             << "  Usage: " << argv[0] << " <hostname> <port>" << endl
             << "  Example: " << argv[0] << " 127.0.0.1 " << listener_network::DEFAULT_PORT << endl;
        return 0;
    }

    SetAddressVersion(listener_network::ADDRESS_VERSION);
    SetMultiSigAddressVersion(listener_network::MULTISIG_ADDRESS_VERSION);
	
    uint32_t port = strtoul(argv[2], NULL, 0);
    SimpleListener listener(argv[1], port);
    try
    {
        cout << "Starting listener..." << flush;
        listener.start();
        cout << "started." << endl << endl;
        //listener.askForPeers();
        //listener.askForBlock("00000000000000659abaa8a2fd7959af0615bc1a9f1b3e4069b2b52e61d21923");
/*
        vector<uchar_vector> locatorHashes;
        locatorHashes.push_back(uchar_vector("0000000000000014ec7f764280f05a0c1c297de0446f72ea68a493376588316f"));
        locatorHashes.push_back(uchar_vector("00000000000000680fc2144d0572119e48d8683b7fc70f673c5d819f54bbc536"));
        locatorHashes.push_back(uchar_vector("0000000000000087da12fd962dbd6e265b9028a85add866c3ab88e0b7c74e804"));
        listener.getHeaders(locatorHashes);
*/
    }
    catch (const exception& e)
    {
        cout << "Error: " << e.what() << endl << endl;
        return -1;
    }

    while (listener.isRunning()) { sleep(1); }
    return 0;
}
