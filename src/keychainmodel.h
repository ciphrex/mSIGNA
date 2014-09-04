///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// keychainmodel.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

#include <QStandardItemModel>

#include <CoinDB/Vault.h>

#include <CoinQ/CoinQ_typedefs.h>

class KeychainModel : public QStandardItemModel
{
    Q_OBJECT

public:
    KeychainModel();

    void setVault(CoinDB::Vault* vault);
    void update();

    void exportKeychain(const QString& keychainName, const QString& fileName, bool exportPrivate) const;
    QString importKeychain(const QString& fileName, bool& importPrivate);
    bool exists(const QString& keychainName) const;
    bool isPrivate(const QString& keychainName) const;
    bool isLocked(const QString& keychainName) const;
    bool isEncrypted(const QString& keychainName) const;
    void unlockKeychain(const QString& keychainName, const secure_bytes_t& unlockKey = secure_bytes_t());
    void lockKeychain(const QString& keychainName);
    void lockAllKeychains();
    void encryptKeychain(const QString& keychainName, const secure_bytes_t& lockKey = secure_bytes_t());
    void decryptKeychain(const QString& keychainName);

    QString getName(int row) const;

    enum Status { PUBLIC, UNLOCKED, LOCKED };
    int getStatus(int row) const;
    bool isEncrypted(int row) const;

    bytes_t getExtendedKeyBytes(const QString& keychainName, bool getPrivate = false, const bytes_t& decryptionKey = bytes_t()) const;

    // Overridden methods
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex& index) const;

signals:
    void error(const QString& message);
    void keychainChanged();

private:
    CoinDB::Vault* vault;
};

