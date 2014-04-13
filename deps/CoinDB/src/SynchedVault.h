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

typedef Signals::Signal<std::shared_ptr<Tx>> TxSignal;
typedef Signals::Signal<std::shared_ptr<MerkleBlock>> MerkleBlockSignal;

class SynchedVault
{
public:
    SynchedVault();
    ~SynchedVault();

    void openVault(const std::string& filename, bool bCreate = false);
    void closeVault();

    void startSync(const std::string& host, const std::string& port);
    void stopSync();
    void resyncVault();
    

    Signals::Connection subscribeTxInserted(TxSignal::Slot slot) { return m_notifyTxInserted.connect(slot); }
    Signals::Connection subscribeMerkleBlockInserted(MerkleBlockSignal::Slot slot) { return m_notifyMerkleBlockInserted.connect(slot); }
    void clearAllSlots();

private:
    Vault* m_vault;

    CoinQ::Network::NetworkSync m_networkSync;
    bool m_bConnected;
    bool m_bSynching;
    bool m_bBlockTreeSynched;
    bool m_bVaultSynched;
    uint32_t m_bestHeight;
    uint32_t m_syncHeight;

    mutable std::mutex m_vaultMutex;

    TxSignal m_notifyTxInserted;
    MerkleBlockSignal m_notifyMerkleBlockInserted;
};

}
