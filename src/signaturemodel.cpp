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

#include <QStandardItemModel>

using namespace CoinDB;
using namespace std;

SignatureModel::SignatureModel(CoinDB::Vault* vault, const bytes_t& txHash, QObject* parent)
    : QStandardItemModel(parent), m_vault(vault), m_txHash(txHash)
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

    if (!m_vault) return;

    SignatureInfo signatureInfo = m_vault->getSignatureInfo(m_txHash);
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
