///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// signatureactions.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include <QObject>

class QAction;
class QMenu;

class QString;

class SignatureDialog;

namespace CoinDB
{
    class SynchedVault;
}

class SignatureActions : public QObject
{
    Q_OBJECT

public:
    SignatureActions(CoinDB::SynchedVault& synchedVault, SignatureDialog& dialog);
    ~SignatureActions();

    QMenu* getMenu() const { return menu; }

//    void updateVaultStatus();

signals:
    void error(const QString& message);

private slots:
    void updateCurrentKeychain(const QModelIndex& current, const QModelIndex& previous);
    void addSignature();
    bool unlockKeychain(bool bUpdateKeychains = true);
    void lockKeychain();

private:
    void createActions();
    void createMenus();

    void refreshCurrentKeychain();

    CoinDB::SynchedVault& m_synchedVault;

    SignatureDialog& m_dialog;

    int m_currentRow;
    QString m_currentKeychain;

    QAction* addSignatureAction;
    QAction* unlockKeychainAction;
    QAction* lockKeychainAction;

    QMenu* menu;
};

