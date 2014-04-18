///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// txactions.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef VAULT_TXACTIONS_H
#define VAULT_TXACTIONS_H

#include <QObject>

class QAction;
class QMenu;

class QString;

class TxModel;
class TxView;
class AccountModel;

namespace CoinQ {
    namespace Network {
        class NetworkSync;
    }
}

class TxActions : public QObject
{
    Q_OBJECT

public:
    TxActions(TxModel* model, TxView* view, AccountModel* acctModel, CoinQ::Network::NetworkSync* sync = NULL); // model and view must be valid, non-null.

    void setNetworkSync(CoinQ::Network::NetworkSync* sync) { networkSync = sync; }

    QMenu* getMenu() const { return menu; }

    void updateVaultStatus();

signals:
    void error(const QString& message);

private slots:
    void updateCurrentTx(const QModelIndex& current, const QModelIndex& previous);
    void signTx();
    void sendTx();
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

    TxModel* txModel;
    TxView* txView;

    AccountModel* accountModel;

    CoinQ::Network::NetworkSync* networkSync;

    int currentRow;

    QAction* signTxAction;
    QAction* sendTxAction;
    QAction* viewRawTxAction;
    QAction* copyTxHashToClipboardAction;
    QAction* copyRawTxToClipboardAction;
    QAction* saveRawTxToFileAction;
    QAction* insertRawTxFromFileAction;
    QAction* viewTxOnWebAction;
    QAction* deleteTxAction;

    QMenu* menu;
};

#endif // VAULT_TXACTIONS_H
