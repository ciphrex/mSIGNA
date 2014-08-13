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
    columns << tr("Keychain Name") << tr("State") << tr("Keychain Hash") << "";
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

        // Keychain state
        int state;
        QString stateStr;
        if (vault->isKeychainPrivate(keychain.name()))
        {
            bool isLocked = vault->isKeychainPrivateKeyLocked(keychain.name());
            state = isLocked ? LOCKED : UNLOCKED;
            stateStr = isLocked ? tr("Locked") : tr("Unlocked");
        }
        else
        {
            state = PUBLIC;
            stateStr = tr("Public");
        }
        QStandardItem* stateItem = new QStandardItem(stateStr);
        stateItem->setData(state, Qt::UserRole);
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

int SignatureModel::getKeychainState(int row) const
{
    if (row < 0 || row > rowCount()) throw std::runtime_error("Invalid row.");

    QStandardItem* stateItem = item(row, 1);
    return stateItem->data(Qt::UserRole).toInt();
}

bool SignatureModel::getKeychainHasSigned(int row) const
{
    if (row < 0 || row > rowCount()) throw std::runtime_error("Invalid row.");

    QStandardItem* hasSignedItem = item(row, 3);
    return hasSignedItem->data(Qt::UserRole).toBool();
}

Qt::ItemFlags SignatureModel::flags(const QModelIndex& /*index*/) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QVariant SignatureModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::BackgroundRole)
    {
        if (getKeychainHasSigned(index.row())) return QBrush(QColor(100, 225, 255)); // light blue
    }

    return QStandardItemModel::data(index, role);
}

