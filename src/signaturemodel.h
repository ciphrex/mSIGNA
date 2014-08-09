///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// signaturemodel.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

#include <CoinCore/typedefs.h>

#include <QStandardItemModel>

namespace CoinDB { class SynchedVault; }

class SignatureModel : public QStandardItemModel
{
    Q_OBJECT

public:
    SignatureModel(QObject* parent = nullptr);
    SignatureModel(CoinDB::SynchedVault& synchedVault, const bytes_t& txHash, QObject* parent = nullptr);

    void setTxHash(const bytes_t& txHash);
    const bytes_t getTxHash() const { return m_txHash; }

    void updateAll();

    unsigned int getSigsNeeded() const { return m_sigsNeeded; }

    Qt::ItemFlags flags(const QModelIndex& /*index*/) const;

signals:
    void txUpdated();

private:
    void initColumns();

    CoinDB::SynchedVault& m_synchedVault;
    bytes_t m_txHash;
    unsigned int m_sigsNeeded;
};

