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
    case STOPPED:
        return "STOPPED";
    case STARTING:
        return "STARTING";
    case FETCHING_HEADERS:
        return "FETCHING_HEADERS";
    case FETCHING_BLOCKS:
        return "FETCHING_BLOCKS";
    case SYNCHED:
        return "SYNCHED";
    default:
        return "UNKNOWN";
    } 
}

// Constructor
SynchedVault::SynchedVault(const CoinQ::CoinParams& coinParams) :
    m_vault(nullptr),
    m_status(STOPPED),
    m_bestHeight(0),
    m_syncHeight(0),
    m_networkSync(coinParams),
    m_bBlockTreeLoaded(false),
    m_bConnected(false),
    m_bSynching(false),
    m_bBlockTreeSynched(false),
    m_bInsertMerkleBlocks(false)
{
    LOGGER(trace) << "SynchedVault::SynchedVault()" << std::endl;

    m_networkSync.subscribeStatus([this](const std::string& message)
    {
        LOGGER(trace) << "P2P network status: " << message << std::endl;
    });

    m_networkSync.subscribeProtocolError([this](const std::string& error)
    {
        LOGGER(trace) << "P2P protocol error: " << error << std::endl;
        m_notifyProtocolError(error);
    });

    m_networkSync.subscribeConnectionError([this](const std::string& error)
    {
        LOGGER(trace) << "P2P connection error: " << error << std::endl;
        m_notifyConnectionError(error);
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
        updateStatus(STOPPED);
    });

    m_networkSync.subscribeStarted([this]()
    {
/*
        LOGGER(trace) << "P2P network sync started." << std::endl;
        m_bSynching = true;
        updateStatus(STARTING);
*/
        //TODO: notify clients
    });

    m_networkSync.subscribeStopped([this]()
    {
/*
        LOGGER(trace) << "P2P network sync stopped." << std::endl;
        m_bSynching = false;
        updateStatus(STOPPED);
*/
        //TODO: notify clients
    });

    m_networkSync.subscribeTimeout([this]()
    {
        LOGGER(trace) << "P2P network sync timeout." << std::endl;
        m_notifyConnectionError("Network timed out.");
    });

    m_networkSync.subscribeFetchingHeaders([this]()
    {
        LOGGER(trace) << "P2P fetching headers." << std::endl;
        updateStatus(FETCHING_HEADERS);
    });

    m_networkSync.subscribeHeadersSynched([this]()
    {
        LOGGER(trace) << "P2P headers sync complete." << std::endl;
        m_bBlockTreeSynched = true;
        updateBestHeight(m_networkSync.getBestHeight());

        try
        {
            if (m_vault)
            {
                LOGGER(trace) << "Attempting to sync blocks." << std::endl;
                syncBlocks();
            }
            else if (m_networkSync.connected())
            {
                updateStatus(SYNCHED);
            }
        }
        catch (const std::exception& e)
        {
            LOGGER(error) << e.what() << std::endl;
        }

    });

    m_networkSync.subscribeFetchingBlocks([this]()
    {
        LOGGER(trace) << "P2P fetching blocks." << std::endl;
        
        if (m_vault)
        {        
            updateStatus(FETCHING_BLOCKS);
        }
        else if (m_networkSync.connected())
        {
            m_networkSync.stopSyncBlocks();
            //updateStatus(SYNCHED);
        }
    });

    m_networkSync.subscribeBlocksSynched([this]()
    {
        LOGGER(trace) << "Block sync complete." << std::endl;

        if (m_networkSync.connected())
        {
            updateStatus(SYNCHED);

            LOGGER(info) << "Fetching mempool." << std::endl;
            m_networkSync.getMempool();
        }
    });

    m_networkSync.subscribeAddBestChain([this](const chain_header_t& header)
    {
        LOGGER(trace) << "P2P network added best chain. New best height: " << header.height << std::endl;

        updateBestHeight(header.height);
    });

    m_networkSync.subscribeRemoveBestChain([this](const chain_header_t& header)
    {
        LOGGER(trace) << "P2P network removed best chain." << std::endl;
        int diff = m_bestHeight - m_networkSync.getBestHeight();
        if (diff >= 0)
        {
            LOGGER(trace) << "Reorganizing " << (diff + 1) << " blocks." << std::endl;
            updateBestHeight(m_networkSync.getBestHeight());
        }
    });

    m_networkSync.subscribeNewTx([this](const Coin::Transaction& cointx)
    {
        LOGGER(trace) << "P2P network received new transaction " << cointx.getHashLittleEndian().getHex() << std::endl;

        if (!m_vault) return;
        std::lock_guard<std::mutex> lock(m_vaultMutex);
        if (!m_vault) return;

        try
        {
            m_vault->insertNewTx(cointx);
        }
        catch (const std::exception& e)
        {
            LOGGER(error) << e.what() << std::endl;
            m_notifyVaultError(e.what());
        } 
    });

	m_networkSync.subscribeMerkleTx([this](const ChainMerkleBlock& chainmerkleblock, const Coin::Transaction& cointx, unsigned int txindex, unsigned int txcount)
	{
        LOGGER(trace) << "P2P network received merkle transaction " << cointx.getHashLittleEndian().getHex() << " in block " << chainmerkleblock.blockHeader.getHashLittleEndian().getHex() << std::endl;

        if (!m_vault) return;
        std::lock_guard<std::mutex> lock(m_vaultMutex);
        if (!m_vault) return;

        try
        {
            m_vault->insertMerkleTx(chainmerkleblock, cointx, txindex, txcount);
        }
        catch (const std::exception& e)
        {
            LOGGER(error) << e.what() << std::endl;
            m_notifyVaultError(e.what());
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
			merkleblock->txsinserted(true);
            m_vault->insertMerkleBlock(merkleblock);
        }
        catch (const std::exception& e)
        {
            LOGGER(error) << e.what() << std::endl;
            m_notifyVaultError(e.what());
        }
    });

    m_networkSync.subscribeBlockTreeChanged([this]()
    {
        LOGGER(trace) << "P2P network block tree changed." << std::endl;

        m_bBlockTreeSynched = false;
        updateBestHeight(m_networkSync.getBestHeight());
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
void SynchedVault::loadHeaders(const std::string& blockTreeFile, bool bCheckProofOfWork, std::function<void(const CoinQBlockTreeMem&)> callback)
{
    LOGGER(trace) << "SynchedVault::loadHeaders(" << blockTreeFile << ", " << (bCheckProofOfWork ? "true" : "false") << ")" << std::endl;

    m_blockTreeFile = blockTreeFile;
    m_bBlockTreeLoaded = false;
    m_networkSync.loadHeaders(blockTreeFile, bCheckProofOfWork, callback);
    m_bBlockTreeLoaded = true;
}

// Vault operations
void SynchedVault::openVault(const std::string& dbname, bool bCreate)
{
    LOGGER(trace) << "SynchedVault::openVault(" << dbname << ", " << (bCreate ? "true" : "false") << ")" << std::endl;

    {
        std::lock_guard<std::mutex> lock(m_vaultMutex);
        m_bInsertMerkleBlocks = false;
        m_notifyVaultClosed();
        if (m_vault) delete m_vault;
        try
        {
            m_vault = new Vault(dbname, bCreate);
        }
        catch (const VaultException& e)
        {
            m_vault = nullptr;
            throw e;
        }
        catch (const std::exception& e)
        {
            m_vault = nullptr;
            throw e;
        }

        updateSyncHeight(m_vault->getBestHeight());
        m_vault->subscribeTxInserted([this](std::shared_ptr<Tx> tx) { m_notifyTxInserted(tx); });
        m_vault->subscribeTxStatusChanged([this](std::shared_ptr<Tx> tx) { m_notifyTxStatusChanged(tx); });
        m_vault->subscribeMerkleBlockInserted([this](std::shared_ptr<MerkleBlock> merkleblock)
        {
            updateSyncHeight(merkleblock->blockheader()->height());
            m_notifyMerkleBlockInserted(merkleblock);
        });
        m_vault->subscribeTxInsertionError([this](std::shared_ptr<Tx> tx, std::string description) { m_notifyTxInsertionError(tx, description); });
        m_vault->subscribeMerkleBlockInsertionError([this](std::shared_ptr<MerkleBlock> merkleblock, std::string description) { m_notifyMerkleBlockInsertionError(merkleblock, description); });
    }

    m_notifyVaultOpened(m_vault);
    if (m_networkSync.connected() && m_networkSync.headersSynched()) { syncBlocks(); }
}

void SynchedVault::openVault(const std::string& dbuser, const std::string& dbpasswd, const std::string& dbname, bool bCreate)
{
    LOGGER(trace) << "SynchedVault::openVault(" << dbuser << ", ..., " << dbname << ", " << (bCreate ? "true" : "false") << ")" << std::endl;

    {
        std::lock_guard<std::mutex> lock(m_vaultMutex);
        m_notifyVaultClosed();
        if (m_vault) delete m_vault;
        try
        {
            m_vault = new Vault(dbuser, dbpasswd, dbname, bCreate);
        }
        catch (const VaultException& e)
        {
            m_vault = nullptr;
            throw e;
        }
        catch (const std::exception& e)
        {
            m_vault = nullptr;
            throw e;
        }

        updateSyncHeight(m_vault->getBestHeight());

        m_vault->subscribeTxInserted([this](std::shared_ptr<Tx> tx) { m_notifyTxInserted(tx); });
        m_vault->subscribeTxStatusChanged([this](std::shared_ptr<Tx> tx) { m_notifyTxStatusChanged(tx); });
        m_vault->subscribeMerkleBlockInserted([this](std::shared_ptr<MerkleBlock> merkleblock)
        {
            updateSyncHeight(merkleblock->blockheader()->height());
            m_notifyMerkleBlockInserted(merkleblock);
        });
        m_vault->subscribeTxInsertionError([this](std::shared_ptr<Tx> tx, std::string description) { m_notifyTxInsertionError(tx, description); });
        m_vault->subscribeMerkleBlockInsertionError([this](std::shared_ptr<MerkleBlock> merkleblock, std::string description) { m_notifyMerkleBlockInsertionError(merkleblock, description); });
    }

    m_notifyVaultOpened(m_vault);
    if (m_networkSync.connected() && m_networkSync.headersSynched()) { syncBlocks(); }
}

void SynchedVault::closeVault()
{
    LOGGER(trace) << "SynchedVault::closeVault()" << std::endl;

    {
        if (!m_vault) return;
        std::lock_guard<std::mutex> lock(m_vaultMutex);
        if (!m_vault) return;

        m_bInsertMerkleBlocks = false;
        m_networkSync.stopSyncBlocks();
        delete m_vault;
    }

    m_vault = nullptr;
    m_notifyVaultClosed();
    updateSyncHeight(0);
    if (m_networkSync.connected() && m_networkSync.headersSynched()) { updateStatus(SYNCHED); }
}

// Peer to peer network operations
void SynchedVault::startSync(const std::string& host, const std::string& port)
{
    LOGGER(trace) << "SynchedVault::startSync(" << host << ", " << port << ")" << std::endl;
    m_bInsertMerkleBlocks = false;
    updateStatus(STARTING);
    m_networkSync.start(host, port); 
}

void SynchedVault::startSync(const std::string& host, int port)
{
    std::stringstream ss;
    ss << port;
    startSync(host, ss.str());
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
    LOGGER(trace) << "SynchedVault::syncBlocks()" << std::endl;

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

    m_networkSync.setBloomFilter(m_vault->getBloomFilter(0.001, 0, 0));

    std::vector<bytes_t> locatorHashes = m_vault->getLocatorHashes();
    m_bInsertMerkleBlocks = true;
    m_networkSync.syncBlocks(locatorHashes, startTime);
}

void SynchedVault::updateBloomFilter()
{
    LOGGER(trace) << "SynchedVault::updateBloomFilter()" << std::endl;

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

void SynchedVault::sendTx(Coin::Transaction& coin_tx)
{
    uchar_vector hash = coin_tx.getHashLittleEndian();
    LOGGER(trace) << "SynchedVault::sendTx(" << hash.getHex() << ")" << std::endl;
    if (!m_bConnected) throw std::runtime_error("Not connected.");

    m_networkSync.sendTx(coin_tx);
    m_networkSync.getTx(hash);
}


// Event subscriptions
void SynchedVault::clearAllSlots()
{
    LOGGER(trace) << "SynchedVault::clearAllSlots()" << std::endl;

    m_notifyVaultOpened.clear();
    m_notifyVaultClosed.clear();
    m_notifyVaultError.clear();

    m_notifyStatusChanged.clear();
    m_notifyBestHeightChanged.clear();
    m_notifySyncHeightChanged.clear();
    m_notifyConnectionError.clear();

    m_notifyTxInserted.clear();
    m_notifyTxStatusChanged.clear();
    m_notifyMerkleBlockInserted.clear();
    m_notifyTxInsertionError.clear();
    m_notifyMerkleBlockInsertionError.clear();
    m_notifyProtocolError.clear();
}

void SynchedVault::updateStatus(status_t newStatus)
{
    if (m_status != newStatus)
    {
        m_status = newStatus;
        m_notifyStatusChanged(newStatus);
    }
}

void SynchedVault::updateBestHeight(uint32_t bestHeight)
{
    if (m_bestHeight != bestHeight)
    {
        m_bestHeight = bestHeight;
        m_notifyBestHeightChanged(bestHeight);
    }
}

void SynchedVault::updateSyncHeight(uint32_t syncHeight)
{
    if (m_syncHeight != syncHeight)
    {
        m_syncHeight = syncHeight;
        m_notifySyncHeightChanged(syncHeight);
    }
}
