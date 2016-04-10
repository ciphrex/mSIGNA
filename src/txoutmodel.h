///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// txoutmodel.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

#include <QStandardItemModel>

#include <Vault.h>

class TxOutModel : public QStandardItemModel
{
    Q_OBJECT

public:
    TxOutModel();

    void setVault(CoinDB::Vault* vault);
    void setAccount(const QString& accountName);
    void update();

private:
    CoinDB::Vault* vault;
    QString accountName; // empty when not loaded
    uint64_t confirmedBalance;
    uint64_t pendingBalance;
};

