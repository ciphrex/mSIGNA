///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// scriptmodel.h
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

class ScriptModel : public QStandardItemModel
{
    Q_OBJECT

public:
    ScriptModel(QObject* parent = NULL);
    ScriptModel(CoinDB::Vault* vault, const QString& accountName, QObject* parent = NULL);

    void setBase58Versions();

    void setVault(CoinDB::Vault* vault);
    void setAccount(const QString& accountName);
    void update();

    // Overriden methods
    bool setData(const QModelIndex& index, const QVariant& value, int role);
    Qt::ItemFlags flags(const QModelIndex& index) const;

private:
    void initColumns();

    CoinDB::Vault* vault;
    QString accountName; // empty when not loaded

    unsigned char base58_versions[2];
};

