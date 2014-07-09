///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// mainwindow.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#ifndef VAULT_MAINWINDOW_H
#define VAULT_MAINWINDOW_H

class QAction;
class QActionGroup;
class QMenu;
class QTabWidget;
class QLabel;
class QItemSelectionModel;
class QItemSelection;

class AccountModel;
class AccountView;

class KeychainModel;
class KeychainView;

class TxModel;
class TxView;

class TxActions;

class RequestPaymentDialog;

#include <CoinQ/CoinQ_netsync.h>

#include "paymentrequest.h"

#include <QMainWindow>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

    bool isLicenseAccepted() const { return licenseAccepted; }
    void setLicenseAccepted(bool licenseAccepted = true) { this->licenseAccepted = licenseAccepted; }

    void loadSettings();
    void saveSettings();
    void clearSettings();

    enum network_state_t {
        NETWORK_STATE_STOPPED,
        NETWORK_STATE_STARTED,
        NETWORK_STATE_SYNCHING,
        NETWORK_STATE_SYNCHED
    };

    void loadBlockTree();
    void tryConnect();
    bool isConnected() const { return networkState >= NETWORK_STATE_SYNCHING; }
    bool isSynched() const { return networkState == NETWORK_STATE_SYNCHED; }

    void updateStatusMessage(const QString& message);

signals:
    void status(const QString& message);
    void updateSyncHeight(int height);
    void updateBestHeight(int height);

    void signal_error(const QString& message);

    void signal_connectionOpen();
    void signal_connectionClosed();
    void signal_networkStarted();
    void signal_networkStopped();
    void signal_networkTimeout();
    void signal_networkDoneSync();

    void signal_addBestChain(const chain_header_t& header);
    void signal_removeBestChain(const chain_header_t& header);

    void unsignedTx();

protected:
    void closeEvent(QCloseEvent* event);
    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent* event);

protected slots:
    void updateSyncLabel();
    void updateNetworkState(network_state_t newState);
    void updateVaultStatus(const QString& name = QString());
    void showError(const QString& errorMsg);
    void showUpdate(const QString& updateMsg);

private slots:
    ///?/////?/////////
    // VAULT OPERATIONS
    void newVault(QString fileName = QString());
    void openVault(QString fileName = QString());
    void importVault(QString fileName = QString());
    void exportVault(QString fileName = QString(), bool exportPrivKeys = true);
    void closeVault();

    //////////////////////
    // KEYCHAIN OPERATIONS
    void newKeychain();
    void unlockKeychain();
    void lockKeychain();
    void lockAllKeychains();
    void importKeychain(QString fileName = QString());
    void exportKeychain(bool exportPrivate);
    void backupKeychain();
//    void mergeKeychains();
//    void removeKeychain();
//    void renameKeychain();
    void updateCurrentKeychain(const QModelIndex& current, const QModelIndex& previous);
    void updateSelectedKeychains(const QItemSelection& selected, const QItemSelection& deselected);

    // ACCOUNT OPERATIONS
    void quickNewAccount();
    void newAccount();
    void importAccount(QString fileName = QString());
    void exportAccount();
    void exportSharedAccount();
    void deleteAccount();
//    void renameAccount();
    void viewAccountHistory();
    void viewScripts();
    void requestPayment();
    void viewUnsignedTxs();
    void updateCurrentAccount(const QModelIndex& current, const QModelIndex& previous);
    void updateSelectedAccounts(const QItemSelection& selected, const QItemSelection& deselected);

    /////////////////////////
    // TRANSACTION OPERATIONS
    void insertRawTx();
//    void removeTx();
//    void requestTx();
    void createRawTx();
    void createTx(const PaymentRequest& paymentRequest = PaymentRequest());
    void signRawTx();
    //void newTx(const coin_tx_t& tx);
    void newTx(const bytes_t& hash);
    void sendRawTx();

    //////////////////////////////
    // BLOCK OPERATIONS AND EVENTS
    void syncBlocks();
    void fetchingHeaders();
    void headersSynched();
    void fetchingBlocks();
    void blocksSynched();
    void addBestChain(const chain_header_t& header);
    void removeBestChain(const chain_header_t& header);
    //void newBlock(const chain_block_t& block);
    void newBlock(const bytes_t& hash, int height);

    /////////////////////
    // NETWORK OPERATIONS
    void startNetworkSync();
    void stopNetworkSync();
    void promptSync();
    void connectionOpen();
    void connectionClosed();
    void networkStatus(const QString& status);
    void networkError(const QString& error);
    void networkStarted();
    void networkStopped();
    void networkTimeout();
    void networkSettings();

    ////////////////////////
    // ABOUT/HELP OPERATIONS
    void about();

    /////////////////
    // STATUS UPDATES
    void errorStatus(const QString& message);

    //////////////////////////////
    // URL/FILE/COMMAND OPERATIONS
    void processUrl(const QUrl& url);
    void processFile(const QString& fileName);
    void processCommand(const QString& command);

private:
    // License accepted?
    bool licenseAccepted;

    void createActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();

    bool maybeSave();

    void loadVault(const QString &fileName);

    void setCurrentFile(const QString &fileName);
    QString strippedName(const QString &fullFileName);

    QString curFile;

    //void updateBestHeight(int newHeight);

    // selects account with index i. returns true iff account with that index exists and was selected.
    bool selectAccount(int i);

    // network
    CoinQ::Network::NetworkSync networkSync;

    // menus
    QMenu* fileMenu;
    QMenu* keychainMenu;
    QMenu* accountMenu;
    QMenu* txMenu;
    QMenu* networkMenu;
    QMenu* helpMenu;

    // toolbars
    QToolBar* fileToolBar;
    QToolBar* keychainToolBar;
    QToolBar* accountToolBar;
    QToolBar* networkToolBar;
    QToolBar* editToolBar;
    QToolBar* txToolBar;

    // application actions
    QAction* quitAction;

    // vault actions
    QAction* newVaultAction;
    QAction* openVaultAction;
    QAction* importVaultAction;
    QAction* exportVaultAction;
    QAction* exportPublicVaultAction;
    QAction* closeVaultAction;

    // keychain actions
    QAction* newKeychainAction;
    QAction* unlockKeychainAction;
    QAction* lockKeychainAction;
    QAction* lockAllKeychainsAction;
    bool     importPrivate;
    QAction* importPrivateAction;
    QAction* importPublicAction;
    QActionGroup* importModeGroup;
    QAction* importKeychainAction;
    QAction* exportPrivateKeychainAction;
    QAction* exportPublicKeychainAction;
    QAction* backupKeychainAction;

    // account actions
    QAction* quickNewAccountAction;
    QAction* newAccountAction;
    QAction* importAccountAction;
    QAction* exportAccountAction;
    QAction* exportSharedAccountAction;
    QAction* deleteAccountAction;
    QAction* viewAccountHistoryAction;
    QAction* viewScriptsAction;
    QAction* requestPaymentAction;
    QAction* sendPaymentAction;
    QAction* viewUnsignedTxsAction;

    // transaction actions
    QAction* insertRawTxAction;
    QAction* createRawTxAction;
    QAction* createTxAction;
    QAction* signRawTxAction;
    QAction* sendRawTxAction;

    // network actions
    int syncHeight;
    int bestHeight;
    QLabel* syncLabel;
    QLabel* networkStateLabel;
    QString blockTreeFile;
    QString host;
    int port;
    bool autoConnect;
    QAction* connectAction;
    QAction* disconnectAction;
    QAction* networkSettingsAction;

    // network sync state
    network_state_t networkState;

    // network state icons
    QPixmap* stoppedIcon;
    QMovie* synchingMovie;
    QPixmap* synchedIcon;

    // about/help actions
    QAction* aboutAction;

    // tabs
    QTabWidget* tabWidget;

    AccountModel* accountModel;
    AccountView* accountView;

    KeychainModel* keychainModel;
    KeychainView* keychainView;

    TxModel *txModel;
    TxView *txView;

    // tab actions
    TxActions* txActions;

    // models
    QItemSelectionModel* keychainSelectionModel;
    QItemSelectionModel* accountSelectionModel;

    // dialogs
    RequestPaymentDialog* requestPaymentDialog;
};

#endif // VAULT_MAINWINDOW_H
