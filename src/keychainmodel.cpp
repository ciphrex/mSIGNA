///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// keychainmodel.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include "keychainmodel.h"

#include <QStandardItemModel>

using namespace CoinQ::Vault;
using namespace std;

KeychainModel::KeychainModel()
    : vault(NULL)
{
    QStringList columns;
    columns << tr("Key Chain Name") << tr("Type") << tr("Keys") << tr("Hash");
    setHorizontalHeaderLabels(columns);
}

void KeychainModel::setVault(CoinQ::Vault::Vault* vault)
{
    this->vault = vault;
    update();
}

void KeychainModel::update()
{
    removeRows(0, rowCount());

    if (!vault) return;

    std::vector<KeychainInfo> keychains = vault->getKeychains();
    for (auto& keychain: keychains) {
        QList<QStandardItem*> row;
        row.append(new QStandardItem(QString::fromStdString(keychain.name())));

        bool isPrivate = (keychain.type() == Keychain::PRIVATE);
        QStandardItem* typeItem = new QStandardItem(
            isPrivate  ? tr("Private") : tr("Public"));
        typeItem->setData(isPrivate, Qt::UserRole);
        row.append(typeItem);
        
        row.append(new QStandardItem(QString::number(keychain.numkeys())));
        row.append(new QStandardItem(QString::fromStdString(uchar_vector(keychain.hash()).getHex())));
        appendRow(row);
    }    
}

void KeychainModel::exportKeychain(const QString& keychainName, const QString& fileName, bool exportPrivate) const
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    vault->exportKeychain(keychainName.toStdString(), fileName.toStdString(), exportPrivate);
}

void KeychainModel::importKeychain(const QString& keychainName, const QString& fileName, bool& importPrivate)
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    vault->importKeychain(keychainName.toStdString(), fileName.toStdString(), importPrivate);
    update();
}

bool KeychainModel::exists(const QString& keychainName) const
{
    QList<QStandardItem*> items = findItems(keychainName, Qt::MatchExactly, 0);
    return !items.isEmpty();
}
