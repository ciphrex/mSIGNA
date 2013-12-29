///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_sqlite3.h 
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef _COINQ_SQLITE3_H_
#define _COINQ_SQLITE3_H_

#include "CoinQ_blocks.h"
#include "CoinQ_txs.h"
#include "CoinQ_keys.h"

#include <sqlite3cpp.h>

/**
 * class CoinQBlockTreeSqlite3
*/
class CoinQBlockTreeSqlite3 : public ICoinQBlockTree
{
private:
    mutable SQLite3DB db;

    CoinQSignal<const ChainHeader&> notifyAddBestChain;
    CoinQSignal<const ChainHeader&> notifyRemoveBestChain;
    CoinQSignal<const ChainHeader&> notifyInsert;
    CoinQSignal<const ChainHeader&> notifyDelete;
    CoinQSignal<const ChainHeader&> notifyReorg;

protected:
    bool setBestChain(const std::string& hash);

public:
    CoinQBlockTreeSqlite3() { }

    void open(const std::string& filename, const Coin::CoinBlockHeader& header, bool bCreateIfNotExists = true);
    void close();
    bool isOpen() const { return db.isOpen(); }

    void subscribeAddBestChain(chain_header_slot_t slot) { notifyAddBestChain.connect(slot); }
    void subscribeRemoveBestChain(chain_header_slot_t slot) { notifyRemoveBestChain.connect(slot); }
    void subscribeInsert(chain_header_slot_t slot) { notifyInsert.connect(slot); }
    void subscribeDelete(chain_header_slot_t slot) { notifyDelete.connect(slot); }
    void subscribeReorg(chain_header_slot_t slot) { notifyReorg.connect(slot); }

    void setGenesisBlock(const Coin::CoinBlockHeader& header) { }

    // returns true if new header added, false if header already exists
    // throws runtime_error if header invalid or parent not known
    bool insertHeader(const Coin::CoinBlockHeader& header);

    // returns true if header removed, false if header unknown
    bool deleteHeader(const uchar_vector& hash) { return false; }

    bool hasHeader(const uchar_vector& hash) const { return false; }
    ChainHeader getHeader(const uchar_vector& hash) const { return ChainHeader(); }
    ChainHeader getHeader(int height) const { return ChainHeader(); } // Use -1 to get top block

    int getBestHeight() const;
    BigInt getTotalWork() const;

    std::vector<uchar_vector> getLocatorHashes(int maxSize) const;

    int getConfirmations(const uchar_vector& hash) const { return 0;}
    void clear() { }
};


/**
 * class CoinQTxStoreSqlite3
*/
class CoinQTxStoreSqlite3 : public ICoinQTxStore
{
private:
    mutable SQLite3DB db;

    CoinQSignal<const ChainTransaction&> notifyInsert;
    CoinQSignal<const ChainTransaction&> notifyDelete;
    CoinQSignal<const ChainTransaction&> notifyConfirm;
    CoinQSignal<const ChainTransaction&> notifyUnconfirm;
    CoinQSignal<int> notifyNewBestHeight;

    bool getTx(uint64_t id, ChainTransaction& tx) const;

public:
    void open(const std::string& filename, bool bCreateIfNotExists = true);
    void close();
    bool isOpen() const { return db.isOpen(); }

    bool insert(const ChainTransaction& tx);
    bool deleteTx(const uchar_vector& txHash);
    bool unconfirm(const uchar_vector& blockHash);

    bool hasTx(const uchar_vector& txHash) const;
    bool getTx(const uchar_vector& txHash, ChainTransaction& tx) const;

    uchar_vector getBestBlockHash(const uchar_vector& txHash) const;
    int getBestBlockHeight(const uchar_vector& txHash) const;
    std::vector<ChainTransaction> getConfirmedTxs(int minHeight, int maxHeight) const;
    std::vector<ChainTransaction> getUnconfirmedTxs() const;

    std::vector<ChainTxOut> getTxOuts(const CoinQ::Keys::AddressSet& addressSet, Status status, int minConf) const;

    void setBestHeight(int height);
    int  getBestHeight() const;

    void subscribeInsert(chain_tx_slot_t slot) { notifyInsert.connect(slot); }
    void subscribeDelete(chain_tx_slot_t slot) { notifyDelete.connect(slot); }
    void subscribeConfirm(chain_tx_slot_t slot) { notifyConfirm.connect(slot); }
    void subscribeUnconfirm(chain_tx_slot_t slot) { notifyUnconfirm.connect(slot); }
    void subscribeNewBestHeight(std::function<void(int)> slot) { notifyNewBestHeight.connect(slot); }
};

/**
 * class CoinQKeychainSqlite3
*/
class CoinQKeychainSqlite3 : public CoinQ::Keys::IKeychain
{
private:
    mutable SQLite3DB db;

public:
    void open(const std::string& filename, bool bCreateIfNotExists = true);
    void close();
    bool isOpen() const { return db.isOpen(); }

    void generateNewKeys(int count, int type, bool bCompressed = true); 
    int getPoolSize(int type) const;

    void insertKey(const uchar_vector& privKey, int type, int status, const std::string& label, bool bCompressed = true, int minHeight = -1, int maxHeight = -1);
    void insertMultiSigRedeemScript(const std::vector<uchar_vector>& pubKeys, int type, int status, const std::string& label, int minHeight = -1, int maxHeight = -1);

    std::vector<uchar_vector> getKeyHashes(int type = RECEIVING | CHANGE, int status = POOL | ALLOCATED) const;
    std::vector<uchar_vector> getRedeemScriptHashes(int type = RECEIVING | CHANGE, int status = POOL | ALLOCATED) const;

    uchar_vector getPublicKeyFromHash(const uchar_vector& keyHash) const;
    uchar_vector getPrivateKeyFromHash(const uchar_vector& keyHash) const;
    uchar_vector getPrivateKeyFromPublicKey(const uchar_vector& pubKey) const;
    uchar_vector getNextPoolPublicKey(int type) const;
    uchar_vector getNextPoolPublicKeyHash(int type) const;
    void removeKeyFromPool(const uchar_vector& keyHash) const;

    uchar_vector getRedeemScript(const uchar_vector& scriptHash) const;
    uchar_vector getNextPoolRedeemScript(int type) const;
    uchar_vector getNextPoolRedeemScriptHash(int type) const;
    void removeRedeemScriptFromPool(const uchar_vector& scriptHash);

    uchar_vector getUnsignedScriptSig(const uchar_vector& scriptPubKey) const;

/*
    bool getUnsignedScriptSig(uchar_vector& scriptSig, const uchar_vector& scriptPubKey) const;
    int signTx(Coin::Transaction& tx, int inIndex, const tx_dependency_map_t& dependencies, unsigned char sigHashType = SIGHASH_ALL) const;
    void removeUsedKeysFromPool(const Coin::Transaction& tx);
*/
    uchar_vector getKeysOnlyHash() const;
    uchar_vector getFullKeychainHash() const;
};

#endif // _COINQ_SQLITE3_H_
