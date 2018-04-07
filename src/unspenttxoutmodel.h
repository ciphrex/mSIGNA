///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// unspenttxoutmodel.h
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

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

    //uint64_t getCurrencyDivisor() const { return currency_divisor; }
    //const char* getCurrencySymbol() const { return currency_symbol; }

signals:

private:
    unsigned char base58_versions[2];
    //uint64_t currency_divisor;
    //const char* currency_symbol;

    void initColumns();

    CoinDB::Vault* vault;
    QString accountName; // empty when not loaded
};

