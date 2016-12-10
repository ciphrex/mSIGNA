///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// keychainmodel.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "keychainmodel.h"

#include <QStandardItemModel>

using namespace CoinDB;
using namespace std;

KeychainModel::KeychainModel()
    : vault(NULL)
{
    QStringList columns;
    columns << tr("Keychain") << tr("") << tr("") << tr("Hash");
    setHorizontalHeaderLabels(columns);

    connect(this, &KeychainModel::dataChanged, [this]() { emit keychainChanged(); });
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

        QStandardItem* encryptedItem = new QStandardItem();
        if (keychain.is_encrypted)
        {
            encryptedItem->setIcon(QIcon(":/icons/encrypted.png"));
            encryptedItem->setData(true, Qt::UserRole);
        }
        else
        {
            encryptedItem->setData(false, Qt::UserRole);
        }
        row.append(encryptedItem);
        
        QStandardItem* statusItem = new QStandardItem();
        if (!keychain.is_private)
        {
            statusItem->setIcon(QIcon(":/icons/shared.png"));
            statusItem->setData(PUBLIC, Qt::UserRole);
        }
        else if (keychain.is_locked)
        {
            statusItem->setIcon(QIcon(":/icons/locked.png"));
            statusItem->setData(LOCKED, Qt::UserRole);
        }
        else
        {
            statusItem->setIcon(QIcon(":/icons/unlocked.png"));
            statusItem->setData(UNLOCKED, Qt::UserRole);
        }
        row.append(statusItem);

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

QString KeychainModel::importKeychain(const QString& fileName, bool& importPrivate)
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    std::shared_ptr<Keychain> keychain = vault->importKeychain(fileName.toStdString(), importPrivate);
    update();
    return QString::fromStdString(keychain->name());
}

bool KeychainModel::unlockKeychain(const QString& keychainName, const secure_bytes_t& unlockKey)
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    try
    {
        vault->unlockKeychain(keychainName.toStdString(), unlockKey);
        update();
        return true;
    }
    catch (const CoinDB::KeychainPrivateKeyUnlockFailedException& e)
    {
        emit error(tr("Keychain decryption failed."));
    }
    catch (const std::exception& e)
    {
        emit error(e.what());
    } 

    return false;
}

void KeychainModel::lockKeychain(const QString& keychainName)
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    try
    {
        vault->lockKeychain(keychainName.toStdString());
        update();
    }
    catch (const std::exception& e)
    {
        emit error(e.what());
    }
}

void KeychainModel::lockAllKeychains()
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    try
    {
        vault->lockAllKeychains();
        update();
    }
    catch (const std::exception& e)
    {
        emit error(e.what());
    }
}

void KeychainModel::encryptKeychain(const QString& keychainName, const secure_bytes_t& lockKey)
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    try
    {
        vault->encryptKeychain(keychainName.toStdString(), lockKey);
        update();
    }
    catch (const std::exception& e)
    {
        emit error(e.what());
    }
}

void KeychainModel::decryptKeychain(const QString& keychainName)
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    try
    {
        vault->decryptKeychain(keychainName.toStdString());
        update();
    }
    catch (const std::exception& e)
    {
        emit error(e.what());
    }
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

bool KeychainModel::isLocked(const QString& keychainName) const
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    std::shared_ptr<Keychain> keychain = vault->getKeychain(keychainName.toStdString());
    return keychain->isLocked();
}

bool KeychainModel::isEncrypted(const QString& keychainName) const
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    std::shared_ptr<Keychain> keychain = vault->getKeychain(keychainName.toStdString());
    return keychain->isEncrypted();
}

bool KeychainModel::hasSeed(const QString& keychainName) const
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    std::shared_ptr<Keychain> keychain = vault->getKeychain(keychainName.toStdString());
    return keychain->hasSeed();
}

QString KeychainModel::getName(int row) const
{
    if (row < 0 || row >= rowCount()) throw std::runtime_error("Invalid row.");

    return item(row, 0)->text();
}

int KeychainModel::getStatus(int row) const
{
    if (row < 0 || row >= rowCount()) throw std::runtime_error("Invalid row.");

    return item(row, 2)->data(Qt::UserRole).toInt();
}

bool KeychainModel::isEncrypted(int row) const
{
    if (row < 0 || row >= rowCount()) throw std::runtime_error("Invalid row.");

    return item(row, 1)->data(Qt::UserRole).toBool();
}

bytes_t KeychainModel::getExtendedKeyBytes(const QString& keychainName, bool getPrivate, const bytes_t& /*decryptionKey*/) const
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    return vault->exportBIP32(keychainName.toStdString(), getPrivate);
}

QVariant KeychainModel::data(const QModelIndex& index, int role) const
{
/*
    if (role == Qt::TextAlignmentRole && index.column() == 1) {
        // Right-align numeric fields
        return Qt::AlignRight;
    }
*/
    if (role == Qt::FontRole)
    {
        QFont font;
        if (index.column() == 0)
        {
            font.setBold(true);
            return font;
        }
    }

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
