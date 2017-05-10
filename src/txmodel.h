///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// txmodel.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include <QStandardItemModel>

#include <CoinDB/Vault.h>

namespace CoinDB
{
    class SynchedVault;
}

class TxModel : public QStandardItemModel
{
    Q_OBJECT

public:
    enum TxType { NONE, SEND, RECEIVE, UNKNOWN };

    TxModel(QObject* parent = nullptr);
    TxModel(CoinDB::Vault* vault, const QString& accountName, QObject* parent = nullptr);

    void setBase58Versions();

    void setVault(CoinDB::Vault* vault);
    CoinDB::Vault* getVault() const { return vault; }
    void setAccount(const QString& accountName);
    void update();

    bytes_t getTxHash(int row) const;
    int getTxStatus(int row) const;
    int getTxConfirmations(int row) const;
    int getTxOutType(int row) const;
    QString getTxOutAddress(int row) const;

    void signTx(int row);
    void sendTx(int row, CoinDB::SynchedVault* synchedVault);
    std::shared_ptr<CoinDB::Tx> getTx(int row);
    void deleteTx(int row);

    // Overridden methods
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex& index) const;
 
signals:
    void txSigned(const QString& keychainNames);
    void txDeleted();
    void error(const QString& message);

private:
    unsigned char base58_versions[2];
    QString currencySymbol;

    void setColumns();

    CoinDB::Vault* vault;
    QString accountName; // empty when not loaded
    uint64_t confirmedBalance;
    uint64_t pendingBalance;
};

