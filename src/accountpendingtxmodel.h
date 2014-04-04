///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// accountpendingtxmodel.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef COINVAULT_ACCOUNTPENDINGTXMODEL_H
#define COINVAULT_ACCOUNTPENDINGTXMODEL_H

#include <QStandardItemModel>

#include <Vault.h>

class AccountPendingTxModel : public QStandardItemModel
{
    Q_OBJECT

public:
    AccountPendingTxModel(QObject* parent = NULL);
    AccountPendingTxModel(CoinDB::Vault* vault, const QString& accountName, QObject* parent = NULL);

    void setVault(CoinDB::Vault* vault);
    void setAccount(const QString& accountName);
    void update();

private:
    void initColumns();

    CoinDB::Vault* vault;
    QString accountName; // empty when not loaded
    uint64_t confirmedBalance;
    uint64_t pendingBalance;
};

#endif // COINVAULT_ACCOUNTPENDINGTXMODEL_H
