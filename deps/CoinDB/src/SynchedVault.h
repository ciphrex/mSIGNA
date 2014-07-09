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
    enum status_t
    {
        NOT_LOADED,
        LOADED,
        STOPPED,
        STARTING,
        FETCHING_HEADERS,
        FETCHING_BLOCKS,
        SYNCHED
    };

    static const std::string getStatusString(status_t status);

    SynchedVault();
    ~SynchedVault();

    void loadBlockTree(const std::string& blockTreeFile, bool bCheckProofOfWork = false);
    bool isBlockTreeLoaded() const { return m_bBlockTreeLoaded; }

    void openVault(const std::string& dbname, bool bCreate = false);
    void openVault(const std::string& dbuser, const std::string& dbpasswd, const std::string& dbname, bool bCreate = false);
    void closeVault();
    Vault* getVault() { return m_vault; }

    void startSync(const std::string& host, const std::string& port);
    void stopSync();
    void suspendBlockUpdates();
    void syncBlocks();
    void updateBloomFilter();

    status_t getStatus() const { return m_status; }
    uint32_t getBestHeight() const { return m_bestHeight; }
    uint32_t getSyncHeight() const { return m_syncHeight; }

    std::shared_ptr<Tx> sendTx(const bytes_t& hash);
    std::shared_ptr<Tx> sendTx(unsigned long tx_id);

    // Sync state events
    typedef Signals::Signal<status_t> StatusSignal;
    Signals::Connection subscribeStatusChanged(StatusSignal::Slot slot) { return m_notifyStatusChanged.connect(slot); }

    // P2P network state events
    Signals::Connection subscribeTxInserted(TxSignal::Slot slot) { return m_notifyTxInserted.connect(slot); }
    Signals::Connection subscribeTxStatusChanged(TxSignal::Slot slot) { return m_notifyTxStatusChanged.connect(slot); }
    Signals::Connection subscribeMerkleBlockInserted(MerkleBlockSignal::Slot slot) { return m_notifyMerkleBlockInserted.connect(slot); }
    void clearAllSlots();

private:
    Vault* m_vault;

    status_t m_status;
    void updateStatus(status_t newStatus);

    CoinQ::Network::NetworkSync m_networkSync;
    std::string m_blockTreeFile;
    bool m_bBlockTreeLoaded;
    bool m_bConnected;
    bool m_bSynching;
    bool m_bBlockTreeSynched;
    bool m_bVaultSynched;
    uint32_t m_bestHeight;
    uint32_t m_syncHeight;

    bool m_bInsertMerkleBlocks;
    mutable std::mutex m_vaultMutex;

    StatusSignal m_notifyStatusChanged;
    TxSignal m_notifyTxInserted;
    TxSignal m_notifyTxStatusChanged;
    MerkleBlockSignal m_notifyMerkleBlockInserted;
};

}
