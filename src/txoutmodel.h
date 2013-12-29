///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// txoutmodel.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef COINVAULT_TXOUTMODEL_H
#define COINVAULT_TXOUTMODEL_H

#include <QStandardItemModel>

#include <CoinQ_vault.h>

class TxOutModel : public QStandardItemModel
{
    Q_OBJECT

public:
    TxOutModel();

    void setVault(CoinQ::Vault::Vault* vault);
    void setAccount(const QString& accountName);
    void update();

private:
    CoinQ::Vault::Vault* vault;
    QString accountName; // empty when not loaded
    uint64_t confirmedBalance;
    uint64_t pendingBalance;
};

#endif // COINVAULT_TXOUTMODEL_H
