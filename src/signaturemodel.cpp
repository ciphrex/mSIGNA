///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// signaturemodel.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#include "signaturemodel.h"

#include <CoinDB/SynchedVault.h>

#include <QStandardItemModel>

using namespace CoinDB;
using namespace std;

SignatureModel::SignatureModel(CoinDB::SynchedVault& synchedVault, const bytes_t& txHash, QObject* parent)
    : QStandardItemModel(parent), m_synchedVault(synchedVault), m_txHash(txHash)
{
    initColumns();
}

void SignatureModel::initColumns()
{
    QStringList columns;
    columns << tr("Keychain Name") << tr("Keychain Hash") << tr("Signed");
    setHorizontalHeaderLabels(columns);
}

void SignatureModel::update()
{
    removeRows(0, rowCount());

    if (!m_synchedVault.isVaultOpen()) return;

    CoinDB::Vault* vault = m_synchedVault.getVault();
    SignatureInfo signatureInfo = vault->getSignatureInfo(m_txHash);
    m_sigsNeeded = signatureInfo.sigsNeeded();
    for (auto& signingKeychain: signatureInfo.signingKeychains())
    {
        QList<QStandardItem*> row;
        QString keychainName = QString::fromStdString(signingKeychain.name());
        QString keychainHash = QString::fromStdString(uchar_vector(signingKeychain.hash()).getHex());
        QString hasSigned = signingKeychain.hasSigned() ? tr("Yes") : tr("No");

        row.append(new QStandardItem(keychainName));
        row.append(new QStandardItem(keychainHash));
        row.append(new QStandardItem(hasSigned));
        appendRow(row);
    }
}

Qt::ItemFlags SignatureModel::flags(const QModelIndex& /*index*/) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

