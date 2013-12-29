///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_sqlite3.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include "CoinQ_sqlite3.h"
#include "CoinQ_script.h"
#include "CoinQ_errors.h"

#include <CoinKey.h>
#include <StandardTransactions.h>

/**
 * class CoinQBlockTreeSqlite3
*/
void CoinQBlockTreeSqlite3::open(const std::string& filename, const Coin::CoinBlockHeader& header, bool bCreateIfNotExists)
{
    uchar_vector genesisHeaderHash = header.getHashLittleEndian();

    std::stringstream sql;
    db.open(filename, bCreateIfNotExists);

    sql << "CREATE TABLE IF NOT EXISTS `block_headers` ("
        <<      "`hash` TEXT UNIQUE NOT NULL,"
        <<      "`in_best_chain` INTEGER NOT NULL,"
        <<      "`height` INTEGER NOT NULL,"
        <<      "`chain_work` TEXT NOT NULL,"
        <<      "`version` INTEGER NOT NULL,"
        <<      "`prev_block_hash` TEXT NOT NULL,"
        <<      "`merkle_root` TEXT NOT NULL,"
        <<      "`timestamp` INTEGER NOT NULL,"
        <<      "`bits` INTEGER NOT NULL,"
        <<      "`nonce` INTEGER NOT NULL,"
        <<      "`time_inserted` INTEGER DEFAULT (strftime('%s', 'now')))";
    db.query(sql.str()); sql.str("");

    SQLite3Tx sqliteTx(db, SQLite3Tx::Type::EXCLUSIVE);
    SQLite3Stmt stmt;
    sql << "SELECT `hash` FROM `block_headers` WHERE `height` = 0";
    stmt.prepare(db, sql.str());
    int count = 0;
    while (stmt.step() == SQLITE_ROW) {
        if (count > 0) {
            throw std::runtime_error("File contains more than one genesis block.");
        }
        uchar_vector hash;
        hash.setHex((char*)stmt.getText(0));
        if (genesisHeaderHash != hash) {
            throw std::runtime_error("Genesis block mismatch.");
        }
        count++;
    }

    if (count == 0) {
        ChainHeader genesisHeader(header, true, 0, header.getWork());

        sql.str("");
        sql << "INSERT INTO `block_headers` (`hash`, `in_best_chain`, `height`, `chain_work`, `version`, `prev_block_hash`, `merkle_root`, `timestamp`, `bits`, `nonce`)"
            << " VALUES ('"
            << genesisHeaderHash.getHex() << "',1,0,'"
            << genesisHeader.chainWork.getDec() << "',"
            << genesisHeader.version << ",'"
            << genesisHeader.prevBlockHash.getHex() << "','"
            << genesisHeader.merkleRoot.getHex() << "',"
            << genesisHeader.timestamp << ","
            << genesisHeader.bits << ","
            << genesisHeader.nonce << ")";
        db.query(sql.str());
        sqliteTx.commit();

        notifyInsert(genesisHeader);
        notifyAddBestChain(genesisHeader);
    }
}

void CoinQBlockTreeSqlite3::close()
{
    db.close();
}

bool CoinQBlockTreeSqlite3::setBestChain(const std::string& hash)
{
    std::stringstream err;
    std::stringstream sql;
    SQLite3Stmt stmt;

    std::string prevHash = hash;
    int bestHeight;

    // Retrace back to best chain ancestor
    std::vector<ChainHeader> newBestChain;
    while (true) {
        sql.str(""); stmt.finalize();
        sql << "SELECT `version`, `timestamp`, `bits`, `nonce`, `prev_block_hash`, `merkle_root`, `in_best_chain`, `height`, `chain_work` "
            << " FROM `block_headers` WHERE `hash` = '" << prevHash << "'";
        stmt.prepare(db, sql.str());
        if (stmt.step() != SQLITE_ROW) {
            if (newBestChain.empty()) {
                throw std::runtime_error("Header not found.");
            }
            else {
                err << "Critical error: header " << prevHash << " appears to be missing.";
                throw std::runtime_error(err.str());
            }
        }
        bestHeight = stmt.getInt(7);
        if (stmt.getInt(6) != 0) break; // We've reached the best chain ancestor and have its height.

        prevHash = (char*)stmt.getText(4);

        BigInt chainWork;
        chainWork.setDec((char*)stmt.getText(8));

        newBestChain.push_back(ChainHeader(stmt.getInt64(0), stmt.getInt64(1), stmt.getInt64(2), stmt.getInt64(3),
            uchar_vector(prevHash), uchar_vector((char*)stmt.getText(5)), true, bestHeight, chainWork));
    }

    if (newBestChain.empty()) {
        return false; // nothing to do
    }

    // Get all best chain descendants from best chain ancestor
    std::vector<ChainHeader> oldBestChain;
    sql.str(""); stmt.finalize();
    sql << "SELECT `version`, `timestamp`, `bits`, `nonce`, `prev_block_hash`, `merkle_root`, `height`, `chain_work` "
        << " FROM `block_headers` WHERE `in_best_chain` != 0 AND `height` > " << bestHeight;
    stmt.prepare(db, sql.str());
    while (stmt.step() == SQLITE_ROW) {
        BigInt chainWork;
        chainWork.setDec((char*)stmt.getText(7));

        oldBestChain.push_back(ChainHeader(stmt.getInt64(0), stmt.getInt64(1), stmt.getInt64(2), stmt.getInt64(3),
            uchar_vector((char*)stmt.getText(4)), uchar_vector((char*)stmt.getText(5)), false, stmt.getInt(6), chainWork));
    }

    // Remove descendants from best chain, send alerts
    if (!oldBestChain.empty()) {
        sql.str("");
        sql << "UPDATE `block_headers` SET `in_best_chain` = 0 WHERE `in_best_chain` != 0 AND `height` > " << bestHeight;
        db.query(sql.str());
        for (unsigned int i = 0; i < oldBestChain.size(); i++) {
            notifyRemoveBestChain(oldBestChain[i]);
        }
    }

    // Set new best chain
    sql.str("");
    sql << "UPDATE `block_headers` SET `in_best_chain` = 1 WHERE `hash` IN ('";
    bool bFirst = true;
    for (auto& header: newBestChain) {
        if (bFirst) { bFirst = false; } else { sql << "','"; }
        sql << header.getHashLittleEndian().getHex();
    }
    sql << "')";
    db.query(sql.str());

    // Send alerts
    int iMax = (int)newBestChain.size() - 1;
    if (iMax >= 0) notifyReorg(newBestChain[iMax]);
    for (int i = iMax; i >= 0; i--) {
        notifyAddBestChain(newBestChain[i]);
    }

    return true;
}

bool CoinQBlockTreeSqlite3::insertHeader(const Coin::CoinBlockHeader& header)
{
    if (!db.isOpen()) {
        throw std::runtime_error("Database is not open.");
    }

    std::stringstream sql;
    SQLite3Stmt stmt;

    sql << "SELECT `height`, `chain_work`, `version`, `timestamp`, `bits`"
        << " FROM `block_headers` WHERE `hash` = '" << header.prevBlockHash.getHex() << "'";
    stmt.prepare(db, sql.str());
    if (stmt.step() != SQLITE_ROW) {
        throw std::runtime_error("Parent not found.");
    }

    // TODO: validate header

    ChainHeader chainHeader(header);
    chainHeader.height = stmt.getInt(0) + 1;
    chainHeader.chainWork.setDec((char*)stmt.getText(1));
    chainHeader.chainWork += chainHeader.getWork();
    std::string chainHeaderHashStr = chainHeader.getHashLittleEndian().getHex();

    SQLite3Tx sqliteTx(db, SQLite3Tx::Type::IMMEDIATE);
    stmt.finalize(); sql.str("");
    sql << "INSERT INTO `block_headers` (`hash`, `in_best_chain`, `height`, `chain_work`, `version`, `prev_block_hash`, `merkle_root`, `timestamp`, `bits`, `nonce`)"
        << " VALUES ('"
        << chainHeaderHashStr << "',0,"
        << chainHeader.height << ",'"
        << chainHeader.chainWork.getDec() << "',"
        << chainHeader.version << ",'"
        << chainHeader.prevBlockHash.getHex() << "','"
        << chainHeader.merkleRoot.getHex() << "',"
        << chainHeader.timestamp << ","
        << chainHeader.bits << ","
        << chainHeader.nonce << ")";
    stmt.prepare(db, sql.str());
    try {
        stmt.step();
        notifyInsert(chainHeader);
    }
    catch (const SQLite3Exception& e) {
        if (e.code() == SQLITE_CONSTRAINT) return false;
        throw e;
    }

    if (chainHeader.chainWork > getTotalWork()) {
        setBestChain(chainHeaderHashStr);
    }

    sqliteTx.commit();
    return true;
}

int CoinQBlockTreeSqlite3::getBestHeight() const
{
    if (!db.isOpen()) {
        throw std::runtime_error("Database is not open.");
    }

    std::stringstream sql;
    SQLite3Stmt stmt;

    sql << "SELECT MAX(`height`) FROM `block_headers` WHERE `in_best_chain` != 0";
    stmt.prepare(db, sql.str());
    if (stmt.step() != SQLITE_ROW) {
        throw std::runtime_error("Tree is empty.");
    }

    return stmt.getInt(0);
}

BigInt CoinQBlockTreeSqlite3::getTotalWork() const
{
    if (!db.isOpen()) {
        throw std::runtime_error("Database is not open.");
    }

    std::stringstream sql;
    SQLite3Stmt stmt;

    sql << "SELECT MAX(`height`), `chain_work` FROM `block_headers` WHERE `in_best_chain` != 0";
    stmt.prepare(db, sql.str());
    if (stmt.step() != SQLITE_ROW) {
        throw std::runtime_error("Tree is empty.");
    }

    BigInt work;
    work.setDec((char*)stmt.getText(1));

    return work;
}

std::vector<uchar_vector> CoinQBlockTreeSqlite3::getLocatorHashes(int maxSize) const
{
    std::vector<uchar_vector> locatorHashes;

    int bestHeight = getBestHeight();
    if (maxSize < 0) maxSize = bestHeight + 1;

    int i = bestHeight;
    int n = 0;
    int step = 1;
    std::vector<int> heights;
    while ((i >= 0) && (n < maxSize)) {
        heights.push_back(i);
        i -= step;
        n++;
        if (n > 10) step *= 2;
    }

    std::stringstream sql;
    SQLite3Stmt stmt;

    sql << "SELECT `hash` FROM `block_headers` WHERE `in_best_chain` != 0 AND `height` IN (";
    bool bFirst = true;
    for (auto height: heights) {
        if (bFirst) { bFirst = false; } else { sql << ","; }
        sql << height;
    }
    sql << ") ORDER BY `height` DESC";
    stmt.prepare(db, sql.str());
    while (stmt.step() == SQLITE_ROW) {
        locatorHashes.push_back(uchar_vector((char*)stmt.getText(0)));
    }

    return locatorHashes;
}

/**
 * class CoinQTxStoreSqlite3
*/
void CoinQTxStoreSqlite3::open(const std::string& filename, bool bCreateIfNotExists)
{
    std::stringstream sql;
    db.open(filename, bCreateIfNotExists);

    sql << "CREATE TABLE IF NOT EXISTS `state` ("
        <<      "`best_height` INTEGER NOT NULL)";
    db.query(sql.str()); sql.str("");

    sql << "INSERT OR IGNORE INTO `state` (`rowid`, `best_height`) VALUES (1,-1)";
    db.query(sql.str()); sql.str("");

    sql << "CREATE TABLE IF NOT EXISTS `block_headers` ("
        <<      "`hash` TEXT UNIQUE NOT NULL,"
        <<      "`height` INTEGER UNIQUE NOT NULL,"
        <<      "`chain_work` TEXT NOT NULL,"
        <<      "`version` INTEGER NOT NULL,"
        <<      "`prev_block_hash` TEXT NOT NULL,"
        <<      "`merkle_root` TEXT NOT NULL,"
        <<      "`timestamp` INTEGER NOT NULL,"
        <<      "`bits` INTEGER NOT NULL,"
        <<      "`nonce` INTEGER NOT NULL,"
        <<      "`time_inserted` INTEGER DEFAULT (strftime('%s', 'now')))";
    db.query(sql.str()); sql.str("");

    sql << "CREATE TABLE IF NOT EXISTS `txs` ("
        <<      "`hash` TEXT UNIQUE NOT NULL,"
        <<      "`version` INTEGER NOT NULL,"
        <<      "`locktime` INTEGER NOT NULL,"
        <<      "`block_header_id` INTEGER NOT NULL DEFAULT 0,"
        <<      "`block_index` INTEGER DEFAULT NULL,"
        <<      "`merkle_path` TEXT DEFAULT NULL,"
        <<      "`time_inserted` INTEGER DEFAULT (strftime('%s', 'now')))";
    db.query(sql.str()); sql.str("");

    sql << "CREATE TABLE IF NOT EXISTS `txouts` ("
        <<      "`tx_id` INTEGER NOT NULL,"
        <<      "`tx_index` INTEGER NOT NULL,"
        <<      "`amount` INTEGER NOT NULL,"
        <<      "`script` TEXT NOT NULL,"
        <<      "`address` TEXT DEFAULT NULL,"
        <<      "`spent` INTEGER NOT NULL DEFAULT 0)";
    db.query(sql.str()); sql.str("");

    sql << "CREATE TABLE IF NOT EXISTS `txins` ("
        <<      "`tx_id` INTEGER NOT NULL,"
        <<      "`tx_index` INTEGER NOT NULL,"
        <<      "`out_hash` TEXT NOT NULL,"
        <<      "`out_index` INTEGER NOT NULL,"
        <<      "`amount` INTEGER DEFAULT NULL,"
        <<      "`script` TEXT NOT NULL,"
        <<      "`sequence` INTEGER NOT NULL,"
        <<      "`address` TEXT DEFAULT NULL)";
    db.query(sql.str());
}

void CoinQTxStoreSqlite3::close()
{
    db.close();
}

bool CoinQTxStoreSqlite3::insert(const ChainTransaction& tx)
{
    if (!db.isOpen()) {
        throw std::runtime_error("Database is not open.");
    }

    std::stringstream sql;
    SQLite3Stmt stmt;

    uchar_vector txHash = tx.getHashLittleEndian();
    sql << "SELECT `block_index` FROM `txs` WHERE `hash` = '" << txHash.getHex() << "'";
    stmt.prepare(db, sql.str());
    // don't replace a confirmed transaction. don't replace a transaction with unconfirmed transaction.
    bool bExists = (stmt.step() == SQLITE_ROW);
    bool bConfirmed = (tx.blockHeader.height != -1);
    if (bExists && (stmt.getInt(0) != -1 || !bConfirmed)) return false;
   
    stmt.finalize(); sql.str("");
    {
        SQLite3Tx sqlite3Tx(db, SQLite3Tx::Type::IMMEDIATE);

        if (bConfirmed) {
            const ChainHeader& header = tx.blockHeader;
            sql << "INSERT OR IGNORE INTO `block_headers` (`hash`, `height`, `chain_work`, `version`, `prev_block_hash`, `merkle_root`, `timestamp`, `bits`, `nonce`) "
                <<      "VALUES ('" << header.getHashLittleEndian().getHex() << "', "
                                    << header.height << ", '"
                                    << header.chainWork.getDec() << "', "
                                    << header.version << ", '"
                                    << header.prevBlockHash.getHex() << "', '"
                                    << header.merkleRoot.getHex() << "', "
                                    << header.timestamp << ", "
                                    << header.bits << ", "
                                    << header.nonce << ")";
            db.query(sql.str()); sql.str("");
        }
        if (!bExists) {
            sql << "INSERT INTO `txs` (`hash`, `version`, `locktime`, `block_header_id`, `block_index`, `merkle_path`) "
                <<      "VALUES ('" << txHash.getHex() << "', "
                                    << tx.version << ", "
                                    << tx.lockTime << ", "
                                    << (bConfirmed ? "last_insert_rowid()" : "0") << ", "
                                    << tx.index << ", "
                                    << "NULL)";

            db.query(sql.str()); sql.str("");
            uint64_t txRowId = (uint64_t)db.lastInsertRowId();

            for (unsigned int i = 0; i < tx.outputs.size(); i++) {
                sql << "SELECT `rowid` FROM `txins` WHERE `out_hash` = '" << txHash.getHex() << "' AND `out_index` = " << i;
                stmt.prepare(db, sql.str());
                bool bSpent = (stmt.step() == SQLITE_ROW);
                stmt.finalize(); sql.str("");
                const Coin::TxOut& txOut = tx.outputs[i];
                sql << "INSERT INTO `txouts` (`tx_id`, `tx_index`, `amount`, `script`, `address`, `spent`) "
                    <<      "VALUES (" << txRowId << ", "
                                       << i << ", "
                                       << txOut.value << ", '"
                                       << txOut.scriptPubKey.getHex() << "', '"
                                       << txOut.getAddress() << "', "
                                       << (bSpent ? "1" : "0") << ")";
                db.query(sql.str()); sql.str("");
            }

            for (unsigned int i = 0; i < tx.inputs.size(); i++) {
                std::stringstream amount;
                const Coin::TxIn& txIn = tx.inputs[i];
                std::string outpointHashHex = txIn.getOutpointHash().getHex();
                sql << "SELECT `txouts`.`rowid`, `txouts`.`amount` FROM `txouts`, `txs` "
                    << "WHERE `txs`.`rowid` = `txouts`.`tx_id` AND `txs`.`hash` = '" << outpointHashHex << "' "
                    << "AND `txouts`.`tx_index` = " << txIn.getOutpointIndex();
                stmt.prepare(db, sql.str());
                if (stmt.step() == SQLITE_ROW) {
                    amount << strtoull((char*)stmt.getText(1), NULL, 0);
                    sql.str("");
                    sql << "UPDATE `txouts` SET `spent` = 1 WHERE `rowid` = " << strtoull((char*)stmt.getText(0), NULL, 0);
                    db.query(sql.str());
                }
                else {
                    amount << "NULL";
                }
                stmt.finalize(); sql.str("");

                sql << "INSERT INTO `txins` (`tx_id`, `tx_index`, `out_hash`, `out_index`, `amount`, `script`, `sequence`, `address`) "
                    <<      "VALUES (" << txRowId << ", "
                                       << i << ", '"
                                       << outpointHashHex << "', "
                                       << txIn.getOutpointIndex() << ", "
                                       << amount.str() << ", '"
                                       << txIn.scriptSig.getHex() << "', "
                                       << txIn.sequence << ", '"
                                       << txIn.getAddress() << "')";
                db.query(sql.str()); sql.str(""); 
            }
        }
        else {
            sql << "UPDATE `txs` SET "
                <<      "`block_header_id` = last_insert_rowid(), "
                <<      "`block_index` = " << tx.index << ", "
                <<      "`merkle_path` = NULL "
                << "WHERE `hash` = '" << txHash.getHex() << "'";
            db.query(sql.str()); sql.str("");
        }

        sqlite3Tx.commit(); 
    }

    if (!bExists) {
        notifyInsert(tx);
    }

    if (bConfirmed) {
        notifyConfirm(tx);
    }

    return true;
}

bool CoinQTxStoreSqlite3::deleteTx(const uchar_vector& txHash)
{
    if (!db.isOpen()) {
        throw std::runtime_error("Database is not open.");
    }

    std::stringstream sql;
    SQLite3Stmt stmt;

    sql << "SELECT `rowid` FROM `txs` WHERE `hash` = '" << txHash.getHex() << "'";
    stmt.prepare(db, sql.str());
    if (stmt.step() == SQLITE_ROW) {
        uint64_t txRowId = strtoull((char*)stmt.getText(0), NULL, 0);
        stmt.finalize(); sql.str("");
        {
            SQLite3Tx sqlite3Tx(db, SQLite3Tx::Type::IMMEDIATE);

            sql << "SELECT `txouts`.`rowid` FROM `txouts`, `txins`, `txs`"
                << " WHERE `txins`.`tx_id` = " << txRowId
                << " AND `txins`.`out_hash` = `txs`.`hash`"
                << " AND `txouts`.`tx_id` = `txs`.`rowid`"
                << " AND `txouts`.`tx_index` = `txins`.`out_index`";
            stmt.prepare(db, sql.str());
            while (stmt.step() == SQLITE_ROW) {
                sql.str("");
                sql << "UPDATE `txouts` SET `spent` = 0 WHERE `rowid` = " << strtoull((char*)stmt.getText(0), NULL, 0);
                db.query(sql.str());
            }
            sql.str("");
            sql << "DELETE FROM `txins` WHERE `tx_id` = " << txRowId << "; "
                << "DELETE FROM `txouts` WHERE `tx_id` = " << txRowId << "; "
                << "DELETE FROM `txs` WHERE `rowid` = " << txRowId << ";";
            db.query(sql.str());

            sqlite3Tx.commit();
        }
        return true;
    }

    return false;
}

bool CoinQTxStoreSqlite3::unconfirm(const uchar_vector& blockHash)
{
    if (!db.isOpen()) {
        throw std::runtime_error("Database is not open.");
    }

    std::stringstream sql;
    SQLite3Stmt stmt;

    sql << "SELECT rowid FROM `block_headers` WHERE `hash` = '" << blockHash.getHex() << "'";
    stmt.prepare(db, sql.str());
    if (stmt.step() == SQLITE_ROW) {
        uint64_t headerRowId = strtoull((char*)stmt.getText(0), NULL, 0);
        stmt.finalize(); sql.str("");
        sql << "SELECT rowid FROM `txs` WHERE `block_header_id` = " << headerRowId;
        stmt.prepare(db, sql.str());
        sql.str("");
        {
            SQLite3Tx sqlite3Tx(db, SQLite3Tx::Type::IMMEDIATE);

            while (stmt.step() == SQLITE_ROW) {
                uint64_t txRowId = strtoull((char*)stmt.getText(0), NULL, 0);
                sql << "UPDATE `txs` SET `block_header_id` = 0, `block_index` = NULL "
                    << "WHERE `rowid` = " << txRowId;
                db.query(sql.str()); sql.str("");
            }
            sql << "DELETE FROM `block_headers` WHERE `rowid` = " << headerRowId;
            db.query(sql.str());

            sqlite3Tx.commit();
        }
        return true;
    } 
    return false;
}

bool CoinQTxStoreSqlite3::hasTx(const uchar_vector& txHash) const
{
    if (!db.isOpen()) {
        throw std::runtime_error("Database is not open.");
    }

    std::stringstream sql;
    SQLite3Stmt stmt;

    sql << "SELECT `rowid` FROM `txs` WHERE `hash` = '" << txHash.getHex() << "'";
    stmt.prepare(db, sql.str());
    return (stmt.step() == SQLITE_ROW);
}

bool CoinQTxStoreSqlite3::getTx(uint64_t txId, ChainTransaction& tx) const
{
#ifdef _COINQ_SQLITE_SAFE_
    if (!db.isOpen()) {
        throw std::runtime_error("Database is not open.");
    }
#endif

    std::stringstream sql;
    std::stringstream err;
    SQLite3Stmt stmt;

    sql << "SELECT `version`, `locktime`, `block_header_id`, `block_index`, `merkle_path` "
        << "FROM `txs` WHERE `rowid` = " << txId;
    stmt.prepare(db, sql.str());
    if (stmt.step() != SQLITE_ROW) return false;

    tx.inputs.clear();
    tx.outputs.clear();
    tx.version = stmt.getInt(0);
    tx.lockTime = stmt.getInt(1);
    tx.blockHeader.clear();

    uint64_t blockHeaderRowId = strtoull((char*)stmt.getText(2), NULL, 0);
    int blockIndex = (int)stmt.getInt(3);

    stmt.finalize(); sql.str("");
    sql << "SELECT `height`, `chain_work`, `version`, `prev_block_hash`, `merkle_root`, `timestamp`, `bits`, `nonce` "
        << "FROM `block_headers` WHERE `rowid` = " << blockHeaderRowId;
    stmt.prepare(db, sql.str());
    if (stmt.step() == SQLITE_ROW) {
        ChainHeader& header = tx.blockHeader;
        header.inBestChain = true;
        header.height = (int)stmt.getInt(0);
        header.chainWork.setHex((char*)stmt.getText(1));
        header.version = (uint32_t)stmt.getInt(2);
        header.prevBlockHash.setHex((char*)stmt.getText(3));
        header.merkleRoot.setHex((char*)stmt.getText(4));
        header.timestamp = strtoul((char*)stmt.getText(5), NULL, 0);
        header.bits = strtoul((char*)stmt.getText(6), NULL, 0);
        header.nonce = strtoul((char*)stmt.getText(7), NULL, 0);
        tx.index = blockIndex;
    }

    stmt.finalize(); sql.str("");
    sql << "SELECT `tx_index`, `amount`, `script` FROM `txouts` WHERE `tx_id` = " << txId << " ORDER BY `tx_index`";
    stmt.prepare(db, sql.str());
    int i = 0;
    while (stmt.step() == SQLITE_ROW) {
        if (stmt.getInt(0) != i) {
            err << "Missing transaction output with index " << i << "."; 
            throw std::runtime_error(err.str());
        }

        tx.outputs.push_back(Coin::TxOut(strtoull((char*)stmt.getText(1), NULL, 0), (char*)stmt.getText(2)));
        i++;
    }
    if (i == 0) {
        throw std::runtime_error("Missing all outputs for transaction.");
    }

    stmt.finalize(); sql.str("");
    sql << "SELECT `tx_index`, `out_hash`, `out_index`, `script`, `sequence` "
        << "FROM `txins` WHERE `tx_id` = " << txId << " ORDER BY `tx_index`";
    stmt.prepare(db, sql.str());
    i = 0;
    while (stmt.step() == SQLITE_ROW) {
        if (stmt.getInt(0) != i) {
            err << "Missing transaction input with index " << i << ".";
            throw std::runtime_error(err.str());
        }

        tx.inputs.push_back(Coin::TxIn(Coin::OutPoint((char*)stmt.getText(1), (uint)stmt.getInt(2)), (char*)stmt.getText(3), strtoul((char*)stmt.getText(4), NULL, 0)));
        i++;
    }
    if (i == 0) {
        throw std::runtime_error("Missing all inputs for transaction.");
    }

    return true;
}

bool CoinQTxStoreSqlite3::getTx(const uchar_vector& txHash, ChainTransaction& tx) const
{
    if (!db.isOpen()) {
        throw std::runtime_error("Database is not open.");
    }

    std::stringstream sql;
    SQLite3Stmt stmt;

    sql << "SELECT `rowid` FROM `txs` WHERE `hash` = '" << txHash.getHex() << "'";
    stmt.prepare(db, sql.str());
    if (stmt.step() != SQLITE_ROW) return false;

    return getTx(strtoull((char*)stmt.getText(0), NULL, 0), tx);
}

uchar_vector CoinQTxStoreSqlite3::getBestBlockHash(const uchar_vector& txHash) const
{
    uchar_vector hash;
    return hash;
}

int CoinQTxStoreSqlite3::getBestBlockHeight(const uchar_vector& txHash) const
{
    return -1;
}

std::vector<ChainTransaction> CoinQTxStoreSqlite3::getConfirmedTxs(int minHeight, int maxHeight) const
{
    if (!db.isOpen()) {
        throw std::runtime_error("Database is not open.");
    }

    std::stringstream sql;
    SQLite3Stmt stmt;
    std::vector<ChainTransaction> txs;

//    sql << "SELECT 
    return txs;
}

std::vector<ChainTransaction> CoinQTxStoreSqlite3::getUnconfirmedTxs() const
{
    std::vector<ChainTransaction> txs;
    return txs;
}

std::vector<ChainTxOut> CoinQTxStoreSqlite3::getTxOuts(const CoinQ::Keys::AddressSet& addressSet, Status status, int minConf) const
{
    if (!db.isOpen()) {
        throw std::runtime_error("Database is not open.");
    }

    std::stringstream sql;
    SQLite3Stmt stmt;

    std::vector<ChainTxOut> txOuts;

    const std::set<std::string>& addresses = addressSet.getAddresses();
    if (addresses.size() > 0) {
        sql << "SELECT DISTINCT `txouts`.`amount`, `txouts`.`script`, `txs`.`hash`, `txouts`.`tx_index`, `txouts`.`spent`"
            << " FROM `txouts`, `txs`, `block_headers`"
            << " WHERE `txouts`.`tx_id` = `txs`.`rowid`"
            << " AND `txouts`.`address` IN (";
        int count = 0;
        for (auto& address: addresses) {
            if (count > 0) sql << ", ";
            sql << "'" << address << "'"; 
            count++;
        }
        sql << ")";
        if (status == Status::UNSPENT) {
            sql << " AND `txouts`.`spent` = 0";
        }
        else if (status == Status::SPENT) {
            sql << " AND `txouts`.`spent` = 1";
        }
        
        if (minConf > 0) {
            sql << " AND `txs`.`block_header_id` = `block_headers`.`rowid` AND `block_headers`.`height` <= " << (getBestHeight() + 1 - minConf);
        }
std::cout << sql.str() << std::endl;
        stmt.prepare(db, sql.str());
        while (stmt.step() == SQLITE_ROW) {
            txOuts.push_back(ChainTxOut(Coin::TxOut(strtoull((char*)stmt.getText(0), NULL, 0), (char*)stmt.getText(1)), uchar_vector((char*)stmt.getText(2)), stmt.getInt(3), (stmt.getInt(4) != 0)));
        }
    }
    return txOuts;
}

void CoinQTxStoreSqlite3::setBestHeight(int height)
{
    if (!db.isOpen()) {
        throw std::runtime_error("Database is not open.");
    }

    std::stringstream sql;
    sql << "INSERT OR REPLACE INTO `state` (`rowid`, `best_height`) VALUES (1," << height << ")";
    db.query(sql.str());
    notifyNewBestHeight(height);    
}

int CoinQTxStoreSqlite3::getBestHeight() const
{
    if (!db.isOpen()) {
        throw std::runtime_error("Database is not open.");
    }

    std::stringstream sql;
    SQLite3Stmt stmt;

    sql << "SELECT `best_height` FROM `state`";
    stmt.prepare(db, sql.str());
    if (stmt.step() != SQLITE_ROW) {
        throw std::runtime_error("State table is empty.");
    }

    return stmt.getInt(0);
}

/**
 * class CoinQKeychainSqlite3
*/
void CoinQKeychainSqlite3::open(const std::string& filename, bool bCreateIfNotExists)
{
    std::stringstream sql;
    db.open(filename, bCreateIfNotExists);

    sql << "CREATE TABLE IF NOT EXISTS `keys` ("
        <<      "`hash` TEXT UNIQUE NOT NULL,"
        <<      "`pubkey` TEXT UNIQUE NOT NULL,"
        <<      "`privkey` TEXT NOT NULL,"
        <<      "`type` INTEGER NOT NULL,"
        <<      "`status` INTEGER NOT NULL,"
        <<      "`minheight` INTEGER DEFAULT NULL,"
        <<      "`maxheight` INTEGER DEFAULT NULL,"
        <<      "`label` TEXT DEFAULT NULL,"
        <<      "`time_inserted` INTEGER DEFAULT (strftime('%s', 'now')))";
    db.query(sql.str()); sql.str("");

    sql << "CREATE TABLE IF NOT EXISTS `redeemscripts` ("
        <<      "`hash` TEXT UNIQUE NOT NULL,"
        <<      "`type` INTEGER NOT NULL,"
        <<      "`status` INTEGER NOT NULL,"
        <<      "`minheight` INTEGER DEFAULT NULL,"
        <<      "`maxheight` INTEGER DEFAULT NULL,"
        <<      "`label` TEXT DEFAULT NULL,"
        <<      "`time_inserted` INTEGER DEFAULT (strftime('%s', 'now')))";
    db.query(sql.str()); sql.str("");

    sql << "CREATE TABLE IF NOT EXISTS `multisigs` ("
        <<      "`redeemscript_id` INTEGER NOT NULL,"
        <<      "`pubkey` TEXT NOT NULL,"
        <<      "`index` INTEGER NOT NULL)";
    db.query(sql.str());
}

void CoinQKeychainSqlite3::close()
{
    db.close();
}

void CoinQKeychainSqlite3::generateNewKeys(int count, int type, bool bCompressed)
{
    SQLite3Stmt stmt;

    CoinKey key;
    key.setCompressed(bCompressed);

    stmt.prepare(db, "INSERT INTO `keys` (`hash`, `pubkey`, `privkey`, `type`, `status`, `minheight`, `maxheight`) VALUES (?,?,?,?,?,-1,-1)");
    for (int i = 0; i < count; i++) {
        key.generateNewKey();
        uchar_vector pubkey = key.getPublicKey();
        std::string pubkeyhash = ripemd160(sha256(pubkey)).getHex();
        std::string pubkeyhex = pubkey.getHex();
        uchar_vector privkey = key.getPrivateKey();
        std::string privkeyhex = privkey.getHex();
        stmt.reset();
        stmt.bindText(1, pubkeyhash);
        stmt.bindText(2, pubkeyhex);
        stmt.bindText(3, privkeyhex);
        stmt.bindInt (4, type);
        stmt.bindInt (5, Status::POOL);
        stmt.step();
    }
}

int CoinQKeychainSqlite3::getPoolSize(int type) const
{
    if (type < 1 || type > 3) {
        throw CoinQ::Exception("Invalid type.", CoinQ::Exception::INVALID_PARAMETERS);
    }

    std::stringstream sql;
    SQLite3Stmt stmt;

    sql << "SELECT COUNT(`rowid`) FROM `keys` WHERE `status` = " << Status::POOL << " AND ("
        << "`type` = " << (type & Type::RECEIVING) << " OR `type` = " << (type & Type::CHANGE) << ")";

    stmt.prepare(db, sql.str());
    
    return (stmt.step() == SQLITE_ROW) ? stmt.getInt(0) : 0;
}

// NEEDS TO BE TESTED
void CoinQKeychainSqlite3::insertKey(const uchar_vector& privKey, int type, int status, const std::string& label, bool bCompressed, int minHeight, int maxHeight)
{
    if (type < 1 || type > 2) {
        throw CoinQ::Exception("Invalid type.", CoinQ::Exception::INVALID_PARAMETERS);
    }

    if (status < 1 || status > 2) {
        throw CoinQ::Exception("Invalid status.", CoinQ::Exception::INVALID_PARAMETERS);
    }

    CoinKey key;
    key.setCompressed(bCompressed);
    key.setPrivateKey(privKey);
    uchar_vector pubkey = key.getPublicKey();
    std::string pubkeyhash = ripemd160(sha256(pubkey)).getHex();
    std::string pubkeyhex = pubkey.getHex();
    std::string privkeyhex = privKey.getHex();

    SQLite3Stmt stmt;
    stmt.prepare(db, "INSERT INTO `keys` (`hash`, `pubkey`, `privkey`, `type`, `status`, `minheight`, `maxheight`, `label`) VALUES (?,?,?,?,?,?,?,?)");
    stmt.bindText(1, pubkeyhash);
    stmt.bindText(2, pubkeyhex);
    stmt.bindText(3, privkeyhex);
    stmt.bindInt (4, type);
    stmt.bindInt (5, status);
    stmt.bindInt (6, minHeight);
    stmt.bindInt (7, maxHeight);
    stmt.bindText(8, label);
    stmt.step();
    stmt.finalize();
}

void CoinQKeychainSqlite3::insertMultiSigRedeemScript(const std::vector<uchar_vector>& pubKeys, int type, int status, const std::string& label, int minHeight, int maxHeight)
{
    if (type < 1 || type > 2) {
        throw CoinQ::Exception("Invalid type.", CoinQ::Exception::INVALID_PARAMETERS);
    }

    if (status < 1 || status > 2) {
        throw CoinQ::Exception("Invalid status.", CoinQ::Exception::INVALID_PARAMETERS);
    }

    Coin::MultiSigRedeemScript multiSig;
    for (auto& pubKey: pubKeys) {
        multiSig.addPubKey(pubKey);
    }

    SQLite3Tx sqliteTx(db, SQLite3Tx::IMMEDIATE);
    SQLite3Stmt stmt;
    stmt.prepare(db, "INSERT INTO `redeemscripts` (`hash`, `type`, `status`, `minheight`, `maxheight`, `label`) VALUES (?,?,?,?,?,?)");
    stmt.bindText(1, ripemd160(sha256(multiSig.getRedeemScript())).getHex());
    stmt.bindInt (2, type);
    stmt.bindInt (3, status);
    stmt.bindInt (4, minHeight);
    stmt.bindInt (5, maxHeight);
    stmt.bindText(6, label);
    stmt.step();
    stmt.finalize();

    std::stringstream sql;
    sql << "INSERT INTO `multisigs` (`redeemscript_id`, `pubkey`, `index`) VALUES (" << db.lastInsertRowId() << ",?,?)";
    stmt.prepare(db, sql.str());

    int i = 0;
    for (auto& pubKey: pubKeys) {
        stmt.reset();
        stmt.bindText(1, pubKey.getHex());
        stmt.bindInt (2, i++);
        stmt.step();
    }
}

std::vector<uchar_vector> CoinQKeychainSqlite3::getKeyHashes(int type, int status) const
{
    SQLite3Stmt stmt;
    stmt.prepare(db, "SELECT `hash` FROM `keys` WHERE (`type` = ? OR `type` = ?) AND (`status` = ? OR `status` = ?)");
    stmt.bindInt (1, type & Type::RECEIVING);
    stmt.bindInt (2, type & Type::CHANGE);
    stmt.bindInt (3, status & Status::POOL);
    stmt.bindInt (4, status & Status::ALLOCATED);

    std::vector<uchar_vector> hashes;
    while (stmt.step() == SQLITE_ROW) {
        hashes.push_back(uchar_vector((char*)stmt.getText(0)));
    }    
    
    return hashes;
}

std::vector<uchar_vector> CoinQKeychainSqlite3::getRedeemScriptHashes(int type, int status) const
{

    SQLite3Stmt stmt;
    stmt.prepare(db, "SELECT `hash` FROM `redeemscripts` WHERE (`type` = ? OR `type` = ?) AND (`status` = ? OR `status` = ?)");
    stmt.bindInt (1, type & Type::RECEIVING);
    stmt.bindInt (2, type & Type::CHANGE);
    stmt.bindInt (3, status & Status::POOL);
    stmt.bindInt (4, status & Status::ALLOCATED);

    std::vector<uchar_vector> hashes;
    while (stmt.step() == SQLITE_ROW) {
        hashes.push_back(uchar_vector((char*)stmt.getText(0)));
    }    
    
    return hashes;
}

uchar_vector CoinQKeychainSqlite3::getPublicKeyFromHash(const uchar_vector& keyHash) const
{
    SQLite3Stmt stmt;
    stmt.prepare(db, "SELECT `pubkey` from `keys` WHERE `hash` = ?");
    stmt.bindText(1, keyHash.getHex());
    if (stmt.step() == SQLITE_ROW) {
        return uchar_vector((char*)stmt.getText(0));
    }

    throw CoinQ::Exception("Not found.", CoinQ::Exception::NOT_FOUND);
}

uchar_vector CoinQKeychainSqlite3::getPrivateKeyFromHash(const uchar_vector& keyHash) const
{
    SQLite3Stmt stmt;
    stmt.prepare(db, "SELECT `privkey` from `keys` WHERE `hash` = ?");
    stmt.bindText(1, keyHash.getHex());
    if (stmt.step() == SQLITE_ROW) {
        return uchar_vector((char*)stmt.getText(0));
    }

    throw CoinQ::Exception("Not found.", CoinQ::Exception::NOT_FOUND);
}

uchar_vector CoinQKeychainSqlite3::getPrivateKeyFromPublicKey(const uchar_vector& pubKey) const
{
    SQLite3Stmt stmt;
    stmt.prepare(db, "SELECT `privkey` from `keys` WHERE `pubkey` = ?");
    stmt.bindText(1, pubKey.getHex());
    if (stmt.step() == SQLITE_ROW) {
        return uchar_vector((char*)stmt.getText(0));
    }

    throw CoinQ::Exception("Not found.", CoinQ::Exception::NOT_FOUND);
}

uchar_vector CoinQKeychainSqlite3::getNextPoolPublicKey(int type) const
{
    SQLite3Stmt stmt;
    stmt.prepare(db, "SELECT `pubkey` from `keys` WHERE `type` IN (?,?) AND `status` = 1 ORDER BY `rowid` LIMIT 1");
    stmt.bindInt (1, type & Type::RECEIVING);
    stmt.bindInt (2, type & Type::CHANGE);
    if (stmt.step() == SQLITE_ROW) {
        return uchar_vector((char*)stmt.getText(0));
    }

    throw CoinQ::Exception("Pool is empty.", CoinQ::Exception::NOT_FOUND);
}

uchar_vector CoinQKeychainSqlite3::getNextPoolPublicKeyHash(int type) const
{
    return ripemd160(sha256(getNextPoolPublicKey(type)));
}

void CoinQKeychainSqlite3::removeKeyFromPool(const uchar_vector& keyHash) const
{
    SQLite3Stmt stmt;
    stmt.prepare(db, "UPDATE `keys` SET `status` = 2 WHERE `hash` = ?");
    stmt.bindText(1, keyHash.getHex());
    stmt.step();
}

uchar_vector CoinQKeychainSqlite3::getRedeemScript(const uchar_vector& scriptHash) const
{
    SQLite3Stmt stmt;
    stmt.prepare(db, "SELECT `multisigs`.`pubkey` FROM `multisigs`, `redeemscripts` WHERE `redeemscripts`.`hash` = ? AND `redeemscripts`.id` = `multisigs`.redeemscript_id` ORDER BY `multisigs`.`index`");
    stmt.bindText(1, scriptHash.getHex());

    Coin::MultiSigRedeemScript multiSig;
    while (stmt.step() == SQLITE_ROW) {
        multiSig.addPubKey(uchar_vector((char*)stmt.getText(0)));
    }
    if (multiSig.getPubKeyCount() > 0) {
        return multiSig.getRedeemScript();
    }

    throw CoinQ::Exception("Not found.", CoinQ::Exception::NOT_FOUND);
}

uchar_vector CoinQKeychainSqlite3::getNextPoolRedeemScript(int type) const
{
    SQLite3Stmt stmt;
    stmt.prepare(db, "SELECT `hash` FROM `redeemscripts` WHERE `status` = 1 AND `type` = ? ORDER BY `rowid` LIMIT 1");
    if (stmt.step() == SQLITE_ROW) {
        return getRedeemScript(uchar_vector((char*)stmt.getText(0)));
    }

    throw CoinQ::Exception("Pool is empty.", CoinQ::Exception::NOT_FOUND);
}

uchar_vector CoinQKeychainSqlite3::getNextPoolRedeemScriptHash(int type) const
{
    return ripemd160(sha256(getNextPoolRedeemScript(type)));
}

void CoinQKeychainSqlite3::removeRedeemScriptFromPool(const uchar_vector& scriptHash)
{
    SQLite3Stmt stmt;
    stmt.prepare(db, "UPDATE `redeemscript` SET `status` = 2 WHERE `hash` = ?");
    stmt.bindText(1, scriptHash.getHex());
    stmt.step();
}

uchar_vector CoinQKeychainSqlite3::getUnsignedScriptSig(const uchar_vector& scriptPubKey) const
{
    CoinQ::Script::payee_t payee = CoinQ::Script::getScriptPubKeyPayee(scriptPubKey);

    switch (payee.first) {
    case CoinQ::Script::SCRIPT_PUBKEY_PAY_TO_PUBKEY_HASH:
    {
        uchar_vector pubKey = getPublicKeyFromHash(payee.second);
        Coin::P2AddressTxIn txIn;
        txIn.addPubKey(pubKey);
        txIn.setScriptSig(Coin::SCRIPT_SIG_EDIT);
        return txIn.scriptSig;
    }

    case CoinQ::Script::SCRIPT_PUBKEY_PAY_TO_SCRIPT_HASH:
    {
        uchar_vector redeemScript = getRedeemScript(payee.second);
        Coin::MofNTxIn txIn;
        txIn.setRedeemScript(redeemScript);
        txIn.setScriptSig(Coin::SCRIPT_SIG_EDIT);
        return txIn.scriptSig;
    }

    case CoinQ::Script::SCRIPT_PUBKEY_PAY_TO_PUBKEY:
    {
        uchar_vector scriptSig;
        scriptSig.push_back(0);
        return scriptSig;
    }

    default:
        throw CoinQ::Keys::KeychainException("Unsupported type.", CoinQ::Keys::KeychainException::UNSUPPORTED_TYPE);
    }
}

/*
void CoinQKeychainSqlite3::removeUsedKeysFromPool(const Coin::Transaction& tx)
{
}
  
*/
  
uchar_vector CoinQKeychainSqlite3::getKeysOnlyHash() const
{
    return uchar_vector();
}

uchar_vector CoinQKeychainSqlite3::getFullKeychainHash() const
{
    return uchar_vector();
}
