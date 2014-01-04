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

#include <CoinQ_vault.h>

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
    TxModel(CoinQ::Vault::Vault* vault, const QString& accountName, QObject* parent = NULL);

    void setVault(CoinQ::Vault::Vault* vault);
    void setAccount(const QString& accountName);
    void update();

    void signTx(int row);
    void sendTx(int row, CoinQ::Network::NetworkSync* networkSync);
    std::shared_ptr<CoinQ::Vault::Tx> getTx(int row);
    void deleteTx(int row);

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

signals:
    void txDeleted();

private:
    void initColumns();

    CoinQ::Vault::Vault* vault;
    QString accountName; // empty when not loaded
    uint64_t confirmedBalance;
    uint64_t pendingBalance;
};

#endif // COINVAULT_TXMODEL_H
