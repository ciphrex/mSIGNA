////////////////////////////////////////////////////////////////////////////////
//
// CoinNodeListener.cpp
//
// Copyright (c) 2012 Eric Lombrozo
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

#include "CoinNodeListener.h"

using namespace Coin;

uint64_t getRandomNonce64()
{
    // TODO: Use better RNG
    srand(time(NULL));
    uint64_t nonce = 0;
    for (uint i = 0; i < 4; i++) {
        nonce <<= 8;
        nonce |= rand() % 0xff;
    }
    return nonce;
}

void coinMessageHandler(CoinNodeSocket* pNodeSocket, const CoinNodeMessage& message)
{
    FullInstanceData* pFullInstanceData = static_cast<FullInstanceData*>(pNodeSocket->pAppData);
	
    try {
        if (std::string(message.getCommand()) == "version") {
            VerackMessage verackMessage;
            CoinNodeMessage msg(pNodeSocket->getMagic(), &verackMessage);
            pNodeSocket->sendMessage(msg);
        }
        else if (std::string(message.getCommand()) == "inv") {
            Inventory* pInventory = static_cast<Inventory*>(message.getPayload());
            GetDataMessage getData(*pInventory);
            CoinNodeMessage msg(pNodeSocket->getMagic(), &getData);
            pNodeSocket->sendMessage(msg);
        }
        else if (std::string(message.getCommand()) == "tx") {
            if (pFullInstanceData->coinTxHandler) {
                Transaction* pTx = static_cast<Transaction*>(message.getPayload());
                pFullInstanceData->coinTxHandler(pTx, pFullInstanceData->pListener, pFullInstanceData->pInstanceData);
            }
        }
        else if (std::string(message.getCommand()) == "block") {
            if (pFullInstanceData->coinBlockHandler) {
                CoinBlock* pBlock = static_cast<CoinBlock*>(message.getPayload());
                pFullInstanceData->coinBlockHandler(pBlock, pFullInstanceData->pListener, pFullInstanceData->pInstanceData);
            }
        }
    }
    catch (const std::exception& e) {
        std::cout << "Exception in coinMessageHandler(): " << e.what() << std::endl;
    }
}