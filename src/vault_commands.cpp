///////////////////////////////////////////////////////////////////////////////
//
// vault_commands.cpp
//
// Copyright (c) 2011-2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "vault_commands.h"

#include <CoinKey.h>
#include <StandardTransactions.h>

#include <sqlite3cpp.h>

#include <sstream>
#include <fstream>
#include <stdexcept>

using namespace Coin;
using namespace std;

result_t vault_newkey(bool bHelp, const params_t& params)
{
    if (bHelp || params.size() > 1) {
        return "newkey [-u | --uncompressed] - generates a new secp256k1 key.";
    }

    stringstream err;

    bool bCompressed = true;
    if (params.size() > 0) {
        if (params[0] == "-u" || params[0] == "--uncompressed") {
            bCompressed = false;
        }
        else {
            err << "Invalid option " << params[0] << ".";            
            throw runtime_error(err.str());
        } 
    }

    CoinKey key;
    key.setCompressed(bCompressed);
    key.generateNewKey();
    return key.getWalletImport();
}

result_t vault_pubkey(bool bHelp, const params_t& params)
{
    if (bHelp || params.size() != 1) {
        return "pubkey <privkey> - returns the public key associated with the private key.";
    }

    CoinKey key;
    if (!key.setWalletImport(params[0])) {
        throw runtime_error("Invalid key.");
    }
    return key.getPublicKey().getHex();
}

result_t vault_addr(bool bHelp, const params_t& params)
{
    if (bHelp || params.size() != 1) {
        return "addr <privkey | pubkey> - returns base58check encoded hash of public key.";
    }

    CoinKey key;
    if (!key.setPublicKey(params[0]) && !key.setWalletImport(params[0])) {
        throw runtime_error("Invalid key.");
    }
    return key.getAddress();
}

result_t vault_newwallet(bool bHelp, const params_t& params)
{
    if (bHelp || params.size() != 2) {
        return "newwallet <walletfile> <nKeys> - creates a new wallet.";
    }

    ifstream ifile(params[0]);
    if (ifile) {
        throw runtime_error("File already exists.");
    }

    SQLite3DB db(params[0]);
    db.query("CREATE TABLE IF NOT EXISTS `keys` (`pubkey` varchar(66) PRIMARY KEY NOT NULL, `address` varchar(40) UNIQUE NOT NULL, `privkey` varchar(66), `status` INTEGER NOT NULL DEFAULT 0)");


    uint nKeys = strtoul(params[1].c_str(), NULL, 10);
    CoinKey key;
    for (uint i = 0; i < nKeys; i++) {
        key.generateNewKey();
        stringstream sql;
        sql << "INSERT INTO `keys` (`pubkey`, `address`, `privkey`) VALUES ('"
            << key.getPublicKey().getHex() << "', '" << key.getAddress() << "', '" << key.getWalletImport() << "')";

        db.query(sql.str());
    }

    stringstream result;
    result << "Created new database " << params[0] << " with " << nKeys << " key" << (nKeys == 1 ? "" : "s") << ".";
    return result.str();
}

result_t vault_importkeys(bool bHelp, const params_t& params)
{
    if (bHelp || params.size() != 2) {
        return "importkeys <importfile> <walletfile> - imports a newline-delimited list of base58-encoded private keys into a wallet.";
    }

    SQLite3DB db(params[1], false);
    SQLite3Tx sqliteTx(db, SQLite3Tx::Type::IMMEDIATE);

    int counter = 0;
    CoinKey key;
    string privkey;
    ifstream fs(params[0]);
    getline(fs, privkey);
    while (fs.good()) {
        key.setWalletImport(privkey);
        stringstream sql;
        sql << "INSERT OR IGNORE INTO `keys` (`pubkey`, `address`, `privkey`) VALUES ('"
            << key.getPublicKey().getHex() << "', '" << key.getAddress() << "', '" << key.getWalletImport() << "')";

        db.query(sql.str());
        counter++;

        getline(fs, privkey);
    }
    if (fs.bad()) {
        throw std::runtime_error("Error reading keys from importfile.");
    }

    sqliteTx.commit();
    stringstream result;
    result << "Imported " << counter << "keys.";
    return result.str();
}

result_t vault_dumpkeys(bool bHelp, const params_t& params)
{
    if (bHelp || params.size() < 2 || params.size() > 3) {
        return "dumpkeys <walletfile> <addressfile> [addressonly=true] - dumps addresses in wallet to file.";
    }

    bool bAddressOnly = (params.size() > 2) ? (params[2] != "false") : true;
    SQLite3DB db(params[0], false);

    ofstream fs(params[1]);

    int count = 0;
    stringstream sql;
    sql << "SELECT " << (bAddressOnly ? "`address`" : "`address`, `pubkey`, `privkey`, `status`") << " FROM `keys`";
    SQLite3Stmt stmt;
    stmt.prepare(db, sql.str());
    while (stmt.step() == SQLITE_ROW) {
        fs << (char*)stmt.getText(0);
        if (!bAddressOnly) {
            fs << " " << (char*)stmt.getText(1) << " " << (char*)stmt.getText(2) << " " << stmt.getInt(3);
        }
        fs << endl;
        if (fs.bad()) throw std::runtime_error("Error writing keys to file.");
        count++;
    }

    stringstream result;
    result << "Dumped " << count << "keys.";
    return result.str();
}

result_t vault_walletkey(bool bHelp, const params_t& params)
{
    if (bHelp || params.size() != 2) {
        return "walletkey <filename> <id> - displays key information.";
    }

    SQLite3DB db(params[0], false);
    uint i = strtoul(params[1].c_str(), NULL, 10);

    stringstream sql;
    sql << "SELECT `pubkey`, `address`, `privkey`, `status` FROM `keys` WHERE `rowid` = " << i;
    SQLite3Stmt stmt(db, sql.str());

    int nRows = 0;
    stringstream result;
    while (stmt.step() == SQLITE_ROW) {
        nRows++;
        result << stmt.getText(0) << ", " << stmt.getText(1) << ", " << stmt.getText(2) << ", " << stmt.getInt(3) << endl;
    }

   if (nRows > 0) {
        return result.str();
    }
    else {
        return "Key not found.";
    }
}

result_t vault_walletsign(bool bHelp, const params_t& params)
{
    if (bHelp || params.size() != 3) {
        return "walletsign <filename> <txblob> <inputIndex | -a | --all> - signs transaction.";
    }

    TransactionBuilder txBuilder;
    txBuilder.setSerialized(params[1]);
    const std::vector<InputSigRequest>& missingSigs = txBuilder.getMissingSigs();

    SQLite3DB db(params[0], false);

    int index;
    if (params[2] == "-a" || params[2] == "--all") {
        index = -1;
    }
    else {
        index = (int)strtoul(params[2].c_str(), NULL, 10);
    }

    uint totalSigsNeeded = 0;
    uint sigsAdded = 0;
    for (auto it = missingSigs.begin(); it != missingSigs.end(); ++it) {
        if (it->minSigsStillNeeded == 0) continue;
        uint sigsNeeded = it->minSigsStillNeeded;
        totalSigsNeeded += sigsNeeded;
        if (index >= 0 && (uint)index != it->inputIndex) continue;

        stringstream sql;
        sql << "SELECT `pubkey`, `privkey` FROM `keys` WHERE `pubkey` IN (";
        for (auto itKey = it->pubKeys.begin(); itKey != it->pubKeys.end(); ++itKey) {
            if (itKey != it->pubKeys.begin()) sql << ",";
            sql << "'" << itKey->getHex() << "'";
        }
        sql << ")";

        SQLite3Stmt stmt(db, sql.str());
        while (sigsNeeded > 0 && stmt.step() == SQLITE_ROW) {
            txBuilder.sign(it->inputIndex, uchar_vector((char*)stmt.getText(0)), (char*)stmt.getText(1));
            sigsAdded++;
        }
    }

    stringstream result;
    result << "{\n    \"sigsAdded\" : " << sigsAdded
           << ",\n    \"sigsStillNeeded\" : " << (totalSigsNeeded - sigsAdded)
           << ",\n    \"txblob\" : \"" << txBuilder.getSerialized().getHex() << "\""
           << "\n}";
    return result.str(); 
}
