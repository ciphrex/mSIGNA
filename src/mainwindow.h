///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// mainwindow.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

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
class SignatureActions;

class RequestPaymentDialog;

#include <CoinDB/SynchedVault.h>
//#include <CoinQ/CoinQ_netsync.h>

#include "paymentrequest.h"

#include <QMainWindow>

#include <vector>

enum fontsize_t { SMALL_FONTS , MEDIUM_FONTS , LARGE_FONTS };

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

    void loadHeaders();
    void tryConnect();
    bool isConnected() const { return networkState >= NETWORK_STATE_STARTED; }
    bool isSynched() const { return networkState == NETWORK_STATE_SYNCHED; }

    void updateStatusMessage(const QString& message);

signals:
    void status(const QString& message);
    void headersLoadProgress(const QString& message);
    void updateSyncHeight(int height);
    void updateBestHeight(int height);

    void signal_error(const QString& message);

    void vaultOpened(CoinDB::Vault* vault);
    void vaultClosed();

    void signal_connectionOpen();
    void signal_connectionClosed();
    void signal_networkStarted();
    void signal_networkStopped();
    void signal_networkTimeout();
    void signal_networkDoneSync();

    void signal_newTx();
    void signal_newBlock();
    void signal_refreshAccounts();

    void signal_addBestChain(const chain_header_t& header);
    void signal_removeBestChain(const chain_header_t& header);

    void signal_currencyUnitChanged();

#ifdef SUPPORT_OLD_ADDRESS_VERSIONS
    void signal_addressVersionsChanged();
#endif


    void unsignedTx();

public slots:
    //////////////////////////////
    // URL/FILE/COMMAND OPERATIONS
    void processUrl(const QUrl& url);
    void processFile(const QString& fileName);
    void processCommand(const QString& command, const std::vector<QString>& args);

protected:
    void closeEvent(QCloseEvent* event);
    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent* event);

protected slots:
    void updateFonts(int fontSize);
    void updateSyncLabel();
    void updateNetworkState(network_state_t newState);
    void updateVaultStatus(const QString& name = QString());
    void showError(const QString& errorMsg);
    void showUpdate(const QString& updateMsg);

private slots:
    ////////////////////
    // GLOBAL OPERATIONS
    void selectCurrencyUnit();
    void selectCurrencyUnit(const QString& newCurrencyUnitPrefix);
    void selectTrailingDecimals(bool newShowTrailingDecimals);
#ifdef SUPPORT_OLD_ADDRESS_VERSIONS
    void selectAddressVersions(bool useOld);
#endif

    ///////////////////
    // VAULT OPERATIONS
    void newVault(QString fileName = QString());
    void openVault(QString fileName = QString());
    void importVault(QString fileName = QString());
    void exportVault(QString fileName = QString(), bool exportPrivKeys = true);
    void closeVault();

    //////////////////////
    // KEYCHAIN OPERATIONS
    void newKeychain();
    bool unlockKeychain(QString name = QString());
    void lockKeychain(QString name = QString());
    void lockAllKeychains();
    int  setKeychainPassphrase(const QString& keychainName = QString());
    int  makeKeychainBackup(const QString& keychainName = QString());
    void importKeychain(QString fileName = QString());
    void exportKeychain(bool exportPrivate);
    void importBIP32();
    void viewBIP32(bool viewPrivate);
    void importBIP39();
    void viewBIP39();
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
    void refreshAccounts();

    /////////////////////////
    // TRANSACTION OPERATIONS
    void insertRawTx();
//    void removeTx();
//    void requestTx();
    void createRawTx();
    void createTx(const PaymentRequest& paymentRequest = PaymentRequest());
    void signRawTx();
    void newTx();
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
    void newBlock();

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

private:
    int fontSize;

    // License accepted?
    bool licenseAccepted;

    void createActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();

    const int MAX_RECENTS = 4; 
    QList<QString> recents;
    void addToRecents(const QString& filename);
    void loadRecents();
    void saveRecents();
    void updateRecentsMenu();

    bool maybeSave();

    void loadVault(const QString &fileName);

    void setCurrentFile(const QString &fileName);
    QString strippedName(const QString &fullFileName);

    QString curFile;
    QString currencyUnitPrefix;
    bool showTrailingDecimals;

    //void updateBestHeight(int newHeight);

    // selects account with index i. returns true iff account with that index exists and was selected.
    bool selectAccount(int i);
    bool selectAccount(const QString& account);
    QString selectedAccount;

    CoinDB::SynchedVault synchedVault;

    // network
    //CoinQ::Network::NetworkSync networkSync;

    // menus
    QMenu* fileMenu;
    QMenu* recentsMenu;
    QMenu* keychainMenu;
    QMenu* accountMenu;
    QMenu* txMenu;
    QMenu* networkMenu;
    QMenu* fontsMenu;
    QMenu* currencyUnitMenu;
#ifdef SUPPORT_OLD_ADDRESS_VERSIONS
    QMenu* addressVersionsMenu;
#endif
    QMenu* helpMenu;

    // toolbars
    QToolBar* fileToolBar;
    QToolBar* keychainToolBar;
    QToolBar* accountToolBar;
    QToolBar* networkToolBar;
    QToolBar* editToolBar;
    QToolBar* txToolBar;

    // application actions
    QAction* selectCurrencyUnitAction;
    QAction* quitAction;
    bool bQuitting;

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
    QAction* setKeychainPassphraseAction;
    QAction* makeKeychainBackupAction;
    bool     importPrivate;
    QAction* importPrivateAction;
    QAction* importPublicAction;
    QActionGroup* importModeGroup;
    QAction* importKeychainAction;
    QAction* exportPrivateKeychainAction;
    QAction* exportPublicKeychainAction;
    QAction* importBIP32Action;
    QAction* viewPrivateBIP32Action;
    QAction* viewPublicBIP32Action;
    QAction* importBIP39Action;
    QAction* viewBIP39Action;

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
    QAction* shortConnectAction;
    QAction* disconnectAction;
    QAction* shortDisconnectAction;
    QAction* networkSettingsAction;

    // network sync state
    network_state_t networkState;

    // network state icons
    QPixmap* stoppedIcon;
    QMovie* synchingMovie;
    QPixmap* synchedIcon;

    // font actions
    QActionGroup* fontSizeGroup;
    QAction* smallFontsAction;
    QAction* mediumFontsAction;
    QAction* largeFontsAction;

    // currency unit actions
    QActionGroup* currencyUnitGroup;
    QList<QAction*> currencyUnitActions;
    QAction* showTrailingDecimalsAction;

#ifdef SUPPORT_OLD_ADDRESS_VERSIONS
    // address version actions
    bool useOldAddressVersions;
    QAction* useOldAddressVersionsAction;
#endif

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

    // categorized actions
    TxActions* txActions;
    SignatureActions* signatureActions;

    // models
    QItemSelectionModel* keychainSelectionModel;
    QItemSelectionModel* accountSelectionModel;

    // dialogs
    RequestPaymentDialog* requestPaymentDialog;
};

