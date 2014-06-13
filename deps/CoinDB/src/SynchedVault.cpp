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

// Constructor
SynchedVault::SynchedVault(const std::string& blockTreeFile) :
    m_vault(nullptr),
    m_blockTreeFile(blockTreeFile),
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
    });

    m_networkSync.subscribeStarted([this]()
    {
        LOGGER(trace) << "P2P network sync started." << std::endl;
        m_bSynching = true;
        //TODO: notify clients
    });

    m_networkSync.subscribeStopped([this]()
    {
        LOGGER(trace) << "P2P network sync stopped." << std::endl;
        m_bSynching = false;
        //TODO: notify clients
    });

    m_networkSync.subscribeTimeout([this]()
    {
        LOGGER(trace) << "P2P network sync timeout." << std::endl;
        m_bConnected = false;
        m_bSynching = false;
        // TODO: notify clients
    });

    m_networkSync.subscribeDoneSync([this]()
    {
        LOGGER(trace) << "P2P network sync complete." << std::endl;
        m_bBlockTreeSynched = true;
        // TODO: notify clients

        try
        {
            LOGGER(trace) << "Attempting to resync vault." << std::endl;
            resyncVault();
        }
        catch (const std::exception& e)
        {
            LOGGER(error) << e.what() << std::endl;
        }
    });

    m_networkSync.subscribeDoneResync([this]()
    {
        LOGGER(trace) << "Block tree resync complete." << std::endl;
        LOGGER(info) << "Fetching mempool." << std::endl;
        m_networkSync.getMempool();
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
        std::lock_guard<std::mutex> lock(m_vaultMutex);
        if (!m_vault) return;

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

    m_networkSync.initBlockTree(m_blockTreeFile, false);
}

// Destructor
SynchedVault::~SynchedVault()
{
    LOGGER(trace) << "SynchedVault::~SynchedVault()" << std::endl;
    closeVault();
}

// Vault operations
void SynchedVault::openVault(const std::string& filename, bool bCreate)
{
    LOGGER(trace) << "SynchedVault::openVault(" << filename << ", " << (bCreate ? "true" : "false") << ")" << std::endl;
    std::lock_guard<std::mutex> lock(m_vaultMutex);
    if (m_vault) delete m_vault;
    m_vault = new Vault(filename, bCreate);
    m_networkSync.setBloomFilter(m_vault->getBloomFilter(0.001, 0, 0));
    m_vault->subscribeTxInserted([this](std::shared_ptr<Tx> tx) { m_notifyTxInserted(tx); });
    m_vault->subscribeTxStatusChanged([this](std::shared_ptr<Tx> tx) { m_notifyTxStatusChanged(tx); });
    m_vault->subscribeMerkleBlockInserted([this](std::shared_ptr<MerkleBlock> merkleblock)
    {
        m_syncHeight = merkleblock->blockheader()->height();
        m_notifyMerkleBlockInserted(merkleblock);
    });
}

void SynchedVault::closeVault()
{
    LOGGER(trace) << "SynchedVault::closeVault()" << std::endl;
    std::lock_guard<std::mutex> lock(m_vaultMutex);
    if (m_vault)
    {
        m_vault->clearAllSlots();
        delete m_vault;
        m_vault = nullptr;
    }
}

// Peer to peer network operations
void SynchedVault::startSync(const std::string& host, const std::string& port)
{
    LOGGER(trace) << "SynchedVault::startSync(" << host << ", " << port << ")" << std::endl;
    m_networkSync.start(host, port); 
}

void SynchedVault::stopSync()
{
    LOGGER(trace) << "SynchedVault::stopSync()" << std::endl;
    m_networkSync.stop();
}

void SynchedVault::resyncVault()
{
    LOGGER(trace) << "SynchedVault::resyncVault()" << std::endl;
    if (!m_bConnected) throw std::runtime_error("Not connected.");

    if (!m_vault) throw std::runtime_error("No vault is open.");
    std::lock_guard<std::mutex> lock(m_vaultMutex);
    if (!m_vault) throw std::runtime_error("No vault is open.");

    uint32_t startTime = m_vault->getMaxFirstBlockTimestamp();
    std::vector<bytes_t> locatorHashes = m_vault->getLocatorHashes();
    m_networkSync.resync(locatorHashes, startTime);
}

// Event subscriptions
void SynchedVault::clearAllSlots()
{
    LOGGER(trace) << "SynchedVault::clearAllSlots()" << std::endl;
    m_notifyTxInserted.clear();
    m_notifyTxStatusChanged.clear();
    m_notifyMerkleBlockInserted.clear();
}

