///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// signaturemodel.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include <CoinCore/typedefs.h>

#include <QStandardItemModel>

namespace CoinDB { class SynchedVault; }

class SignatureModel : public QStandardItemModel
{
    Q_OBJECT

public:
    enum { PUBLIC, UNLOCKED, LOCKED };

    SignatureModel(QObject* parent = nullptr);
    SignatureModel(CoinDB::SynchedVault& synchedVault, const bytes_t& txHash, QObject* parent = nullptr);

    void setTxHash(const bytes_t& txHash);
    const bytes_t getTxHash() const { return m_txHash; }

    void updateAll();

    unsigned int getSigsNeeded() const { return m_sigsNeeded; }

    bool isKeychainEncrypted(int row) const;
    int getKeychainState(int row) const;
    bool getKeychainHasSigned(int row) const;

    Qt::ItemFlags flags(const QModelIndex& index) const;

    QVariant data(const QModelIndex& index, int role) const;

signals:
    void txUpdated();

private:
    void initColumns();

    CoinDB::SynchedVault& m_synchedVault;
    bytes_t m_txHash;
    unsigned int m_sigsNeeded;
};

