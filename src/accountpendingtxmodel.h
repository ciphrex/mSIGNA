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

#include <CoinQ_vault.h>

class AccountPendingTxModel : public QStandardItemModel
{
    Q_OBJECT

public:
    AccountPendingTxModel(QObject* parent = NULL);
    AccountPendingTxModel(CoinQ::Vault::Vault* vault, const QString& accountName, QObject* parent = NULL);

    void setVault(CoinQ::Vault::Vault* vault);
    void setAccount(const QString& accountName);
    void update();

private:
    void initColumns();

    CoinQ::Vault::Vault* vault;
    QString accountName; // empty when not loaded
    uint64_t confirmedBalance;
    uint64_t pendingBalance;
};

#endif // COINVAULT_ACCOUNTPENDINGTXMODEL_H
