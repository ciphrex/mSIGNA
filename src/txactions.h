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

class QWidget;
class QAction;
class QMenu;

class QString;

class TxModel;
class TxView;
class AccountModel;
class KeychainModel;

namespace CoinDB
{
    class SynchedVault;
}

class TxActions : public QObject
{
    Q_OBJECT

public:
    TxActions(TxModel* txModel, TxView* txView, AccountModel* accountModel, KeychainModel* keychainModel, CoinDB::SynchedVault* synchedVault = nullptr, QWidget* parent = nullptr); // model and view must be valid, non-null.

    void setNetworkSync(CoinDB::SynchedVault* synchedVault) { m_synchedVault = synchedVault; }

    QMenu* getMenu() const { return menu; }

    void updateVaultStatus();

signals:
    void error(const QString& message);
    void setCurrentWidget(QWidget* widget);
    void txsChanged();

private slots:
    void updateCurrentTx(const QModelIndex& current, const QModelIndex& previous);
    void searchTx();
    void showSignatureDialog();
    void signTx();
    void sendTx();
    void exportTxToFile();
    void importTxFromFile();
    void exportAllTxsToFile();
    void importTxsFromFile();
    void viewRawTx();
    void copyAddressToClipboard();
    void copyTxHashToClipboard();
    void copyRawTxToClipboard();
    void insertRawTxFromClipboard();
    void saveRawTxToFile();
    void insertRawTxFromFile();
    void viewTxOnWeb();
    void deleteTx();

private:
    QWidget* m_parent;

    void createActions();
    void createMenus();

    TxModel* m_txModel;
    TxView* m_txView;

    AccountModel* m_accountModel;

    KeychainModel* m_keychainModel;

    CoinDB::SynchedVault* m_synchedVault;

    int currentRow;

    QAction* searchTxAction;
    QAction* signaturesAction;
    QAction* signTxAction;
    QAction* sendTxAction;
    QAction* exportTxToFileAction;
    QAction* importTxFromFileAction;
    QAction* exportAllTxsToFileAction;
    QAction* importTxsFromFileAction;
    QAction* viewRawTxAction;
    QAction* copyAddressToClipboardAction;
    QAction* copyTxHashToClipboardAction;
    QAction* copyRawTxToClipboardAction;
    QAction* insertRawTxFromClipboardAction;
    QAction* saveRawTxToFileAction;
    QAction* insertRawTxFromFileAction;
    QAction* viewTxOnWebAction;
    QAction* deleteTxAction;

    QMenu* menu;
};

