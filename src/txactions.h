///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// txactions.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

#include <QObject>

class QAction;
class QMenu;

class QString;

class TxModel;
class TxView;
class AccountModel;

namespace CoinDB
{
    class SynchedVault;
}

class TxActions : public QObject
{
    Q_OBJECT

public:
    TxActions(TxModel* txModel, TxView* txView, AccountModel* accountModel, CoinDB::SynchedVault* synchedVault = nullptr); // model and view must be valid, non-null.

    void setNetworkSync(CoinDB::SynchedVault* synchedVault) { m_synchedVault = synchedVault; }

    QMenu* getMenu() const { return menu; }

    void updateVaultStatus();

signals:
    void error(const QString& message);

private slots:
    void updateCurrentTx(const QModelIndex& current, const QModelIndex& previous);
    void showSignatureDialog();
    void signTx();
    void sendTx();
    void exportTxToFile();
    void importTxFromFile();
    void viewRawTx();
    void copyTxHashToClipboard();
    void copyRawTxToClipboard();
    void saveRawTxToFile();
    void insertRawTxFromFile();
    void viewTxOnWeb();
    void deleteTx();

private:
    void createActions();
    void createMenus();

    TxModel* m_txModel;
    TxView* m_txView;

    AccountModel* m_accountModel;

    CoinDB::SynchedVault* m_synchedVault;

    int currentRow;

    QAction* signaturesAction;
    QAction* signTxAction;
    QAction* sendTxAction;
    QAction* exportTxToFileAction;
    QAction* importTxFromFileAction;
    QAction* viewRawTxAction;
    QAction* copyTxHashToClipboardAction;
    QAction* copyRawTxToClipboardAction;
    QAction* saveRawTxToFileAction;
    QAction* insertRawTxFromFileAction;
    QAction* viewTxOnWebAction;
    QAction* deleteTxAction;

    QMenu* menu;
};

