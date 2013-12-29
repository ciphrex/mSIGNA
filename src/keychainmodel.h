///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// keychainmodel.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef COINVAULT_KEYCHAINMODEL_H
#define COINVAULT_KEYCHAINMODEL_H

#include <QStandardItemModel>

#include <CoinQ_vault.h>

class KeychainModel : public QStandardItemModel
{
    Q_OBJECT

public:
    KeychainModel();

    void setVault(CoinQ::Vault::Vault* vault);
    void update();

    void exportKeychain(const QString& keychainName, const QString& fileName, bool exportPrivate) const;
    void importKeychain(const QString& keychainName, const QString& fileName, bool& importPrivate);
    bool exists(const QString& keychainName) const;

private:
    CoinQ::Vault::Vault* vault;
};

#endif // COINVAULT_ACCOUNTMODEL_H
