///////////////////////////////////////////////////////////////////////////////
//
// SynchedVault.cpp
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
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
    case SYNCHING_HEADERS:
        return "SYNCHING_HEADERS";
    case SYNCHING_BLOCKS:
        return "SYNCHING_BLOCKS";
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
    m_filterFalsePositiveRate(0.001),
    m_filterTweak(0),
    m_filterFlags(0),
    m_networkSync(coinParams),
    m_bBlockTreeLoaded(false),
    m_bConnected(false),
    m_bSynching(false),
    m_bBlockTreeSynched(false),
    m_bGotMempool(false),
    m_bInsertMerkleBlocks(false)
{
    LOGGER(trace) << "SynchedVault::SynchedVault()" << std::endl;

    m_networkSync.subscribeStatus([this](const std::string& message)
    {
        LOGGER(trace) << "SynchedVault - Status: " << message << std::endl;
    });

    m_networkSync.subscribeProtocolError([this](const std::string& error, int code)
    {
        LOGGER(trace) << "SynchedVault - Protocol error: " << error << std::endl;
        m_notifyProtocolError(error, code);
    });

    m_networkSync.subscribeConnectionError([this](const std::string& error, int code)
    {
        LOGGER(trace) << "SynchedVault - Connection error: " << error << std::endl;
        m_notifyConnectionError(error, code);
    });

    m_networkSync.subscribeBlockTreeError([this](const std::string& error, int code)
    {
        LOGGER(trace) << "SynchedVault - Blocktree error: " << error << std::endl;
        m_notifyBlockTreeError(error, code);
    });

    m_networkSync.subscribeOpen([this]()
    {
        LOGGER(trace) << "SynchedVault - connection opened." << std::endl;
        m_bConnected = true;
        m_notifyPeerConnected();
    });

    m_networkSync.subscribeClose([this]()
    {
        LOGGER(trace) << "SynchedVault - connection closed." << std::endl;
        m_bConnected = false;
        m_bSynching = false;
        m_notifyPeerDisconnected();
    });

    m_networkSync.subscribeStarted([this]()
    {
        LOGGER(trace) << "SynchedVault - Sync started." << std::endl;
    });

    m_networkSync.subscribeStopped([this]()
    {
        LOGGER(trace) << "SynchedVault - Sync stopped." << std::endl;
        updateStatus(STOPPED);
    });

    m_networkSync.subscribeTimeout([this]()
    {
        LOGGER(trace) << "SynchedVault - Sync timeout." << std::endl;
        m_notifyConnectionError("Network timed out.", -1);
    });

    m_networkSync.subscribeSynchingHeaders([this]()
    {
        LOGGER(trace) << "SynchedVault - Synching headers." << std::endl;
        updateStatus(SYNCHING_HEADERS);
    });

    m_networkSync.subscribeHeadersSynched([this]()
    {
        LOGGER(trace) << "SynchedVault - Headers sync complete." << std::endl;
        m_bBlockTreeSynched = true;
        updateBestHeader(m_networkSync.getBestHeight(), m_networkSync.getBestHash());

        if (!m_networkSync.connected())
        {
            updateStatus(STOPPED);
        }
        else if (!m_vault)
        {
            updateStatus(SYNCHED);
        }
        else
        {
            try
            {
                syncBlocks();
            }
            catch (const std::exception& e)
            {
                LOGGER(error) << e.what() << std::endl;
            }
        }
    });

    m_networkSync.subscribeSynchingBlocks([this]()
    {
        LOGGER(trace) << "SynchedVault - Synching blocks." << std::endl;
        
        if (m_vault) { updateStatus(SYNCHING_BLOCKS); }
    });

    m_networkSync.subscribeBlocksSynched([this]()
    {
        LOGGER(trace) << "SynchedVault - Block sync complete." << std::endl;

        if (m_networkSync.connected())
        {
            updateStatus(SYNCHED);

            if (m_vault && !m_bGotMempool)
            {
                LOGGER(info) << "SynchedVault - Fetching mempool." << std::endl;
                m_networkSync.getMempool();
                m_bGotMempool = true;
            }
        }
    });

    m_networkSync.subscribeAddBestChain([this](const chain_header_t& header)
    {
        LOGGER(trace) << "SynchedVault - Added best chain. New best height: " << header.height << std::endl;

        updateBestHeader(header.height, header.hash());
    });

    m_networkSync.subscribeRemoveBestChain([this](const chain_header_t& header)
    {
        LOGGER(trace) << "SynchedVault - removed best chain." << std::endl;
        int diff = m_bestHeight - m_networkSync.getBestHeight();
        if (diff >= 0)
        {
            LOGGER(trace) << "Reorganizing " << (diff + 1) << " blocks." << std::endl;
            updateBestHeader(m_networkSync.getBestHeight(), m_networkSync.getBestHash());
        }
    });

    m_networkSync.subscribeNewTx([this](const Coin::Transaction& cointx)
    {
        LOGGER(trace) << "SynchedVault - Received new transaction " << cointx.hash().getHex() << std::endl;

        if (!m_vault) return;
        std::lock_guard<std::mutex> lock(m_vaultMutex);
        if (!m_vault) return;

        try
        {
            m_vault->insertNewTx(cointx);
        }
        catch (const VaultException& e)
        {
            LOGGER(error) << e.what() << std::endl;
            m_notifyVaultError(e.what(), e.code());
        } 
        catch (const std::exception& e)
        {
            LOGGER(error) << e.what() << std::endl;
            m_notifyVaultError(e.what(), -1);
        } 
    });

    m_networkSync.subscribeMerkleTx([this](const ChainMerkleBlock& chainmerkleblock, const Coin::Transaction& cointx, unsigned int txindex, unsigned int txcount)
    {
        LOGGER(trace) << "SynchedVault - Received merkle transaction " << cointx.hash().getHex() << " in block " << chainmerkleblock.hash().getHex() << std::endl;

        if (!m_vault) return;
        std::lock_guard<std::mutex> lock(m_vaultMutex);
        if (!m_vault) return;

        try
        {
            m_vault->insertMerkleTx(chainmerkleblock, cointx, txindex, txcount);
        }
        catch (const VaultException& e)
        {
            LOGGER(error) << e.what() << std::endl;
            m_notifyVaultError(e.what(), e.code());
        } 
        catch (const std::exception& e)
        {
            LOGGER(error) << e.what() << std::endl;
            m_notifyVaultError(e.what(), -1);
        } 
	
    });

    m_networkSync.subscribeTxConfirmed([this](const ChainMerkleBlock& chainmerkleblock, const bytes_t& txhash, unsigned int txindex, unsigned int txcount)
    {
        LOGGER(trace) << "SynchedVault - Received transaction confirmation " << uchar_vector(txhash).getHex() << " in block " << chainmerkleblock.hash().getHex() << std::endl;

        if (!m_vault) return;
        std::lock_guard<std::mutex> lock(m_vaultMutex);
        if (!m_vault) return;

        try
        {
            m_vault->confirmMerkleTx(chainmerkleblock, txhash, txindex, txcount);
        }
        catch (const VaultException& e)
        {
            LOGGER(error) << e.what() << std::endl;
            m_notifyVaultError(e.what(), e.code());
        } 
        catch (const std::exception& e)
        {
            LOGGER(error) << e.what() << std::endl;
            m_notifyVaultError(e.what(), -1);
        } 
    });

    m_networkSync.subscribeMerkleBlock([this](const ChainMerkleBlock& chainMerkleBlock)
    {
        LOGGER(trace) << "SynchedVault - received merkle block " << chainMerkleBlock.hash().getHex() << " height: " << chainMerkleBlock.height << std::endl;

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
        catch (const VaultException& e)
        {
            LOGGER(error) << e.what() << std::endl;
            m_notifyVaultError(e.what(), e.code());
        } 
        catch (const std::exception& e)
        {
            LOGGER(error) << e.what() << std::endl;
            m_notifyVaultError(e.what(), -1);
        }
    });

    m_networkSync.subscribeBlockTreeChanged([this]()
    {
        LOGGER(trace) << "SynchedVault - block tree changed." << std::endl;

        m_bBlockTreeSynched = false;
        updateBestHeader(m_networkSync.getBestHeight(), m_networkSync.getBestHash());
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
void SynchedVault::loadHeaders(const std::string& blockTreeFile, bool bCheckProofOfWork, CoinQBlockTreeMem::callback_t callback)
{
    LOGGER(trace) << "SynchedVault::loadHeaders(" << blockTreeFile << ", " << (bCheckProofOfWork ? "true" : "false") << ")" << std::endl;

    m_blockTreeFile = blockTreeFile;
    m_bBlockTreeLoaded = false;
    m_networkSync.loadHeaders(blockTreeFile, bCheckProofOfWork, callback);
    m_bBlockTreeLoaded = true;
}

// Vault operations
void SynchedVault::openVault(const std::string& dbname, bool bCreate, uint32_t version, const std::string& network, bool migrate)
{
    openVault("", "", dbname, bCreate, version, network, migrate);
}

void SynchedVault::openVault(const std::string& dbuser, const std::string& dbpasswd, const std::string& dbname, bool bCreate, uint32_t version, const std::string& network, bool migrate)
{
    LOGGER(trace) << "SynchedVault::openVault(" << dbuser << ", ..., " << dbname << ", " << (bCreate ? "true" : "false") << ", " << version << ", " << network << ", " << (migrate ? "true" : "false") << ")" << std::endl;

    {
        std::lock_guard<std::mutex> lock(m_vaultMutex);
        m_notifyVaultClosed();
        if (m_vault) delete m_vault;
        m_vault = new Vault;
        try
        {
            m_vault->open(dbuser, dbpasswd, dbname, bCreate, version, network, migrate);
        }
        catch (const std::exception& e)
        {
            delete m_vault;
            m_vault = nullptr;
            throw;
        }

        std::shared_ptr<BlockHeader> blockheader = m_vault->getBestBlockHeader();
        if (blockheader)    { updateSyncHeader(blockheader->height(), blockheader->hash()); }
        else                { updateSyncHeader(0, bytes_t()); }

        m_vault->subscribeKeychainUnlocked([this](const std::string& keychainName) { m_notifyKeychainUnlocked(keychainName); });
        m_vault->subscribeKeychainLocked([this](const std::string& keychainName) { m_notifyKeychainLocked(keychainName); });
        m_vault->subscribeTxInserted([this](std::shared_ptr<Tx> tx) { m_notifyTxInserted(tx); });
        m_vault->subscribeTxUpdated([this](std::shared_ptr<Tx> tx)
        {
            if (tx->status() == Tx::PROPAGATED) { m_networkSync.addToMempool(tx->hash()); }
            m_notifyTxUpdated(tx);
        });
        m_vault->subscribeTxDeleted([this](std::shared_ptr<Tx> tx) { m_notifyTxDeleted(tx); });
        m_vault->subscribeMerkleBlockInserted([this](std::shared_ptr<MerkleBlock> merkleblock)
        {
            updateSyncHeader(merkleblock->blockheader()->height(), merkleblock->blockheader()->hash());
            m_notifyMerkleBlockInserted(merkleblock);
        });
        m_vault->subscribeTxInsertionError([this](std::shared_ptr<Tx> tx, std::string description) { m_notifyTxInsertionError(tx, description); });
        m_vault->subscribeMerkleBlockInsertionError([this](std::shared_ptr<MerkleBlock> merkleblock, std::string description) { m_notifyMerkleBlockInsertionError(merkleblock, description); });
        m_vault->subscribeTxConfirmationError([this](std::shared_ptr<MerkleBlock> merkleblock, bytes_t txhash) { m_notifyTxConfirmationError(merkleblock, txhash); });
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
        m_networkSync.stopSynchingBlocks();
        delete m_vault;
        m_vault = nullptr;
    }

    m_notifyVaultClosed();
    updateSyncHeader(0, bytes_t());
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

//TODO: get rid of m_bInsertMerkleBlocks
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
    m_bGotMempool = false;
    m_bInsertMerkleBlocks = true;
    m_networkSync.syncBlocks(locatorHashes, startTime);
}

void SynchedVault::setFilterParams(double falsePositiveRate, uint32_t nTweak, uint8_t nFlags)
{
    m_filterFalsePositiveRate = falsePositiveRate;
    m_filterTweak = nTweak;
    m_filterFlags = nFlags;
}

void SynchedVault::updateBloomFilter()
{
    LOGGER(trace) << "SynchedVault::updateBloomFilter()" << std::endl;

    if (!m_vault) throw std::runtime_error("No vault is open.");
    std::lock_guard<std::mutex> lock(m_vaultMutex);
    if (!m_vault) throw std::runtime_error("No vault is open.");

    m_networkSync.setBloomFilter(m_vault->getBloomFilter(0.001, 0, 0));
}

// This function recursively tries to send dependencies.
// TODO: We might want to make recursive sending optional and allowing an exception to be thrown instead if any dependency is still unpropagated.
void recursiveSendTx(Vault& vault, CoinQ::Network::NetworkSync& networkSync, std::shared_ptr<Tx>& tx)
{
    if (tx->status() == Tx::UNSIGNED)
        throw std::runtime_error("Transaction is missing signatures.");

    // Send any unsent dependencies first.
    // TODO: Write a new protected method to 
    for (auto& txin: tx->txins())
    {
        try
        {
            std::shared_ptr<Tx> dependencyTx = vault.getTx(txin->outhash());
            if (dependencyTx->status() == Tx::UNSIGNED)
                throw std::runtime_error("Transaction depends on another transaction that is missing signatures.");

            // Only try sending dependencies that have not confirmed. 
            if (dependencyTx->status() == Tx::UNSENT || dependencyTx->status() == Tx::PROPAGATED)
            {
                recursiveSendTx(vault, networkSync, dependencyTx);
            }
        }
        catch (const TxNotFoundException& e)
        {
            // Ignore dependencies we do not have.
        }
    }

    Coin::Transaction coin_tx = tx->toCoinCore();
    networkSync.sendTx(coin_tx);
    uchar_vector txhash(tx->hash());
    networkSync.getTx(txhash); // To ensure propagation
}

std::shared_ptr<Tx> SynchedVault::sendTx(const bytes_t& hash)
{
    LOGGER(trace) << "SynchedVault::sendTx(" << uchar_vector(hash).getHex() << ")" << std::endl;
    if (!m_bConnected) throw std::runtime_error("Not connected.");

    if (!m_vault) throw std::runtime_error("No vault is open.");
    std::lock_guard<std::mutex> lock(m_vaultMutex);
    if (!m_vault) throw std::runtime_error("No vault is open.");

    std::shared_ptr<Tx> tx = m_vault->getTx(hash);
    recursiveSendTx(*m_vault, m_networkSync, tx);
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
    recursiveSendTx(*m_vault, m_networkSync, tx);
    return tx;
}

void SynchedVault::sendTx(Coin::Transaction& coin_tx)
{
    uchar_vector hash = coin_tx.hash();
    LOGGER(trace) << "SynchedVault::sendTx(" << hash.getHex() << ")" << std::endl;
    if (!m_bConnected) throw std::runtime_error("Not connected.");

    m_networkSync.sendTx(coin_tx);
    m_networkSync.getTx(hash);
}

// For testing
void SynchedVault::insertFakeMerkleBlock(unsigned int nExtraLeaves)
{
    if (!m_vault) throw std::runtime_error("No vault is open.");
    std::unique_lock<std::mutex> lock(m_vaultMutex);
    if (!m_vault) throw std::runtime_error("No vault is open.");

    txs_t txs = m_vault->getTxs(Tx::PROPAGATED);
    std::vector<Coin::Transaction> cointxs;
    std::vector<uchar_vector> txhashes;
    for (auto& tx: txs)
    {
        cointxs.push_back(tx->toCoinCore());
        txhashes.push_back(tx->hash());
    }

    std::shared_ptr<BlockHeader> header = m_vault->getBestBlockHeader();

    Coin::MerkleBlock coinmerkleblock(Coin::randomPartialMerkleTree(txhashes, txhashes.size() + nExtraLeaves), header->version(), header->hash(), time(NULL), header->bits(), 0);

    lock.unlock();
    m_networkSync.insertMerkleBlock(coinmerkleblock, cointxs);
}


// Event subscriptions
void SynchedVault::clearAllSlots()
{
    LOGGER(trace) << "SynchedVault::clearAllSlots()" << std::endl;

    m_notifyVaultOpened.clear();
    m_notifyVaultClosed.clear();
    m_notifyVaultError.clear();

    m_notifyStatusChanged.clear();
    m_notifyBestHeaderChanged.clear();
    m_notifySyncHeaderChanged.clear();
    m_notifyConnectionError.clear();

    m_notifyPeerConnected.clear();
    m_notifyPeerDisconnected.clear();
    m_notifyTxInserted.clear();
    m_notifyTxUpdated.clear();
    m_notifyTxDeleted.clear();
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

void SynchedVault::updateBestHeader(uint32_t bestHeight, const bytes_t& bestHash)
{
    if (m_bestHeight != bestHeight || m_bestHash != bestHash)
    {
        m_bestHeight = bestHeight;
        m_bestHash = bestHash;
        m_notifyBestHeaderChanged(bestHeight, bestHash);
    }
}

void SynchedVault::updateSyncHeader(uint32_t syncHeight, const bytes_t& syncHash)
{
    if (m_syncHeight != syncHeight || m_syncHash != syncHash)
    {
        m_syncHeight = syncHeight;
        m_syncHash = syncHash;
        m_notifySyncHeaderChanged(syncHeight, syncHash);
    }
}
