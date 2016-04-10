///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// scriptmodel.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

#include <QStandardItemModel>

#include <CoinDB/Vault.h>

class ScriptModel : public QStandardItemModel
{
    Q_OBJECT

public:
    ScriptModel(QObject* parent = NULL);
    ScriptModel(CoinDB::Vault* vault, const QString& accountName, QObject* parent = NULL);

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
};

