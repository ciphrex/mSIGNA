///////////////////////////////////////////////////////////////////////////////
//
// SynchedVault.h
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.
//

#pragma once

#include "Vault.h"

#include <Signals.h>

#include <CoinQ_netsync.h>

#include <mutex>

namespace CoinDB
{

typedef Signals::Signal<std::shared_ptr<Tx>>::Slot TxSlot;

class SynchedVault
{
public:
    SynchedVault();
    ~SynchedVault();

    void openVault(const std::string& filename, bool bCreate = false);
    void closeVault();

    void subscribeTxInserted(TxSlot slot) { m_notifyTxInserted.connect(slot); }
    void clearAllSlots();

private:
    Vault* m_vault;
    CoinQ::Network::NetworkSync m_networkSync;

    mutable std::mutex m_mutex;

    Signals::Signal<std::shared_ptr<Tx>> m_notifyTxInserted;
};

}
