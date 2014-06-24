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

#include <Signals/Signals.h>

#include <CoinQ/CoinQ_netsync.h>

#include <mutex>

namespace CoinDB
{

class SynchedVault
{
public:
    SynchedVault(const std::string& blockTreeFile = "blocktree.dat");
    ~SynchedVault();

    void openVault(const std::string& filename, bool bCreate = false);
    void closeVault();
    Vault* getVault() { return m_vault; }

    void startSync(const std::string& host, const std::string& port);
    void stopSync();
    void resyncVault();
    void updateBloomFilter();

    void sendTx(const bytes_t& hash);
    void sendTx(unsigned long tx_id);

    // P2P network state events
    Signals::Connection subscribeTxInserted(TxSignal::Slot slot) { return m_notifyTxInserted.connect(slot); }
    Signals::Connection subscribeTxStatusChanged(TxSignal::Slot slot) { return m_notifyTxStatusChanged.connect(slot); }
    Signals::Connection subscribeMerkleBlockInserted(MerkleBlockSignal::Slot slot) { return m_notifyMerkleBlockInserted.connect(slot); }
    void clearAllSlots();

private:
    Vault* m_vault;

    CoinQ::Network::NetworkSync m_networkSync;
    std::string m_blockTreeFile;
    bool m_bConnected;
    bool m_bSynching;
    bool m_bBlockTreeSynched;
    bool m_bVaultSynched;
    uint32_t m_bestHeight;
    uint32_t m_syncHeight;

    mutable std::mutex m_vaultMutex;

    TxSignal m_notifyTxInserted;
    TxSignal m_notifyTxStatusChanged;
    MerkleBlockSignal m_notifyMerkleBlockInserted;
};

}
