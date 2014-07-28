///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// txmodel.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef COINVAULT_TXMODEL_H
#define COINVAULT_TXMODEL_H

#include <QStandardItemModel>

#include <CoinDB/Vault.h>

namespace CoinQ {
    namespace Network {
        class NetworkSync;
    }
}

class TxModel : public QStandardItemModel
{
    Q_OBJECT

public:
    TxModel(QObject* parent = NULL);
    TxModel(CoinDB::Vault* vault, const QString& accountName, QObject* parent = NULL);

    void setVault(CoinDB::Vault* vault);
    void setAccount(const QString& accountName);
    void update();

    void signTx(int row);
    void sendTx(int row, CoinQ::Network::NetworkSync* networkSync);
    std::shared_ptr<CoinDB::Tx> getTx(int row);
    void deleteTx(int row);

    // Overridden methods
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex& index) const;

signals:
    void txSigned(const QString& keychainNames);
    void txDeleted();

private:
    unsigned char base58_versions[2];
    QString currencySymbol;

    void setColumns();

    CoinDB::Vault* vault;
    QString accountName; // empty when not loaded
    uint64_t confirmedBalance;
    uint64_t pendingBalance;
};

#endif // COINVAULT_TXMODEL_H
