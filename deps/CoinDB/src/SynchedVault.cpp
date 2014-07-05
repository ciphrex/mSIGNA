///////////////////////////////////////////////////////////////////////////////
//
// SynchedVault.cpp
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.
//

#include "SynchedVault.h"

#include <logger/logger.h>

using namespace CoinDB;
using namespace CoinQ;

const std::string SynchedVault::getStatusString(status_t status)
{
    switch (status)
    {
    case NOT_LOADED:
        return "NOT_LOADED";
    case SYNC_STOPPED:
        return "SYNC_STOPPED";
    case SYNCHING:
        return "SYNCHING";
    case READY:
        return "READY";
    default:
        return "UNKNOWN";
    } 
}

// Constructor
SynchedVault::SynchedVault() :
    m_vault(nullptr),
    m_status(NOT_LOADED),
    m_bBlockTreeLoaded(false),
    m_bConnected(false),
    m_bSynching(false),
    m_bBlockTreeSynched(false),
    m_bVaultSynched(false),
    m_bestHeight(0),
    m_syncHeight(0)
{
    LOGGER(trace) << "SynchedVault::SynchedVault()" << std::endl;

    m_networkSync.subscribeStatus([this](const std::string& message)
    {
        LOGGER(trace) << "P2P network status: " << message << std::endl;
    });

    m_networkSync.subscribeError([this](const std::string& error)
    {
        LOGGER(trace) << "P2P network error: " << error << std::endl;
    });

    m_networkSync.subscribeOpen([this]()
    {
        LOGGER(trace) << "P2P network connection opened." << std::endl;
        m_bConnected = true;
    });

    m_networkSync.subscribeClose([this]()
    {
        LOGGER(trace) << "P2P network connection closed." << std::endl;
        m_bConnected = false;
        m_bSynching = false;
        updateStatus(SYNC_STOPPED);
    });

    m_networkSync.subscribeStarted([this]()
    {
        LOGGER(trace) << "P2P network sync started." << std::endl;
        m_bSynching = true;
        updateStatus(SYNCHING);
        //TODO: notify clients
    });

    m_networkSync.subscribeStopped([this]()
    {
        LOGGER(trace) << "P2P network sync stopped." << std::endl;
        m_bSynching = false;
        updateStatus(SYNC_STOPPED);
        //TODO: notify clients
    });

    m_networkSync.subscribeTimeout([this]()
    {
        LOGGER(trace) << "P2P network sync timeout." << std::endl;
        m_bConnected = false;
        m_bSynching = false;
        // TODO: notify clients
    });

    m_networkSync.subscribeHeadersSynched([this]()
    {
        LOGGER(trace) << "P2P headers sync complete." << std::endl;
        m_bBlockTreeSynched = true;
        // TODO: notify clients

        try
        {
            LOGGER(trace) << "Attempting to sync blocks." << std::endl;
            syncBlocks();
        }
        catch (const std::exception& e)
        {
            LOGGER(error) << e.what() << std::endl;
        }
    });

    m_networkSync.subscribeBlocksSynched([this]()
    {
        LOGGER(trace) << "Block sync complete." << std::endl;
        LOGGER(info) << "Fetching mempool." << std::endl;
        m_networkSync.getMempool();
        updateStatus(READY);
    });

    m_networkSync.subscribeAddBestChain([this](const chain_header_t& header)
    {
        LOGGER(trace) << "P2P network added best chain. New best height: " << header.height << std::endl;
        m_bestHeight = header.height;
        // TODO: notify clients
    });

    m_networkSync.subscribeRemoveBestChain([this](const chain_header_t& header)
    {
        LOGGER(trace) << "P2P network removed best chain." << std::endl;
        int diff = m_bestHeight - m_networkSync.getBestHeight();
        if (diff >= 0)
        {
            LOGGER(trace) << "Reorganizing " << (diff + 1) << " blocks." << std::endl;
            try
            {
                m_bestHeight = m_networkSync.getBestHeight();
                // TODO: notify clients
            }
            catch (const std::exception& e)
            {
                LOGGER(error) << e.what() << std::endl;
            }
        }
    });

    m_networkSync.subscribeTx([this](const Coin::Transaction& coinTx)
    {
        LOGGER(trace) << "P2P network received transaction " << coinTx.getHashLittleEndian().getHex() << std::endl;

        if (!m_vault) return;
        std::lock_guard<std::mutex> lock(m_vaultMutex);
        if (!m_vault) return;

        try
        {
            std::shared_ptr<Tx> tx(new Tx());
            tx->set(coinTx);
            m_vault->insertTx(tx);
        }
        catch (const std::exception& e)
        {
            LOGGER(error) << e.what() << std::endl;
        } 
    });

    m_networkSync.subscribeMerkleBlock([this](const ChainMerkleBlock& chainMerkleBlock)
    {
        LOGGER(trace) << "P2P network received merkle block " << chainMerkleBlock.blockHeader.getHashLittleEndian().getHex() << " height: " << chainMerkleBlock.height << std::endl;

        if (!m_vault) return;
        if (!m_bInsertMerkleBlocks) return;
        std::lock_guard<std::mutex> lock(m_vaultMutex);
        if (!m_vault) return;
        if (!m_bInsertMerkleBlocks) return;

        try
        {
            std::shared_ptr<MerkleBlock> merkleblock(new MerkleBlock(chainMerkleBlock));
            m_vault->insertMerkleBlock(merkleblock);
        }
        catch (const std::exception& e)
        {
            LOGGER(error) << e.what() << std::endl;
        }
    });

    m_networkSync.subscribeBlockTreeChanged([this]()
    {
        LOGGER(trace) << "P2P network block tree changed." << std::endl;
        m_bBlockTreeSynched = false;
        m_bestHeight = m_networkSync.getBestHeight();
        // TODO: update state and notify clients
    });
}

// Destructor
SynchedVault::~SynchedVault()
{
    LOGGER(trace) << "SynchedVault::~SynchedVault()" << std::endl;
    stopSync();
    closeVault();
}

// Block tree operations
void SynchedVault::loadBlockTree(const std::string& blockTreeFile, bool bCheckProofOfWork)
{
    LOGGER(trace) << "SynchedVault::loadBlockTree(" << blockTreeFile << ", " << (bCheckProofOfWork ? "true" : "false") << ")" << std::endl;

    m_blockTreeFile = blockTreeFile;
    m_bBlockTreeLoaded = false;
    m_networkSync.loadHeaders(blockTreeFile, bCheckProofOfWork);
    m_bBlockTreeLoaded = true;
}

// Vault operations
void SynchedVault::openVault(const std::string& dbname, bool bCreate)
{
    LOGGER(trace) << "SynchedVault::openVault(" << dbname << ", " << (bCreate ? "true" : "false") << ")" << std::endl;
    std::lock_guard<std::mutex> lock(m_vaultMutex);
    if (m_vault) delete m_vault;
    m_vault = new Vault(dbname, bCreate);
    m_networkSync.setBloomFilter(m_vault->getBloomFilter(0.001, 0, 0));
    m_vault->subscribeTxInserted([this](std::shared_ptr<Tx> tx) { m_notifyTxInserted(tx); });
    m_vault->subscribeTxStatusChanged([this](std::shared_ptr<Tx> tx) { m_notifyTxStatusChanged(tx); });
    m_vault->subscribeMerkleBlockInserted([this](std::shared_ptr<MerkleBlock> merkleblock)
    {
        m_syncHeight = merkleblock->blockheader()->height();
        m_notifyMerkleBlockInserted(merkleblock);
    });

    if (m_networkSync.connected())      { updateStatus(SYNCHING); }
    else                                { updateStatus(SYNC_STOPPED); }
}

void SynchedVault::openVault(const std::string& dbuser, const std::string& dbpasswd, const std::string& dbname, bool bCreate)
{
    LOGGER(trace) << "SynchedVault::openVault(" << dbuser << ", ..., " << dbname << ", " << (bCreate ? "true" : "false") << ")" << std::endl;
    std::lock_guard<std::mutex> lock(m_vaultMutex);
    if (m_vault) delete m_vault;
    m_vault = new Vault(dbuser, dbpasswd, dbname, bCreate);
    m_networkSync.setBloomFilter(m_vault->getBloomFilter(0.001, 0, 0));
    m_vault->subscribeTxInserted([this](std::shared_ptr<Tx> tx) { m_notifyTxInserted(tx); });
    m_vault->subscribeTxStatusChanged([this](std::shared_ptr<Tx> tx) { m_notifyTxStatusChanged(tx); });
    m_vault->subscribeMerkleBlockInserted([this](std::shared_ptr<MerkleBlock> merkleblock)
    {
        m_syncHeight = merkleblock->blockheader()->height();
        m_notifyMerkleBlockInserted(merkleblock);
    });

    if (m_networkSync.connected())      { updateStatus(SYNCHING); }
    else                                { updateStatus(SYNC_STOPPED); }
}

void SynchedVault::closeVault()
{
    LOGGER(trace) << "SynchedVault::closeVault()" << std::endl;
    suspendBlockUpdates();
    std::lock_guard<std::mutex> lock(m_vaultMutex);
    if (m_vault)
    {
        m_vault->clearAllSlots();
        delete m_vault;
        m_vault = nullptr;
        updateStatus(NOT_LOADED);
    }
}

// Peer to peer network operations
void SynchedVault::startSync(const std::string& host, const std::string& port)
{
    LOGGER(trace) << "SynchedVault::startSync(" << host << ", " << port << ")" << std::endl;
    m_bInsertMerkleBlocks = false;
    m_networkSync.start(host, port); 
}

void SynchedVault::stopSync()
{
    LOGGER(trace) << "SynchedVault::stopSync()" << std::endl;
    m_networkSync.stop();
}

void SynchedVault::suspendBlockUpdates()
{
    LOGGER(trace) << "SynchedVault::suspendBlockUpdates()" << std::endl;

    if (!m_vault) return;
    if (!m_bInsertMerkleBlocks) return;
    std::lock_guard<std::mutex> lock(m_vaultMutex);
    m_bInsertMerkleBlocks = false;
}

void SynchedVault::syncBlocks()
{
    LOGGER(trace) << "SynchedVault::resyncVault()" << std::endl;
    if (!m_bConnected) throw std::runtime_error("Not connected.");

    if (!m_vault) throw std::runtime_error("No vault is open.");
    std::lock_guard<std::mutex> lock(m_vaultMutex);
    if (!m_vault) throw std::runtime_error("No vault is open.");

    uint32_t startTime = m_vault->getMaxFirstBlockTimestamp();
    if (startTime == 0)
    {
        m_bInsertMerkleBlocks = false;
        return;
    }

    std::vector<bytes_t> locatorHashes = m_vault->getLocatorHashes();
    m_bInsertMerkleBlocks = true;
    m_networkSync.syncBlocks(locatorHashes, startTime);
}

void SynchedVault::updateBloomFilter()
{
    LOGGER(trace) << "SynchedVault::updateBloomFilter()" << std::endl;
    if (!m_bConnected) throw std::runtime_error("Not connected.");

    if (!m_vault) throw std::runtime_error("No vault is open.");
    std::lock_guard<std::mutex> lock(m_vaultMutex);
    if (!m_vault) throw std::runtime_error("No vault is open.");

    m_networkSync.setBloomFilter(m_vault->getBloomFilter(0.001, 0, 0));
}

std::shared_ptr<Tx> SynchedVault::sendTx(const bytes_t& hash)
{
    LOGGER(trace) << "SynchedVault::sendTx(" << uchar_vector(hash).getHex() << ")" << std::endl;
    if (!m_bConnected) throw std::runtime_error("Not connected.");

    if (!m_vault) throw std::runtime_error("No vault is open.");
    std::lock_guard<std::mutex> lock(m_vaultMutex);
    if (!m_vault) throw std::runtime_error("No vault is open.");

    std::shared_ptr<Tx> tx = m_vault->getTx(hash);
    if (tx->status() == Tx::UNSIGNED)
        throw std::runtime_error("Transaction is missing signatures.");

    Coin::Transaction coin_tx = tx->toCoinCore();
    m_networkSync.sendTx(coin_tx);
    uchar_vector txhash(tx->hash());
    m_networkSync.getTx(txhash); // To ensure propagation

    return tx;
}

std::shared_ptr<Tx> SynchedVault::sendTx(unsigned long tx_id)
{
    LOGGER(trace) << "SynchedVault::sendTx(" << tx_id << ")" << std::endl;
    if (!m_bConnected) throw std::runtime_error("Not connected.");

    if (!m_vault) throw std::runtime_error("No vault is open.");
    std::lock_guard<std::mutex> lock(m_vaultMutex);
    if (!m_vault) throw std::runtime_error("No vault is open.");

    std::shared_ptr<Tx> tx = m_vault->getTx(tx_id);
    if (tx->status() == Tx::UNSIGNED)
        throw std::runtime_error("Transaction is missing signatures.");

    Coin::Transaction coin_tx = tx->toCoinCore();
    m_networkSync.sendTx(coin_tx);
    uchar_vector txhash(tx->hash());
    m_networkSync.getTx(txhash); // To ensure propagation

    return tx;
}


// Event subscriptions
void SynchedVault::clearAllSlots()
{
    LOGGER(trace) << "SynchedVault::clearAllSlots()" << std::endl;
    m_notifyStatusChanged.clear();
    m_notifyTxInserted.clear();
    m_notifyTxStatusChanged.clear();
    m_notifyMerkleBlockInserted.clear();
}

void SynchedVault::updateStatus(status_t newStatus)
{
    if (!m_vault) { newStatus = NOT_LOADED; }
    if (m_status != newStatus)
    {
        m_status = newStatus;
        m_notifyStatusChanged(newStatus);
    }
}
