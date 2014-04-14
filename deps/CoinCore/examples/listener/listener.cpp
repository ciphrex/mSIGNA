///////////////////////////////////////////////////////////////////////////////
//
// listener.cpp
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

#include <CoinNodeListener.h>

#include <iostream>

using namespace std;
using namespace Coin;

// Change these to use a different network
namespace listener_network
{
    const uint32_t MAGIC_BYTES = 0xd9b4bef9ul;
    const uint32_t PROTOCOL_VERSION = 60002;
    const uint8_t ADDRESS_VERSION = 0x00;
    const uint8_t MULTISIG_ADDRESS_VERSION = 0x05;
};

void onBlock(CoinBlock* pBlock, CoinNodeListener* pListener, void* pInstanceData)
{
    cout << "-----------------------------------------------------------------------------" << endl
         << "--New Block: " << pBlock->blockHeader.getHashLittleEndian().getHex() << endl
         << "-----------------------------------------------------------------------------" << endl
         << pBlock->toIndentedString() << endl << endl;
}

void onTx(Transaction* pTx, CoinNodeListener* pListener, void* pInstanceData)
{
    cout << "--------------------------------------------------------------------------" << endl
         << "--New Tx: " << pTx->getHashLittleEndian().getHex() << endl
         << "--------------------------------------------------------------------------" << endl
         << pTx->toIndentedString() << endl << endl;
}

int main(int argc, char* argv[])
{
    if (argc < 3) {
        cout << "Usage: " << argv[0] << " <hostname> <port>" << endl
             << "Example: " << argv[0] << " 127.0.0.1 8333" << endl;
        return 0;
    }

    SetAddressVersion(listener_network::ADDRESS_VERSION);
    SetMultiSigAddressVersion(listener_network::MULTISIG_ADDRESS_VERSION);
	
    uint32_t port = strtoul(argv[2], NULL, 0);
    CoinNodeListener listener(listener_network::MAGIC_BYTES, argv[1], DEFAULT_Ipv6, port, DEFAULT_Ipv6, listener_network::PROTOCOL_VERSION);
    try
    {
        listener.setInstanceData(NULL);
        listener.setBlockHandler(onBlock);
        listener.setTxHandler(onTx);
        cout << "Starting listener..." << flush;
        listener.start();
        cout << "started." << endl << endl;	
    }
    catch (const exception& e)
    {
        cout << "Error: " << e.what() << endl << endl;
        return -1;
    }

    while (true) { sleep(5000); }
    return 0;
}
