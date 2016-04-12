///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// accountpendingtxmodel.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

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

