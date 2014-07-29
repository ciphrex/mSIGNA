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

using namespace CoinDB;
using namespace std;

KeychainModel::KeychainModel()
    : vault(NULL)
{
    QStringList columns;
    columns << tr("Keychain") << tr("Type") << tr("Unlocked") << tr("Hash");
    setHorizontalHeaderLabels(columns);
}

void KeychainModel::setVault(CoinDB::Vault* vault)
{
    this->vault = vault;
}

void KeychainModel::update()
{
    removeRows(0, rowCount());

    if (!vault) return;

    std::vector<KeychainView> keychains = vault->getRootKeychainViews();
    for (auto& keychain: keychains)
    {
        QList<QStandardItem*> row;
        row.append(new QStandardItem(QString::fromStdString(keychain.name)));

        QStandardItem* typeItem = new QStandardItem(
            keychain.is_private  ? tr("Private") : tr("Public"));
        typeItem->setData(((int)keychain.is_private << 1) | (int)keychain.is_locked, Qt::UserRole);
        row.append(typeItem);

        QStandardItem* lockedItem = new QStandardItem(
            keychain.is_private ? (keychain.is_locked ? tr("No") : tr("Yes")) : tr(""));
        row.append(lockedItem);
        
        row.append(new QStandardItem(QString::fromStdString(uchar_vector(keychain.hash).getHex())));
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

void KeychainModel::importKeychain(const QString& /*keychainName*/, const QString& fileName, bool& importPrivate)
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    vault->importKeychain(fileName.toStdString(), importPrivate);
    update();
}

void KeychainModel::unlockKeychain(const QString& keychainName, const secure_bytes_t& unlockKey)
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    vault->unlockKeychain(keychainName.toStdString(), unlockKey);
    update();
}

void KeychainModel::lockKeychain(const QString& keychainName)
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    vault->lockKeychain(keychainName.toStdString());
    update();
}

void KeychainModel::lockAllKeychains()
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    vault->lockAllKeychains();
    update();
}

bool KeychainModel::exists(const QString& keychainName) const
{
    QList<QStandardItem*> items = findItems(keychainName, Qt::MatchExactly, 0);
    return !items.isEmpty();
}

bool KeychainModel::isPrivate(const QString& keychainName) const
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    std::shared_ptr<Keychain> keychain = vault->getKeychain(keychainName.toStdString());
    return keychain->isPrivate();
}

bool KeychainModel::isEncrypted(const QString& keychainName) const
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    std::shared_ptr<Keychain> keychain = vault->getKeychain(keychainName.toStdString());
    return keychain->isEncrypted();
}

bytes_t KeychainModel::getExtendedKeyBytes(const QString& keychainName, bool getPrivate, const bytes_t& /*decryptionKey*/) const
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    return vault->getKeychainExtendedKey(keychainName.toStdString(), getPrivate);
}

QVariant KeychainModel::data(const QModelIndex& index, int role) const
{
/*
    if (role == Qt::TextAlignmentRole && index.column() == 1) {
        // Right-align numeric fields
        return Qt::AlignRight;
    }
*/

    return QStandardItemModel::data(index, role);
}

bool KeychainModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (role == Qt::EditRole) {
        if (index.column() == 0) {
            // Keychain name edited.
            if (!vault) return false;

            try {
                vault->renameKeychain(index.data().toString().toStdString(), value.toString().toStdString());
                setItem(index.row(), index.column(), new QStandardItem(value.toString()));
                return true;
            }
            catch (const std::exception& e) {
                emit error(QString::fromStdString(e.what()));
            }
        }
        return false;
    }

    return true;
}

Qt::ItemFlags KeychainModel::flags(const QModelIndex& index) const
{
    if (index.column() == 0) {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
    }

    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}
