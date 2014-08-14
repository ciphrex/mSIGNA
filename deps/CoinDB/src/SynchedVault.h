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
        STOPPED,
        STARTING,
        SYNCHING_HEADERS,
        SYNCHING_BLOCKS,
        SYNCHED
    };

    static const std::string getStatusString(status_t status);

    SynchedVault(const CoinQ::CoinParams& coinParams = CoinQ::getBitcoinParams());
    ~SynchedVault();

    void loadHeaders(const std::string& blockTreeFile, bool bCheckProofOfWork = false, CoinQBlockTreeMem::callback_t callback = nullptr);
    bool areHeadersLoaded() const { return m_bBlockTreeLoaded; }

    void openVault(const std::string& dbname, bool bCreate = false);
    void openVault(const std::string& dbuser, const std::string& dbpasswd, const std::string& dbname, bool bCreate = false);
    void closeVault();
    bool isVaultOpen() const { return (m_vault != nullptr); }
    Vault* getVault() const { return m_vault; }

    void startSync(const std::string& host, const std::string& port);
    void startSync(const std::string& host, int port);
    void stopSync();
    bool isConnected() const { return m_networkSync.connected(); }
    void suspendBlockUpdates();
    void syncBlocks();

    void setFilterParams(double falsePositiveRate, uint32_t nTweak, uint8_t nFlags);
    void updateBloomFilter();

    status_t getStatus() const { return m_status; }
    uint32_t getBestHeight() const { return m_bestHeight; }
    uint32_t getSyncHeight() const { return m_syncHeight; }

    std::shared_ptr<Tx> sendTx(const bytes_t& hash);
    std::shared_ptr<Tx> sendTx(unsigned long tx_id);
    void sendTx(Coin::Transaction& coin_tx);

    // For testing
    void insertFakeMerkleBlock(unsigned int nExtraLeaves = 0);

    // Signal types
    typedef Signals::Signal<>                           VoidSignal;
    typedef Signals::Signal<Vault*>                     VaultSignal;
    typedef Signals::Signal<const std::string&, int>    ErrorSignal;
    typedef Signals::Signal<status_t>                   StatusSignal;
    typedef Signals::Signal<uint32_t>                   HeightSignal;

    // Vault state events
    Signals::Connection subscribeVaultOpened(VaultSignal::Slot slot) { return m_notifyVaultOpened.connect(slot); }
    Signals::Connection subscribeVaultClosed(VoidSignal::Slot slot) { return m_notifyVaultClosed.connect(slot); }
    Signals::Connection subscribeVaultError(ErrorSignal::Slot slot) { return m_notifyVaultError.connect(slot); }
    Signals::Connection subscribeKeychainUnlocked(KeychainUnlockedSignal::Slot slot) { return m_notifyKeychainUnlocked.connect(slot); }
    Signals::Connection subscribeKeychainLocked(KeychainLockedSignal::Slot slot) { return m_notifyKeychainLocked.connect(slot); }

    // Sync state events
    Signals::Connection subscribeStatusChanged(StatusSignal::Slot slot) { return m_notifyStatusChanged.connect(slot); }
    Signals::Connection subscribeBestHeightChanged(HeightSignal::Slot slot) { return m_notifyBestHeightChanged.connect(slot); }
    Signals::Connection subscribeSyncHeightChanged(HeightSignal::Slot slot) { return m_notifySyncHeightChanged.connect(slot); }
    Signals::Connection subscribeConnectionError(ErrorSignal::Slot slot) { return m_notifyConnectionError.connect(slot); }
    Signals::Connection subscribeBlockTreeError(ErrorSignal::Slot slot) { return m_notifyBlockTreeError.connect(slot); }

    // P2P network state events
    Signals::Connection subscribeTxInserted(TxSignal::Slot slot) { return m_notifyTxInserted.connect(slot); }
    Signals::Connection subscribeTxUpdated(TxSignal::Slot slot) { return m_notifyTxUpdated.connect(slot); }
    Signals::Connection subscribeMerkleBlockInserted(MerkleBlockSignal::Slot slot) { return m_notifyMerkleBlockInserted.connect(slot); }
    Signals::Connection subscribeTxInsertionError(TxErrorSignal::Slot slot) { return m_notifyTxInsertionError.connect(slot); }
    Signals::Connection subscribeMerkleBlockInsertionError(MerkleBlockErrorSignal::Slot slot) { return m_notifyMerkleBlockInsertionError.connect(slot); }
    Signals::Connection subscribeTxConfirmationError(TxConfirmationErrorSignal::Slot slot) { return m_notifyTxConfirmationError.connect(slot); }
    Signals::Connection subscribeProtocolError(ErrorSignal::Slot slot) { return m_notifyProtocolError.connect(slot); }

    void clearAllSlots();

private:
    friend class VaultLock;

    mutable std::mutex          m_vaultMutex;
    Vault*                      m_vault;

    status_t                    m_status;
    void                        updateStatus(status_t newStatus);

    // Height of best known chain
    uint32_t                    m_bestHeight;
    void                        updateBestHeight(uint32_t bestHeight);

    // Height of most recent block stored in vault
    uint32_t                    m_syncHeight;
    void                        updateSyncHeight(uint32_t syncHeight);

    // Bloom filter parameters
    double                      m_filterFalsePositiveRate;
    uint32_t                    m_filterTweak;
    uint8_t                     m_filterFlags;

    CoinQ::Network::NetworkSync m_networkSync;
    std::string                 m_blockTreeFile;
    bool                        m_bBlockTreeLoaded;
    bool                        m_bConnected;
    bool                        m_bSynching;
    bool                        m_bBlockTreeSynched;
    bool                        m_bGotMempool;

    bool                        m_bInsertMerkleBlocks;

    // Vault state events
    VaultSignal                 m_notifyVaultOpened;
    VoidSignal                  m_notifyVaultClosed;
    ErrorSignal                 m_notifyVaultError;
    KeychainUnlockedSignal      m_notifyKeychainUnlocked;
    KeychainLockedSignal        m_notifyKeychainLocked;

    // Sync state events
    StatusSignal                m_notifyStatusChanged;
    HeightSignal                m_notifyBestHeightChanged;
    HeightSignal                m_notifySyncHeightChanged;
    ErrorSignal                 m_notifyConnectionError;
    ErrorSignal                 m_notifyBlockTreeError;

    // P2P network state events
    TxSignal                    m_notifyTxInserted;
    TxSignal                    m_notifyTxUpdated;
    MerkleBlockSignal           m_notifyMerkleBlockInserted;
    TxErrorSignal               m_notifyTxInsertionError;
    MerkleBlockErrorSignal      m_notifyMerkleBlockInsertionError;
    TxConfirmationErrorSignal   m_notifyTxConfirmationError;
    ErrorSignal                 m_notifyProtocolError;
};

class VaultLock
{
public:
    explicit VaultLock(const SynchedVault& synchedVault) : m_lock(synchedVault.m_vaultMutex) { }

private:
    std::lock_guard<std::mutex> m_lock;
};

}
