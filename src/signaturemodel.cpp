///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// signaturemodel.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

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
    columns << tr("Keychain Name") << tr("") << tr("") << tr("Keychain Hash") << "";
    setHorizontalHeaderLabels(columns);
}

void SignatureModel::updateAll()
{
    removeRows(0, rowCount());

    if (!m_synchedVault.isVaultOpen()) return;

    CoinDB::Vault* vault = m_synchedVault.getVault();
    SignatureInfo signatureInfo = vault->getSignatureInfo(m_txHash);
    m_sigsNeeded = signatureInfo.sigsNeeded();
    for (auto& keychain: signatureInfo.signingKeychains())
    {
        QList<QStandardItem*> row;

        // Keychain name
        QString name = QString::fromStdString(keychain.name());
        QStandardItem* nameItem = new QStandardItem(name);
        row.append(nameItem);

        // Keychain encryption
        QStandardItem* encryptedItem = new QStandardItem();
        if (vault->isKeychainEncrypted(keychain.name()))
        {
            encryptedItem->setIcon(QIcon(":/icons/encrypted.png"));
            encryptedItem->setData(true, Qt::UserRole);
        }
        else
        {
            encryptedItem->setData(false, Qt::UserRole);
        }
        row.append(encryptedItem);

        // Keychain state
        QStandardItem* stateItem = new QStandardItem();
        if (!vault->isKeychainPrivate(keychain.name()))
        {
            stateItem->setIcon(QIcon(":/icons/shared.png"));
            stateItem->setData(PUBLIC, Qt::UserRole);
        }
        else if (vault->isKeychainLocked(keychain.name()))
        {
            stateItem->setIcon(QIcon(":/icons/locked.png"));
            stateItem->setData(LOCKED, Qt::UserRole);
        }
        else
        {
            stateItem->setIcon(QIcon(":/icons/unlocked.png"));
            stateItem->setData(UNLOCKED, Qt::UserRole);
        }
        row.append(stateItem);

        // Keychain hash
        QString hashStr = QString::fromStdString(uchar_vector(keychain.hash()).getHex());
        row.append(new QStandardItem(hashStr));

        // Has signed
        bool hasSigned = keychain.hasSigned();
        QString hasSignedStr = hasSigned ? tr("Signed") : tr("");
        QStandardItem* hasSignedItem = new QStandardItem(hasSignedStr);
        hasSignedItem->setData(hasSigned, Qt::UserRole);
        row.append(hasSignedItem);

        appendRow(row);
    }
}

bool SignatureModel::isKeychainEncrypted(int row) const
{
    if (row < 0 || row > rowCount()) throw std::runtime_error("Invalid row.");

    return item(row, 1)->data(Qt::UserRole).toBool();
}

int SignatureModel::getKeychainState(int row) const
{
    if (row < 0 || row > rowCount()) throw std::runtime_error("Invalid row.");

    return item(row, 2)->data(Qt::UserRole).toInt();
}

bool SignatureModel::getKeychainHasSigned(int row) const
{
    if (row < 0 || row > rowCount()) throw std::runtime_error("Invalid row.");

    return item(row, 4)->data(Qt::UserRole).toBool();
}

Qt::ItemFlags SignatureModel::flags(const QModelIndex& /*index*/) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QVariant SignatureModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::BackgroundRole)
    {
        if (getKeychainHasSigned(index.row())) return QBrush(QColor(184, 207, 233)); // light blue
    }

    return QStandardItemModel::data(index, role);
}

