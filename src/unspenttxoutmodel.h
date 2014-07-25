///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// unspenttxoutmodel.h
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

#include <QStandardItemModel>

#include <CoinDB/Vault.h>

class UnspentTxOutModel : public QStandardItemModel
{
    Q_OBJECT

public:
    UnspentTxOutModel(QObject* parent = nullptr);
    UnspentTxOutModel(CoinDB::Vault* vault, const QString& accountName, QObject* parent = nullptr);

    void setVault(CoinDB::Vault* vault);
    void setAccount(const QString& accountName);
    void update();

    // Overridden methods
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex& index) const;

signals:

private:
    unsigned char base58_versions[2];
    uint64_t currency_divisor;
    const char* currency_symbol;

    void initColumns();

    CoinDB::Vault* vault;
    QString accountName; // empty when not loaded
};

