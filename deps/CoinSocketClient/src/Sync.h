////////////////////////////////////////////////////////////////////////////////
//
// CoinSocketClient
//
// Sync.h
//
// Copyright (c) 2014 Eric Lombrozo, all rights reserved
//

#pragma once

#include <CoinDB/Vault.h>
#include <WebSocketAPI/Client.h>

namespace CoinSocket
{
    namespace Client
    {

class Sync
{
public:
    Sync();
    ~Sync();

    void start(int txStatusFlags, const std::string& url);
    void stop();

    Signals::Connection subscribeTx(CoinDB::TxSignal::Slot slot) { m_notifyTx.connect(slot); }

    void sendTx(std::shared_ptr<Tx> tx);

private:
    mutable std::mutex m_connectionMutex;

    WebSocket::Client m_socket;

    CoinDB::TxSignal m_notifyTx;
};

    }
}
