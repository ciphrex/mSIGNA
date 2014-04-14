///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// mainwindow.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include <QtWidgets>
#include <QDir>
#include <QTreeWidgetItem>
#include <QTabWidget>
#include <QInputDialog>
#include <QItemSelectionModel>

#include "settings.h"
#include "versioninfo.h"
#include "copyrightinfo.h"

#include "mainwindow.h"

// Random
#include <random.h>

// Coin scripts
#include <CoinQ_script.h>

// Network
#include <CoinQ_netsync.h>

// Models/Views
#include "accountmodel.h"
#include "accountview.h"
#include "keychainmodel.h"
#include "keychainview.h"
#include "txmodel.h"
#include "txview.h"

// Actions
#include "txactions.h"

// Dialogs
#include "newkeychaindialog.h"
#include "quicknewaccountdialog.h"
#include "newaccountdialog.h"
#include "rawtxdialog.h"
#include "createtxdialog.h"
#include "accounthistorydialog.h"
#include "scriptdialog.h"
#include "requestpaymentdialog.h"
#include "networksettingsdialog.h"
#include "keychainbackupdialog.h"
#include "passphrasedialog.h"
#include "resyncdialog.h"

// Logging
#include "severitylogger.h"

// Coin Parameters
#include "coinparams.h"

boost::mutex repaintMutex;

using namespace CoinQ::Script;
using namespace std;

MainWindow::MainWindow() :
    licenseAccepted(false),
    networkSync(getCoinParams()),
    syncHeight(0),
    bestHeight(0),
    connected(false),
    doneHeaderSync(false),
    networkState(NETWORK_STATE_NOT_CONNECTED),
    accountModel(nullptr),
    keychainModel(nullptr)
{
    loadSettings();

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();

    //setCurrentFile("");
    setUnifiedTitleAndToolBarOnMac(true);

    // Keychain tab page
    keychainModel = new KeychainModel();
    keychainView = new KeychainView();
    keychainView->setModel(keychainModel);
    keychainView->setSelectionMode(KeychainView::MultiSelection);
    keychainView->setMenu(keychainMenu);

    keychainSelectionModel = keychainView->selectionModel();
    connect(keychainSelectionModel, &QItemSelectionModel::currentChanged,
            this, &MainWindow::updateCurrentKeychain);
    connect(keychainSelectionModel, &QItemSelectionModel::selectionChanged,
            this, &MainWindow::updateSelectedKeychains);

    // Account tab page
    accountModel = new AccountModel();
    accountView = new AccountView();
    accountView->setModel(accountModel);
    accountView->setMenu(accountMenu);

    networkSync.subscribeTx([&](const Coin::Transaction& tx) {
        try {
            accountModel->insertTx(tx);
        }
        catch (const std::exception& e) {
            // TODO
        }
    });
//    networkSync.subscribeBlock([&](const ChainBlock& block) { accountModel->insertBlock(block); });
    networkSync.subscribeMerkleBlock([&](const ChainMerkleBlock& merkleBlock) { accountModel->insertMerkleBlock(merkleBlock); });
    networkSync.subscribeBlockTreeChanged([&]() { doneHeaderSync = false; emit updateBestHeight(networkSync.getBestHeight()); });

    qRegisterMetaType<bytes_t>("bytes_t");
    connect(accountModel, SIGNAL(newTx(const bytes_t&)), this, SLOT(newTx(const bytes_t&)));
    connect(accountModel, SIGNAL(newBlock(const bytes_t&, int)), this, SLOT(newBlock(const bytes_t&, int)));
    connect(accountModel, &AccountModel::updateSyncHeight, [this](int height) { emit updateSyncHeight(height); });
    connect(accountModel, SIGNAL(error(const QString&)), this, SLOT(showError(const QString&)));

    accountSelectionModel = accountView->selectionModel();
    connect(accountSelectionModel, &QItemSelectionModel::currentChanged,
            this, &MainWindow::updateCurrentAccount);
    connect(accountSelectionModel, &QItemSelectionModel::selectionChanged,
            this, &MainWindow::updateSelectedAccounts);

    connect(keychainModel, SIGNAL(error(const QString&)), this, SLOT(showError(const QString&)));

    // Transaction tab page
    txModel = new TxModel();
    connect(txModel, SIGNAL(txSigned(const QString&)), this, SLOT(showUpdate(const QString&)));

    txView = new TxView();
    txView->setModel(txModel);
    txActions = new TxActions(txModel, txView, &networkSync);
    connect(txActions, SIGNAL(error(const QString&)), this, SLOT(showError(const QString&)));

    txView->setMenu(txActions->getMenu());

    tabWidget = new QTabWidget();
    tabWidget->addTab(keychainView, tr("Keychains"));
    tabWidget->addTab(accountView, tr("Accounts"));
    tabWidget->addTab(txView, tr("Transactions"));
    setCentralWidget(tabWidget);

    requestPaymentDialog = new RequestPaymentDialog(accountModel, this);

    // status updates
    connect(this, &MainWindow::status, [this](const QString& message) { updateStatusMessage(message); });
    connect(this, &MainWindow::updateSyncHeight, [this](int height) {
        LOGGER(debug) << "MainWindow::updateBestHeight emitted. New best height: " << height << std::endl;
        syncHeight = height;
        updateSyncLabel();
        updateNetworkState();
    });
    connect(this, &MainWindow::updateBestHeight, [this](int height) {
        LOGGER(debug) << "MainWindow::updateBestHeight emitted. New best height: " << height << std::endl;
        bestHeight = height;
        updateSyncLabel();
        updateNetworkState();
    });

    setAcceptDrops(true);
}

void MainWindow::loadBlockTree()
{
    networkSync.initBlockTree(blockTreeFile.toStdString(), false);
    emit updateBestHeight(networkSync.getBestHeight());
}

void MainWindow::tryConnect()
{
    if (!autoConnect) return;
    startNetworkSync();
}

void MainWindow::updateStatusMessage(const QString& message)
{
    LOGGER(debug) << "MainWindow::updateStatusMessage - " << message.toStdString() << std::endl;
//    boost::lock_guard<boost::mutex> lock(repaintMutex);
//    statusBar()->showMessage(message);
}

void MainWindow::closeEvent(QCloseEvent * /*event*/)
{
    networkSync.stop();
    saveSettings();
/*
    if (maybeSave()) {
        writeSettings();
        event->accept();
    } else {
        event->ignore();
    }
*/
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    //if (event->mimeData()->hasFormat("text/plain"))
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event)
{
    const QMimeData* mimeData = event->mimeData();
    if (mimeData->hasUrls()) {
        for (auto& url: mimeData->urls()) {
            if (url.isLocalFile()) {
                processFile(url.toLocalFile());
            }
        }
        return;
    }

    QMessageBox::information(this, tr("Drop Event"), mimeData->text());
}

void MainWindow::updateSyncLabel()
{
    syncLabel->setText(QString::number(syncHeight) + "/" + QString::number(bestHeight));
}

void MainWindow::updateNetworkState(network_state_t newState)
{
    if (newState == NETWORK_STATE_UNKNOWN) {
        if (!connected) {
            newState = NETWORK_STATE_NOT_CONNECTED;
        }
        else if (syncHeight != bestHeight && (!doneHeaderSync || accountModel->getNumAccounts() > 0)) {
            newState = NETWORK_STATE_SYNCHING;
        }
        else {
            doneHeaderSync = true;
            newState = NETWORK_STATE_SYNCHED;
        }
    }

    if (newState != networkState) {
        networkState = newState;
        switch (networkState) {
        case NETWORK_STATE_NOT_CONNECTED:
            networkStateLabel->setPixmap(*notConnectedIcon);
            break;
        case NETWORK_STATE_SYNCHING:
            networkStateLabel->setMovie(synchingMovie);
            break;
        case NETWORK_STATE_SYNCHED:
            networkStateLabel->setPixmap(*synchedIcon);
            break;
        default:
            // We should never get here
            ;
        }
    }
}

void MainWindow::updateVaultStatus(const QString& name)
{
    bool isOpen = !name.isEmpty();

    // vault actions
    closeVaultAction->setEnabled(isOpen);

    // keychain actions
    newKeychainAction->setEnabled(isOpen);
    lockAllKeychainsAction->setEnabled(keychainModel && keychainModel->rowCount());
    importKeychainAction->setEnabled(isOpen);

    // account actions
    quickNewAccountAction->setEnabled(isOpen);
    newAccountAction->setEnabled(isOpen);
    importAccountAction->setEnabled(isOpen);

    // transaction actions
    insertRawTxAction->setEnabled(isOpen);
    createRawTxAction->setEnabled(isOpen);
    //createTxAction->setEnabled(isOpen);
    signRawTxAction->setEnabled(isOpen);

    if (isOpen) {
        setWindowTitle(getDefaultSettings().getAppName() + " - " + name);
    }
    else {
        setWindowTitle(getDefaultSettings().getAppName());
    }
}

void MainWindow::showError(const QString& errorMsg)
{
    updateStatusMessage(tr("Operation failed"));
    QMessageBox::critical(this, tr("Error"), errorMsg);
}

void MainWindow::showUpdate(const QString& updateMsg)
{
    QMessageBox::information(this, tr("Update"), updateMsg);
}

void MainWindow::newVault(QString fileName)
{
    if (fileName.isEmpty()) {
        fileName = QFileDialog::getSaveFileName(
            this,
            tr("Create New Vault"),
            lastVaultDir,
            tr("Vaults (*.vault)"));
    }
    if (fileName.isEmpty()) return;

    QFileInfo fileInfo(fileName);
    lastVaultDir = fileInfo.dir().absolutePath();
    saveSettings();

    try {
        accountModel->create(fileName);
        accountView->update();

        keychainModel->setVault(accountModel->getVault());
        keychainModel->update();
        keychainView->update();

        txModel->setVault(accountModel->getVault());
        txModel->update();
        txView->update();

        updateVaultStatus(fileName);

        // TODO: prompt user to unlock chain codes.
        accountModel->getVault()->unlockChainCodes(uchar_vector("1234"));
    }
    catch (const exception& e) {
        LOGGER(debug) << "MainWindow::newVault - " << e.what() << std::endl;
        showError(e.what());
    }
}

void MainWindow::promptResync()
{
    if (!connected) {
        QMessageBox msgBox;
        msgBox.setText(tr("You are not connected to network."));
        msgBox.setInformativeText(tr("Would you like to connect to network to synchronize your accounts?"));
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Ok);
        if (msgBox.exec() == QMessageBox::Ok) {
            startNetworkSync();
        }
    }
    else {
        resync();
    }
}

void MainWindow::openVault(QString fileName)
{
    if (fileName.isEmpty()) {
        fileName = QFileDialog::getOpenFileName(
            this,
            tr("Open Vault"),
            lastVaultDir,
            tr("Vaults (*.vault)"));
    }
    if (fileName.isEmpty()) return;

    fileName = QFileInfo(fileName).absoluteFilePath();

    QFileInfo fileInfo(fileName);
    lastVaultDir = fileInfo.dir().absolutePath();
    saveSettings();

    try {
        loadVault(fileName);
        updateVaultStatus(fileName);
        selectAccount(0);
        updateStatusMessage(tr("Opened ") + fileName);

        promptResync();
    }
    catch (const exception& e) {
        LOGGER(debug) << "MainWindow::openVault - " << e.what() << std::endl;
        showError(e.what());
    }
}

void MainWindow::closeVault()
{
    try {
        networkSync.stopResync();

        accountModel->close();
        keychainModel->setVault(NULL);
        txModel->setVault(NULL);
        txView->update();

        updateVaultStatus();
        updateStatusMessage(tr("Closed vault"));
    }
    catch (const exception& e) {
        LOGGER(debug) << "MainWindow::closeVault - " << e.what() << std::endl;
        showError(e.what());
    }
}

void MainWindow::newKeychain()
{
    try {
        NewKeychainDialog dlg(this);
        if (dlg.exec()) {
            QString name = dlg.getName();
            //unsigned long numKeys = dlg.getNumKeys();
            //updateStatusMessage(tr("Generating ") + QString::number(numKeys) + tr(" keys..."));

            // TODO: Randomize using user input for entropy
            accountModel->newKeychain(name, random_bytes(32));
            accountModel->update();
            keychainModel->update();
            keychainView->update();
            tabWidget->setCurrentWidget(keychainView);
            updateStatusMessage(tr("Created keychain ") + name);
        }
    }
    catch (const exception& e) {
        LOGGER(debug) << "MainWindow::newKeyChain - " << e.what() << std::endl;
        showError(e.what());
    }    
}

void MainWindow::unlockKeychain()
{
    QModelIndex index = keychainSelectionModel->currentIndex();
    int row = index.row();
    if (row < 0)
    {
        showError(tr("No keychain is selected."));
        return;
    }

    QStandardItem* nameItem = keychainModel->item(row, 0);
    QString name = nameItem->data(Qt::DisplayRole).toString();
    secure_bytes_t hash;
    if (keychainModel->isEncrypted(name))
    {
        PassphraseDialog dlg(tr("Enter unlock passphrase for ") + name + tr(":"));
        if (!dlg.exec()) return;

        // TODO: proper hash
        hash = sha256_2(dlg.getPassphrase().toStdString());
    }

    try
    {
        keychainModel->unlockKeychain(name, hash);
    }
    catch (const CoinDB::KeychainPrivateKeyUnlockFailedException& e)
    {
        throw std::runtime_error(tr("Invalid passphrase.").toStdString());
    }
}

void MainWindow::lockKeychain()
{
    QModelIndex index = keychainSelectionModel->currentIndex();
    int row = index.row();
    if (row < 0)
    {
        showError(tr("No keychain is selected."));
        return;
    }

    QStandardItem* nameItem = keychainModel->item(row, 0);
    QString name = nameItem->data(Qt::DisplayRole).toString();

    keychainModel->lockKeychain(name);
}

void MainWindow::lockAllKeychains()
{
    keychainModel->lockAllKeychains();
}

void MainWindow::importKeychain(QString fileName)
{
    if (fileName.isEmpty()) {
        fileName = QFileDialog::getOpenFileName(
            this,
            tr("Import Keychain"),
            lastVaultDir,
            tr("Keychains") + "(*.keys)");
    }
    if (fileName.isEmpty()) return;

    QFileInfo fileInfo(fileName);
    lastVaultDir = fileInfo.dir().absolutePath();
    saveSettings();

    bool ok;
    QString name = QFileInfo(fileName).baseName();
    while (true) {
        name = QInputDialog::getText(this, tr("Keychain Import"), tr("Keychain Name:"), QLineEdit::Normal, name, &ok);

        if (!ok) return;
        if (name.isEmpty()) {
            showError(tr("Name cannot be empty."));
            continue;
        }
        if (keychainModel->exists(name)) {
            showError(tr("There is already a keychain with that name."));
            continue;
        }
        break;
    }

    try {
        updateStatusMessage(tr("Importing keychain..."));
        bool isPrivate = importPrivate; // importPrivate is a user setting. isPrivate is whether or not this particular keychain is private.
        keychainModel->importKeychain(name, fileName, isPrivate);
        keychainView->update();
        tabWidget->setCurrentWidget(keychainView);
        updateStatusMessage(tr("Imported ") + (isPrivate ? tr("private") : tr("public")) + tr(" keychain ") + name);
    }
    catch (const exception& e) {
        LOGGER(debug) << "MainWindow::importKeychain - " << e.what() << std::endl;
        showError(e.what());
    }
}

void MainWindow::exportKeychain(bool exportPrivate)
{
    QModelIndex index = keychainSelectionModel->currentIndex();
    int row = index.row();
    if (row < 0) {
        showError(tr("No keychain is selected."));
        return;
    }

    QStandardItem* typeItem = keychainModel->item(row, 1);
    int lockFlags = typeItem->data(Qt::UserRole).toInt();
    bool isPrivate = lockFlags & (1 << 1);
    bool isLocked = lockFlags & 1;

    if (exportPrivate && !isPrivate) {
        showError(tr("Cannot export private keys for public keychain."));
        return;
    }

    if (exportPrivate && isLocked) {
        showError(tr("Cannot export private keys for locked keychain."));
        return;
    }

    QStandardItem* nameItem = keychainModel->item(row, 0);
    QString name = nameItem->data(Qt::DisplayRole).toString();

    QString fileName = name + (exportPrivate ? ".priv.keys" : ".pub.keys");

    fileName = QFileDialog::getSaveFileName(
        this,
        tr("Exporting ") + (exportPrivate ? tr("Private") : tr("Public")) + tr(" Keychain - ") + name,
        lastVaultDir + "/" + fileName,
        tr("Keychains (*.keys)"));

    if (fileName.isEmpty()) return;

    QFileInfo fileInfo(fileName);
    lastVaultDir = fileInfo.dir().absolutePath();
    saveSettings();

    try {
        updateStatusMessage(tr("Exporting ") + (exportPrivate ? tr("private") : tr("public")) + tr("  keychain...") + name);
        keychainModel->exportKeychain(name, fileName, exportPrivate);
        updateStatusMessage(tr("Saved ") + fileName);
    }
    catch (const exception& e) {
        LOGGER(debug) << "MainWindow::exportKeychain - " << e.what() << std::endl;
        showError(e.what());
    }
}

void MainWindow::backupKeychain()
{
    QModelIndex index = keychainSelectionModel->currentIndex();
    int row = index.row();
    if (row < 0) {
        showError(tr("No keychain is selected."));
        return;
    }

    QStandardItem* nameItem = keychainModel->item(row, 0);
    QString name = nameItem->data(Qt::DisplayRole).toString();

    try {
        bytes_t extendedKey;

        if (keychainModel->isPrivate(name)) {
            // TODO: prompt user for decryption key
            extendedKey = keychainModel->getExtendedKeyBytes(name, true);
        }
        else {
            extendedKey = keychainModel->getExtendedKeyBytes(name);
        }

        KeychainBackupDialog dlg(tr("Keychain information"));
        dlg.setExtendedKey(extendedKey);
        dlg.exec();
    }
    catch (const exception& e) {
        LOGGER(debug) << "MainWindow::backupKeychain - " << e.what() << std::endl;
        showError(e.what());
    }
}

void MainWindow::updateCurrentKeychain(const QModelIndex& current, const QModelIndex& /*previous*/)
{
    int row = current.row();
    if (row == -1) {
        exportPrivateKeychainAction->setEnabled(false);
        exportPublicKeychainAction->setEnabled(false);
        backupKeychainAction->setEnabled(false);
    }
    else {
        QStandardItem* typeItem = keychainModel->item(row, 1);
        int lockFlags = typeItem->data(Qt::UserRole).toInt();
        bool isPrivate = lockFlags & (1 << 1);
        bool isLocked = lockFlags & 1;

        unlockKeychainAction->setEnabled(isPrivate && isLocked);
        lockKeychainAction->setEnabled(isPrivate && !isLocked);
        exportPrivateKeychainAction->setEnabled(isPrivate);
        exportPublicKeychainAction->setEnabled(true);
        backupKeychainAction->setEnabled(true);
    }
}

void MainWindow::updateSelectedKeychains(const QItemSelection& /*selected*/, const QItemSelection& /*deselected*/)
{
    int selectionCount = keychainView->selectionModel()->selectedRows(0).size();
    bool isSelected = selectionCount > 0;
    newAccountAction->setEnabled(isSelected);
}

bool MainWindow::selectAccount(int i)
{
    if (accountModel->rowCount() <= i) return false;
    QItemSelection selection(accountModel->index(i, 0), accountModel->index(i, accountModel->columnCount() - 1));
    accountView->selectionModel()->select(selection, QItemSelectionModel::SelectCurrent);
    return true;
}

void MainWindow::updateCurrentAccount(const QModelIndex& /*current*/, const QModelIndex& /*previous*/)
{
/*
    int row = current.row();
    bool isCurrent = (row != -1);

    deleteAccountAction->setEnabled(isCurrent);
    viewAccountHistoryAction->setEnabled(isCurrent);
*/
}

void MainWindow::updateSelectedAccounts(const QItemSelection& /*selected*/, const QItemSelection& /*deselected*/)
{
    QItemSelectionModel* selectionModel = accountView->selectionModel();
    QModelIndexList indexes = selectionModel->selectedRows(0);
    bool isSelected = indexes.size() > 0;
    if (isSelected) {
        QString accountName = accountModel->data(indexes.at(0)).toString();
        txModel->setAccount(accountName);
        txModel->update();
        txView->update();
        tabWidget->setTabText(2, tr("Transactions - ") + accountName);
        requestPaymentDialog->setCurrentAccount(accountName);
    }
    else {
        tabWidget->setTabText(2, tr("Transactions"));
    }
    deleteAccountAction->setEnabled(isSelected);
    exportAccountAction->setEnabled(isSelected);
    viewAccountHistoryAction->setEnabled(isSelected);
    viewScriptsAction->setEnabled(isSelected);
    requestPaymentAction->setEnabled(isSelected);
    sendPaymentAction->setEnabled(isSelected);
    viewUnsignedTxsAction->setEnabled(isSelected);
}

void MainWindow::quickNewAccount()
{
    QuickNewAccountDialog dlg(this);
    while (dlg.exec()) {
        try {
            QString accountName = dlg.getName();
            if (accountName.isEmpty())
                throw std::runtime_error(tr("Account name is empty.").toStdString());

            if (dlg.getMinSigs() > dlg.getMaxSigs())
                throw std::runtime_error(tr("The minimum signature count cannot exceed the number of keychains.").toStdString());

            if (accountModel->accountExists(accountName))
                throw std::runtime_error(tr("An account with that name already exists.").toStdString());

            const int MAX_KEYCHAIN_INDEX = 1000;
            int i = 0;
            QList<QString> keychainNames;
            while (keychainNames.size() < dlg.getMaxSigs() && ++i <= MAX_KEYCHAIN_INDEX) {
                QString keychainName = accountName + "_" + QString::number(i);
                if (!keychainModel->exists(keychainName))
                    keychainNames << keychainName;
            }
            if (i > MAX_KEYCHAIN_INDEX)
                throw std::runtime_error(tr("Ran out of keychain indices.").toStdString());

            for (auto& keychainName: keychainNames) {
                // TODO: Randomize using user input for entropy
                accountModel->newKeychain(keychainName, random_bytes(32));
            }

            accountModel->newAccount(accountName, dlg.getMinSigs(), keychainNames);
            accountModel->update();
            keychainModel->update();
            keychainView->update();
            tabWidget->setCurrentWidget(accountView);
            updateStatusMessage(tr("Created account ") + accountName);
            break;
        }
        catch (const exception& e) {
            showError(e.what());
        }
    }
}

void MainWindow::newAccount()
{
    QItemSelectionModel* selectionModel = keychainView->selectionModel();
    QModelIndexList indexes = selectionModel->selectedRows(0);

    QList<QString> keychainNames;
    for (auto& index: indexes) {
        keychainNames << keychainModel->data(index).toString();
    }

    try {
        NewAccountDialog dlg(keychainNames, this);
        if (dlg.exec()) {
            accountModel->newAccount(dlg.getName(), dlg.getMinSigs(), dlg.getKeychainNames());
            accountView->update();
            tabWidget->setCurrentWidget(accountView);
            networkSync.setBloomFilter(accountModel->getBloomFilter(0.0001, 0, 0));
            if (connected) resync();
        }
    }
    catch (const exception& e) {
        // TODO: Handle other possible errors.
        showError(tr("No keychains selected."));
    }
}

void MainWindow::importAccount(QString fileName)
{
    if (fileName.isEmpty()) {
        fileName = QFileDialog::getOpenFileName(
            this,
            tr("Import Account"),
            lastVaultDir,
            tr("Account") + "(*.acct)");
    }

    if (fileName.isEmpty()) return;

    QFileInfo fileInfo(fileName);
    lastVaultDir = fileInfo.dir().absolutePath();
    saveSettings();

    bool ok;
    QString name = QFileInfo(fileName).baseName();
    while (true) {
        name = QInputDialog::getText(this, tr("Account Import"), tr("Account Name:"), QLineEdit::Normal, name, &ok);
        if (!ok) return;
        if (name.isEmpty()) {
            showError(tr("Name cannot be empty."));
            continue;
        }
        if (accountModel->accountExists(name)) {
            showError(tr("There is already an account with that name."));
            continue;
        }
        break;
    }

    try {
        updateStatusMessage(tr("Importing account..."));
        accountModel->importAccount(name, fileName);
        accountModel->update();
        accountView->update();
        keychainModel->update();
        keychainView->update();
        tabWidget->setCurrentWidget(accountView);
        networkSync.setBloomFilter(accountModel->getBloomFilter(0.0001, 0, 0));
        updateStatusMessage(tr("Imported account ") + name);
        promptResync();
    }
    catch (const exception& e) {
        LOGGER(debug) << "MainWindow::importAccount - " << e.what() << std::endl;
        showError(e.what());
    }

}

void MainWindow::exportAccount()
{
    QItemSelectionModel* selectionModel = accountView->selectionModel();
    QModelIndexList indexes = selectionModel->selectedRows(0);
    if (indexes.isEmpty()) {
        showError(tr("No account selected."));
        return;
    }

    QString name = accountModel->data(indexes.at(0)).toString();

    QString fileName = name + ".acct";

    fileName = QFileDialog::getSaveFileName(
        this,
        tr("Exporting Account - ") + name,
        lastVaultDir + "/" + fileName,
        tr("Accounts (*.acct)"));

    if (fileName.isEmpty()) return;

    QFileInfo fileInfo(fileName);
    lastVaultDir = fileInfo.dir().absolutePath();
    saveSettings();

    try {
        updateStatusMessage(tr("Exporting account ") + name + "...");
        accountModel->exportAccount(name, fileName);
        updateStatusMessage(tr("Saved ") + fileName);
    }
    catch (const exception& e) {
        LOGGER(debug) << "MainWindow::exportAccount - " << e.what() << std::endl;
        showError(e.what());
    }
}

void MainWindow::deleteAccount()
{
    QItemSelectionModel* selectionModel = accountView->selectionModel();
    QModelIndexList indexes = selectionModel->selectedRows(0);
    if (indexes.isEmpty()) {
        showError(tr("No account selected."));
        return;
    }

    QString accountName = accountModel->data(indexes.at(0)).toString();

    if (QMessageBox::Yes == QMessageBox::question(this, tr("Confirm"), tr("Are you sure you want to delete account ") + accountName + "?")) {
        try {
            accountModel->deleteAccount(accountName);
            accountView->update();
            networkSync.setBloomFilter(accountModel->getBloomFilter(0.0001, 0, 0));
        }
        catch (const exception& e) {
            LOGGER(debug) << "MainWindow::deleteAccount - " << e.what() << std::endl;
            showError(e.what());
        }
    }
}

void MainWindow::viewAccountHistory()
{
    QItemSelectionModel* selectionModel = accountView->selectionModel();
    QModelIndexList indexes = selectionModel->selectedRows(0);
    if (indexes.isEmpty()) {
        showError(tr("No account selected."));
        return;
    }

    // Only open one - show modal for now.
    QString accountName = accountModel->data(indexes.at(0)).toString();

    try {
        // TODO: Make this dialog not modal and allow viewing several account histories simultaneously.
        AccountHistoryDialog dlg(accountModel->getVault(), accountName, &networkSync, this);
        connect(&dlg, &AccountHistoryDialog::txDeleted, [this]() { accountModel->update(); });
        dlg.exec();
    }
    catch (const exception& e) {
        LOGGER(debug) << "MainWindow::viewAccountHistory - " << e.what() << std::endl;
        showError(e.what());
    }
}

void MainWindow::viewScripts()
{
    QItemSelectionModel* selectionModel = accountView->selectionModel();
    QModelIndexList indexes = selectionModel->selectedRows(0);
    if (indexes.isEmpty()) {
        showError(tr("No account selected."));
        return;
    }

    // Only open one - show modal for now.
    QString accountName = accountModel->data(indexes.at(0)).toString();

    try {
        // TODO: Make this dialog not modal and allow viewing several account histories simultaneously.
        ScriptDialog dlg(accountModel->getVault(), accountName, this);
        dlg.exec();
    }
    catch (const exception& e) {
        LOGGER(debug) << "MainWindow::viewScripts - " << e.what() << std::endl;
        showError(e.what());
    }
}

void MainWindow::requestPayment()
{
    QItemSelectionModel* selectionModel = accountView->selectionModel();
    QModelIndexList indexes = selectionModel->selectedRows(0);
    if (indexes.isEmpty()) {
        showError(tr("No account selected."));
        return;
    }

    // Only open one
    QString accountName = accountModel->data(indexes.at(0)).toString();

    try {
        //RequestPaymentDialog dlg(accountModel, accountName);
        requestPaymentDialog->show();
        requestPaymentDialog->raise();
        requestPaymentDialog->activateWindow();
        //dlg.exec();
/*
        if (dlg.exec()) {
            accountName = dlg.getAccountName(); // in case it was changed by user
            QString label = dlg.getSender();
            auto pair = accountModel->issueNewScript(accountName, label);

            // TODO: create a prettier dialog to display this information
            QMessageBox::information(this, tr("New Script - ") + accountName,
                tr("Label: ") + label + tr(" Address: ") + pair.first + tr(" Script: ") + QString::fromStdString(uchar_vector(pair.second).getHex()));
        }
*/
    }
    catch (const exception& e) {
        LOGGER(debug) << "MainWindow::requestPayment - " << e.what() << std::endl;
        showError(e.what());
    }
}

void MainWindow::viewUnsignedTxs()
{
    showError(tr("Not implemented yet"));
}

void MainWindow::insertRawTx()
{
    try {
        RawTxDialog dlg(tr("Add Raw Transaction:"));
        if (dlg.exec() && accountModel->insertRawTx(dlg.getRawTx())) {
            accountView->update();
            txModel->update();
            txView->update();
            tabWidget->setCurrentWidget(txView);
        } 
    }
    catch (const exception& e) {
        LOGGER(debug) << "MainWindow::insertRawTx - " << e.what() << std::endl;
        showError(e.what());
    } 
}

void MainWindow::createRawTx()
{
    QItemSelectionModel* selectionModel = accountView->selectionModel();
    QModelIndexList indexes = selectionModel->selectedRows(0);
    if (indexes.isEmpty()) {
        showError(tr("No account selected."));
        return;
    }
 
    QString accountName = accountModel->data(indexes.at(0)).toString();

    CreateTxDialog dlg(accountName);
    while (dlg.exec()) {
        try {
            std::vector<TaggedOutput> outputs = dlg.getOutputs();
            uint64_t fee = dlg.getFeeValue();
            bytes_t rawTx = accountModel->createRawTx(dlg.getAccountName(), outputs, fee);
            RawTxDialog rawTxDlg(tr("Unsigned Transaction"));
            rawTxDlg.setRawTx(rawTx);
            rawTxDlg.exec();
            return;
        }
        catch (const exception& e) {
            LOGGER(debug) << "MainWindow::createRawTx - " << e.what() << std::endl;
            showError(e.what());
        }
    }
}

void MainWindow::createTx(const PaymentRequest& paymentRequest)
{
    QItemSelectionModel* selectionModel = accountView->selectionModel();
    QModelIndexList indexes = selectionModel->selectedRows(0);
    if (indexes.isEmpty()) {
        showError(tr("No account selected."));
        return;
    }
 
    QString accountName = accountModel->data(indexes.at(0)).toString();

    CreateTxDialog dlg(accountName, paymentRequest);
    bool saved = false;
    while (!saved && dlg.exec()) {
        try {
            CreateTxDialog::status_t status = dlg.getStatus();
            bool sign = status == CreateTxDialog::SIGN_AND_SEND || status == CreateTxDialog::SIGN_AND_SAVE;
            std::shared_ptr<CoinDB::Tx> tx = accountModel->createTx(dlg.getAccountName(), dlg.getTxOuts(), dlg.getFeeValue());
            tx = accountModel->insertTx(tx, sign);
            saved = true;
            txModel->update();
            txView->update();
            tabWidget->setCurrentWidget(txView);
            if (status == CreateTxDialog::SIGN_AND_SEND) {
                // TODO: Clean up signing and sending code
                if (tx->status() == CoinDB::Tx::UNSIGNED) {
                    throw std::runtime_error(tr("Could not send - transaction still missing signatures.").toStdString());
                }
                if (!connected) {
                    throw std::runtime_error(tr("Must be connected to network to send.").toStdString());
                }
                Coin::Transaction coin_tx = tx->toCoinCore();
                networkSync.sendTx(coin_tx);

                // TODO: Check transaction has propagated before changing status
                tx->updateStatus(CoinDB::Tx::PROPAGATED);
                accountModel->getVault()->insertTx(tx);
            }
            txModel->update();
            txView->update();
            return;
        }
        catch (const exception& e) {
            LOGGER(debug) << "MainWindow::createTx - " << e.what() << std::endl;
            showError(e.what());
        }
    }
}

void MainWindow::signRawTx()
{
    try {
        RawTxDialog dlg(tr("Sign Raw Transaction:"));
        if (dlg.exec()) {
            bytes_t rawTx = accountModel->signRawTx(dlg.getRawTx());
            RawTxDialog rawTxDlg(tr("Signed Transaction"));
            rawTxDlg.setRawTx(rawTx);
            rawTxDlg.exec();
        }
    }
    catch (const exception& e) {
        LOGGER(debug) << "MainWindow::signRawTx - " << e.what() << std::endl;
        showError(e.what());
    }

}

void MainWindow::sendRawTx()
{
    if (!connected) {
        showError(tr("Must be connected to send raw transaction"));
        return;
    }
 
    try {
        RawTxDialog dlg(tr("Send Raw Transaction:"));
        if (dlg.exec()) {
            bytes_t rawTx = dlg.getRawTx();
            Coin::Transaction tx(rawTx);
            networkSync.sendTx(tx);
            updateStatusMessage(tr("Sent tx " ) + QString::fromStdString(tx.getHashLittleEndian().getHex()) + tr(" to peer"));
        }
    }
    catch (const exception& e) {
        LOGGER(debug) << "MainWindow::sendRawTx - " << e.what() << std::endl;
        showError(e.what());
    }
}

void MainWindow::newTx(const bytes_t& hash)
{
    QString message = tr("Added transaction ") + QString::fromStdString(uchar_vector(hash).getHex());
//    updateStatusMessage(tr("Added transaction ") + QString::fromStdString(uchar_vector(hash).getHex()));
    emit status(message);

    txModel->update();
    txView->update();
}

void MainWindow::newBlock(const bytes_t& hash, int height)
{
    if (height > syncHeight) {
        emit updateSyncHeight(height);
    }
    QString message = tr("Inserted block ") + QString::fromStdString(uchar_vector(hash).getHex()) + tr(" height: ") + QString::number(height);
//    updateStatusMessage(tr("Inserted block ") + QString::fromStdString(uchar_vector(hash).getHex()) + tr(" height: ") + QString::number(height));
    emit status(message);

    txModel->update();
    txView->update();
}

/*
void MainWindow::newTx(const coin_tx_t& tx)
{
    QString hash = QString::fromStdString(tx.getHashLittleEndian().getHex());
    try {
        if (accountModel->insertTx(tx)) {
            accountView->update();
            updateStatusMessage(tr("Received transaction ") + hash);
        }
    }
    catch (const exception& e) {
        LOGGER(debug) << "MainWindow::newTx - " << hash.toStdString() << " : " << e.what();
        updateStatusMessage(tr("Error attempting to insert transaction ") + hash);
    }
}

void MainWindow::newBlock(const chain_block_t& block)
{
    QString hash = QString::fromStdString(block.blockHeader.getHashLittleEndian().getHex());
    updateStatusMessage(tr("Received block ") + hash + tr(" with height ") + QString::number(block.height));
    if (accountModel->isOpen()) {
        try {
            updateStatusaccountModel->insertBlock(block);
        }
        catch (const exception& e) {
            LOGGER(debug) << "MainWindow::newBlock - " << hash.toStdString() << " : " << e.what();
            showError(e.what());
        }
    }
    updateBestHeight(networkSync.getBestHeight());
}
*/

void MainWindow::resync()
{
    updateStatusMessage(tr("Resynchronizing vault"));
    uint32_t startTime = accountModel->getMaxFirstBlockTimestamp();
    std::vector<bytes_t> locatorHashes = accountModel->getLocatorHashes();
    for (auto& hash: locatorHashes) {
        LOGGER(debug) << "MainWindow::resync() - hash: " << uchar_vector(hash).getHex() << std::endl;
    }
    networkSync.resync(locatorHashes, startTime);
}

void MainWindow::doneSync()
{
    doneHeaderSync = true;
    emit updateBestHeight(networkSync.getBestHeight());
//    updateStatusMessage(tr("Finished headers sync"));
    emit status(tr("Finished headers sync"));
    //networkStateLabel->setPixmap(*synchedIcon);
    if (accountModel->isOpen()) {
        try {
            resync();
        }
        catch (const exception& e) {
            LOGGER(debug) << "MainWindow::doneSync - " << e.what() << std::endl;
//            updateStatusMessage(QString::fromStdString(e.what()));
            emit status(QString::fromStdString(e.what()));
        }
    }
}

void MainWindow::doneResync()
{
    stopResyncAction->setEnabled(false);
//    updateStatusMessage(tr("Finished resynch"));
    emit status(tr("Finished resynch"));
}

void MainWindow::addBestChain(const chain_header_t& header)
{
    emit updateBestHeight(header.height);
}

void MainWindow::removeBestChain(const chain_header_t& header)
{
    bytes_t hash = header.getHashLittleEndian();
    LOGGER(debug) << "MainWindow::removeBestChain - " << uchar_vector(hash).getHex() << std::endl;
    //accountModel->deleteMerkleBlock(hash);    
    int diff = bestHeight - networkSync.getBestHeight();
    if (diff >= 0) {
        QString message = tr("Reorganization of ") + QString::number(diff + 1) + tr(" blocks");
//        updateStatusMessage(tr("Reorganization of ") + QString::number(diff + 1) + tr(" blocks"));
        emit status(message);
        emit updateBestHeight(networkSync.getBestHeight());
    }
}

void MainWindow::connectionOpen()
{
//    updateStatusMessage(tr("Connection opened"));
    QString message = tr("Connected to ") + host + ":" + QString::number(port);
//    updateStatusMessage(tr("Connected to ") + host + ":" + QString::number(port));
    emit status(message);
//    networkStateLabel->setText(tr("Connected to ") + host + ":" + QString::number(port));
}

void MainWindow::connectionClosed()
{
    doneHeaderSync = false;
//    updateStatusMessage(tr("Connection closed"));
    emit status(tr("Connection closed"));
    // networkStateLabel->setPixmap(*notConnectedIcon);    
//    updateStatusMessage(tr("Disconneted"));
//    networkStateLabel->setText(tr("Disconnected"));
}

void MainWindow::startNetworkSync()
{
    connectAction->setEnabled(false);
    //networkStateLabel->setMovie(synchingMovie);
    try {
        QString message(tr("Connecting to ") + host + ":" + QString::number(port) + "...");
        updateStatusMessage(message);
//        updateStatusMessage(tr("Connecting to ") + host + ":" + QString::number(port) + "...");
        networkSync.start(host.toStdString(), port);
        updateNetworkState();
    }
    catch (const exception& e) {
        LOGGER(debug) << "MainWindow::startNetworkSync - " << e.what() << std::endl;
        showError(e.what());
    }
}

void MainWindow::stopNetworkSync()
{
    disconnectAction->setEnabled(false);
    try {
        updateStatusMessage(tr("Disconnecting..."));
        networkSync.stop();
    }
    catch (const exception& e) {
        LOGGER(debug) << "MainWindow::stopNetworkSync - " << e.what() << std::endl;
        showError(e.what());
    }
}

void MainWindow::resyncBlocks()
{
    if (!connected) {
        showError(tr("Must be connected to resync."));
    }

    ResyncDialog dlg(resyncHeight, networkSync.getBestHeight());
    if (dlg.exec()) {
        stopResyncAction->setEnabled(true);
        resyncHeight = dlg.getResyncHeight();
        try {
            networkSync.resync(resyncHeight);
        }
        catch (const exception& e) {
            LOGGER(debug) << "MainWindow::resyncBlocks - " << e.what() << std::endl;
            showError(e.what());
        }
    }
}

void MainWindow::stopResyncBlocks()
{
    networkSync.stopResync();
}

void MainWindow::networkStatus(const QString& status)
{
    updateStatusMessage(status);
}

void MainWindow::networkError(const QString& error)
{
    QString message = tr("Network Error: ") + error;
//    updateStatusMessage(tr("Network Error: ") + error);
    emit status(message);
    connected = false;
    disconnectAction->setEnabled(false);
    connectAction->setEnabled(true);
    sendRawTxAction->setEnabled(false);

    //boost::lock_guard<boost::mutex> lock(repaintMutex);
    //networkStateLabel->setText(tr("Disconnected"));
}

void MainWindow::networkStarted()
{
    connected = true;
    connectAction->setEnabled(false);
    disconnectAction->setEnabled(true);
    resyncAction->setEnabled(true);
    sendRawTxAction->setEnabled(true);
//    updateStatusMessage(tr("Network started"));
    emit status(tr("Network started"));
}

// TODO: put common state change operations into common functions
void MainWindow::networkStopped()
{
    connected = false;
    updateNetworkState();
    resyncAction->setEnabled(false);
    disconnectAction->setEnabled(false);
    connectAction->setEnabled(true);
    sendRawTxAction->setEnabled(false);
}

void MainWindow::networkTimeout()
{
    connected = false;
//    updateStatusMessage(tr("Network timed out"));
    emit status(tr("Network timed out"));
    disconnectAction->setEnabled(false);
    connectAction->setEnabled(true);
    sendRawTxAction->setEnabled(false);
    //networkStateLabel->setText(tr("Disconnected"));
    // TODO: Attempt reconnect
}

void MainWindow::networkSettings()
{
    // TODO: Add a connect button to dialog.
    NetworkSettingsDialog dlg(host, port, autoConnect);
    if (dlg.exec()) {
        host = dlg.getHost();
        port = dlg.getPort();
        autoConnect = dlg.getAutoConnect();
        connectAction->setText(tr("Connect to ") + host);

        QSettings settings("Ciphrex", getDefaultSettings().getAppName());
        settings.setValue("host", host);
        settings.setValue("port", port);
    }
}

void MainWindow::about()
{
   QMessageBox::about(this, tr("About ") + getDefaultSettings().getAppName(),
            tr("<b>") + getDefaultSettings().getAppName() + "(TM) " + VERSIONTEXT + "</b><br />" +
            "Commit: " + getCommitHash() + "<br />" + COPYRIGHTTEXT);
}

void MainWindow::errorStatus(const QString& message)
{
    LOGGER(debug) << "MainWindow::errorStatus - " << message.toStdString() << std::endl;
    QString error = tr("Error - ") + message;
//    updateStatusMessage(tr("Error - ") + message);
    emit status(error);
}

void MainWindow::processUrl(const QUrl& url)
{
    try {
        if (url.scheme().compare("bitcoin:", Qt::CaseInsensitive)) {
            PaymentRequest paymentRequest(url);
            createTx(paymentRequest);            
        }
        else {
            throw std::runtime_error(tr("Unhandled URL protocol").toStdString());
        }
    }
    catch (const exception& e) {
        showError(e.what());
    }
}

void MainWindow::processFile(const QString& fileName)
{
    QString ext = fileName.section('.', -1);
    if (ext == "vault") {
        openVault(fileName);
    }
    else if (ext == "acct") {
        importAccount(fileName);
    }
    else if (ext == "keys") {
        importKeychain(fileName);
    }
    else {
        LOGGER(debug) << "MainWindow::processFile - unhandled file type: " << fileName.toStdString() << std::endl;
    }
}

void MainWindow::processCommand(const QString& command)
{
    LOGGER(trace) << "MainWindow::processCommand - " << command.toStdString() << std::endl;
    if (command == "clearsettings") {
        clearSettings();
    }
    else if (command == "unlicense") {
        licenseAccepted = false;
        saveSettings();
    }
    else {
        LOGGER(trace) << "MainWindow::processCommand - unhandled command: " << command.toStdString() << std::endl;
    }
}

void MainWindow::createActions()
{
    // application actions
    quitAction = new QAction(tr("&Quit ") + getDefaultSettings().getAppName(), this);
    quitAction->setShortcuts(QKeySequence::Quit);
    quitAction->setStatusTip(tr("Quit the application"));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

    // vault actions
    newVaultAction = new QAction(QIcon(":/icons/newvault.png"), tr("&New Vault..."), this);
    newVaultAction->setShortcuts(QKeySequence::New);
    newVaultAction->setStatusTip(tr("Create a new vault"));
    connect(newVaultAction, SIGNAL(triggered()), this, SLOT(newVault()));

    openVaultAction = new QAction(QIcon(":/icons/openvault.png"), tr("&Open Vault..."), this);
    openVaultAction->setShortcuts(QKeySequence::Open);
    openVaultAction->setStatusTip(tr("Open an existing vault"));
    connect(openVaultAction, SIGNAL(triggered()), this, SLOT(openVault())); 

    closeVaultAction = new QAction(QIcon(":/icons/closevault.png"), tr("Close Vault"), this);
    closeVaultAction->setStatusTip(tr("Close vault"));
    closeVaultAction->setEnabled(false);
    connect(closeVaultAction, SIGNAL(triggered()), this, SLOT(closeVault()));

    // keychain actions
    newKeychainAction = new QAction(QIcon(":/icons/keypair.png"), tr("New &Keychain..."), this);
    newKeychainAction->setStatusTip(tr("Create a new keychain"));
    newKeychainAction->setEnabled(false);
    connect(newKeychainAction, SIGNAL(triggered()), this, SLOT(newKeychain()));

    unlockKeychainAction = new QAction(tr("Unlock keychain..."), this);
    unlockKeychainAction->setStatusTip(tr("Unlock keychain"));
    unlockKeychainAction->setEnabled(false);
    connect(unlockKeychainAction, SIGNAL(triggered()), this, SLOT(unlockKeychain()));

    lockKeychainAction = new QAction(tr("Lock keychain"), this);
    lockKeychainAction->setStatusTip(tr("Lock keychain"));
    lockKeychainAction->setEnabled(false);
    connect(lockKeychainAction, SIGNAL(triggered()), this, SLOT(lockKeychain()));

    lockAllKeychainsAction = new QAction(tr("Lock all keychains"), this);
    lockAllKeychainsAction->setStatusTip(tr("Lock all keychains"));
    lockAllKeychainsAction->setEnabled(false);
    connect(lockAllKeychainsAction, SIGNAL(triggered()), this, SLOT(lockAllKeychains()));

    importPrivateAction = new QAction(tr("Private Imports"), this);
    importPrivateAction->setCheckable(true);
    importPrivateAction->setStatusTip(tr("Import private keys if available"));
    connect(importPrivateAction, &QAction::triggered, [=]() { this->importPrivate = true; });

    importPublicAction = new QAction(tr("Public Imports"), this);
    importPublicAction->setCheckable(true);
    importPublicAction->setStatusTip(tr("Only import public keys"));
    connect(importPublicAction, &QAction::triggered, [=]() { this->importPrivate = false; });

    importModeGroup = new QActionGroup(this);
    importModeGroup->addAction(importPrivateAction);
    importModeGroup->addAction(importPublicAction);
    importPrivateAction->setChecked(true);
    importPrivate = true;

    importKeychainAction = new QAction(tr("Import Keychain..."), this);
    importKeychainAction->setStatusTip(tr("Import keychain from file"));
    importKeychainAction->setEnabled(false);
    connect(importKeychainAction, SIGNAL(triggered()), this, SLOT(importKeychain()));

    exportPrivateKeychainAction = new QAction(tr("Export Private Keychain..."), this);
    exportPrivateKeychainAction->setStatusTip(tr("Export private keychain to file"));
    exportPrivateKeychainAction->setEnabled(false);
    connect(exportPrivateKeychainAction, &QAction::triggered, [=]() { this->exportKeychain(true); });

    exportPublicKeychainAction = new QAction(tr("Export Public Keychain..."), this);
    exportPublicKeychainAction->setStatusTip(tr("Export public keychain to file"));
    exportPublicKeychainAction->setEnabled(false);
    connect(exportPublicKeychainAction, &QAction::triggered, [=]() { this->exportKeychain(false); });

    backupKeychainAction = new QAction(tr("Backup Keychain..."), this);
    backupKeychainAction->setStatusTip(tr("Make paper backup"));
    backupKeychainAction->setEnabled(false);
    connect(backupKeychainAction, SIGNAL(triggered()), this, SLOT(backupKeychain()));

    // account actions
    quickNewAccountAction = new QAction(QIcon(":/icons/magicwand.png"), tr("Account &Wizard..."), this);
    quickNewAccountAction->setStatusTip(tr("Create a new account, automatically create new keychains for it"));
    quickNewAccountAction->setEnabled(false);
    connect(quickNewAccountAction, SIGNAL(triggered()), this, SLOT(quickNewAccount()));

    newAccountAction = new QAction(QIcon(":/icons/money.png"), tr("Create &Account..."), this);
    newAccountAction->setStatusTip(tr("Create a new account with selected keychains"));
    newAccountAction->setEnabled(false);
    connect(newAccountAction, SIGNAL(triggered()), this, SLOT(newAccount()));

    requestPaymentAction = new QAction(QIcon(":/icons/cashregister.png"), tr("Request Payment..."), this);
    requestPaymentAction->setStatusTip(tr("Get a new address to request a payment"));
    requestPaymentAction->setEnabled(false);
    connect(requestPaymentAction, SIGNAL(triggered()), this, SLOT(requestPayment()));

    sendPaymentAction = new QAction(QIcon(":/icons/payment.png"), tr("Send Payment..."), this);
    sendPaymentAction->setStatusTip(tr("Create a new transaction to send payment"));
    sendPaymentAction->setEnabled(false);
    connect(sendPaymentAction, SIGNAL(triggered()), this, SLOT(createTx()));

    importAccountAction = new QAction(tr("Import Account..."), this);
    importAccountAction->setStatusTip(tr("Import an account"));
    importAccountAction->setEnabled(false);
    connect(importAccountAction, SIGNAL(triggered()), this, SLOT(importAccount()));

    exportAccountAction = new QAction(tr("Export Account..."), this);
    exportAccountAction->setStatusTip(tr("Export an account"));
    exportAccountAction->setEnabled(false);
    connect(exportAccountAction, SIGNAL(triggered()), this, SLOT(exportAccount()));

    deleteAccountAction = new QAction(tr("Delete Account"), this);
    deleteAccountAction->setStatusTip(tr("Delete current account"));
    deleteAccountAction->setEnabled(false);
    connect(deleteAccountAction, SIGNAL(triggered()), this, SLOT(deleteAccount()));

    viewAccountHistoryAction = new QAction(tr("View Account History"), this);
    viewAccountHistoryAction->setStatusTip(tr("View history for active account"));
    viewAccountHistoryAction->setEnabled(false);
    connect(viewAccountHistoryAction, SIGNAL(triggered()), this, SLOT(viewAccountHistory()));

    viewScriptsAction = new QAction(tr("View Scripts"), this);
    viewScriptsAction->setStatusTip(tr("View scripts for active account"));
    viewScriptsAction->setEnabled(false);
    connect(viewScriptsAction, SIGNAL(triggered()), this, SLOT(viewScripts()));
    
    viewUnsignedTxsAction = new QAction(tr("View Unsigned Transactions"), this);
    viewUnsignedTxsAction->setStatusTip(tr("View transactions pending signature"));
    viewUnsignedTxsAction->setEnabled(false);
    connect(viewUnsignedTxsAction, SIGNAL(triggered()), this, SLOT(viewUnsignedTxs()));

    // transaction actions
    insertRawTxAction = new QAction(tr("Insert Raw Transaction..."), this);
    insertRawTxAction->setStatusTip(tr("Insert a raw transaction into vault"));
    insertRawTxAction->setEnabled(false);
    connect(insertRawTxAction, SIGNAL(triggered()), this, SLOT(insertRawTx()));

    signRawTxAction = new QAction(tr("Sign Raw Transaction..."), this);
    signRawTxAction->setStatusTip(tr("Sign a raw transaction using keychains in vault"));
    signRawTxAction->setEnabled(false);
    connect(signRawTxAction, SIGNAL(triggered()), this, SLOT(signRawTx()));

    createRawTxAction = new QAction(tr("Create Transaction..."), this);
    createRawTxAction->setStatusTip(tr("Create a new transaction"));
    createRawTxAction->setEnabled(false);
//    connect(createRawTxAction, SIGNAL(triggered()), this, SLOT(createRawTx()));

    createTxAction = new QAction(tr("Create Transaction..."), this);
    createTxAction->setStatusTip(tr("Create a new transaction"));
    createTxAction->setEnabled(false);
    connect(createTxAction, SIGNAL(triggered()), this, SLOT(createTx()));

    sendRawTxAction = new QAction(tr("Send Raw Transaction..."), this);
    sendRawTxAction->setStatusTip(tr("Send a raw transaction to peer"));
    sendRawTxAction->setEnabled(false);
    connect(sendRawTxAction, SIGNAL(triggered()), this, SLOT(sendRawTx()));

    // network actions
    connectAction = new QAction(QIcon(":/icons/connect.png"), tr("Connect to ") + host, this);
    connectAction->setStatusTip(tr("Connect to a p2p node"));
    connectAction->setEnabled(true);

    disconnectAction = new QAction(QIcon(":/icons/disconnect.png"), tr("Disconnect from ") + host, this);
    disconnectAction->setStatusTip(tr("Disconnect from p2p node"));
    disconnectAction->setEnabled(false);

    resyncAction = new QAction(tr("Resync..."), this);
    resyncAction->setStatusTip(tr("Resync from a specific height"));
    resyncAction->setEnabled(false);

    stopResyncAction = new QAction(tr("Stop Resync"), this);
    stopResyncAction->setStatusTip(tr("Stop resync"));
    stopResyncAction->setEnabled(false);

    connect(connectAction, SIGNAL(triggered()), this, SLOT(startNetworkSync()));
    connect(disconnectAction, SIGNAL(triggered()), this, SLOT(stopNetworkSync()));
    connect(resyncAction, SIGNAL(triggered()), this, SLOT(resyncBlocks()));
    connect(stopResyncAction, SIGNAL(triggered()), this, SLOT(stopResyncBlocks()));

/*    connect(this, SIGNAL(status(const QString&)), this, SLOT(networkStatus(const QString&)));
    connect(this, SIGNAL(signal_error(const QString&)), this, SLOT(networkError(const QString&)));
    connect(this, SIGNAL(signal_connectionOpen()), this, SLOT(connectionOpen()));
    connect(this, SIGNAL(signal_connectionClosed()), this, SLOT(connectionClosed()));
    connect(this, SIGNAL(signal_networkStarted()), this, SLOT(networkStarted()));
    connect(this, SIGNAL(signal_networkStopped()), this, SLOT(networkStopped()));
    connect(this, SIGNAL(signal_networkTimeout()), this, SLOT(networkTimeout()));
    connect(this, SIGNAL(signal_networkDoneSync()), this, SLOT(doneSync()));
*/
    networkSync.subscribeStatus([this](const std::string& message) {
        LOGGER(debug) << "status slot" << std::endl;
        networkStatus(QString::fromStdString(message)); 
    });

    networkSync.subscribeError([this](const std::string& error) {
        LOGGER(debug) << "error slot" << std::endl;
        networkError(QString::fromStdString(error));
    });

    networkSync.subscribeOpen([this]() {
        LOGGER(debug) << "open slot" << std::endl;
        connectionOpen();
    });

    networkSync.subscribeClose([this]() {
        LOGGER(debug) << "close slot" << std::endl;
        connectionClosed();
    });

    networkSync.subscribeStarted([this]() {
        LOGGER(debug) << "started slot" << std::endl;
        networkStarted();
    });

    networkSync.subscribeStopped([this]() {
        LOGGER(debug) << "stopped slot" << std::endl;
        networkStopped();
    });

    networkSync.subscribeTimeout([this]() {
        LOGGER(debug) << "timeout slot" << std::endl;
        networkTimeout();
    });

    networkSync.subscribeDoneSync([this]() {
        LOGGER(debug) << "done sync slot" << std::endl;
        doneSync();
    });

/*
    qRegisterMetaType<chain_header_t>("chain_header_t");
    connect(this, SIGNAL(signal_addBestChain(const chain_header_t&)), this, SLOT(addBestChain(const chain_header_t&)));
    connect(this, SIGNAL(signal_removeBestChain(const chain_header_t&)), this, SLOT(removeBestChain(const chain_header_t&)));
*/

    networkSync.subscribeAddBestChain([this](const chain_header_t& header) {
        LOGGER(debug) << "add best chain slot" << std::endl;
        addBestChain(header);
    });

    networkSync.subscribeRemoveBestChain([this](const chain_header_t& header) {
        LOGGER(debug) << "remove best chain slot" << std::endl;
        removeBestChain(header);
    });

    networkSettingsAction = new QAction(tr("Settings..."), this);
    networkSettingsAction->setStatusTip(tr("Configure network settings"));
    networkSettingsAction->setEnabled(true);
    connect(networkSettingsAction, SIGNAL(triggered()), this, SLOT(networkSettings()));

    // about/help actions
    aboutAction = new QAction(tr("About..."), this);
    aboutAction->setStatusTip(tr("About ") + getDefaultSettings().getAppName());
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

    updateVaultStatus();
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(newVaultAction);
    fileMenu->addAction(openVaultAction);
    fileMenu->addAction(closeVaultAction);
    // fileMenu->addAction(importVaultAction);
    // fileMenu->addAction(exportVaultAction);
    fileMenu->addSeparator();
    fileMenu->addAction(quitAction);    

    keychainMenu = menuBar()->addMenu(tr("&Keychains"));
    keychainMenu->addAction(quickNewAccountAction);
    keychainMenu->addSeparator();
    keychainMenu->addAction(newKeychainAction);
    keychainMenu->addAction(newAccountAction);
    keychainMenu->addSeparator();
    keychainMenu->addAction(unlockKeychainAction);
    keychainMenu->addAction(lockKeychainAction);
    keychainMenu->addAction(lockAllKeychainsAction);
    keychainMenu->addSeparator()->setText(tr("Import Mode"));
    keychainMenu->addAction(importPrivateAction);
    keychainMenu->addAction(importPublicAction);
    keychainMenu->addSeparator();
    keychainMenu->addAction(importKeychainAction);
    keychainMenu->addAction(exportPrivateKeychainAction);
    keychainMenu->addAction(exportPublicKeychainAction);
    keychainMenu->addSeparator();
    keychainMenu->addAction(backupKeychainAction);

    accountMenu = menuBar()->addMenu(tr("&Accounts"));
    accountMenu->addAction(requestPaymentAction);
    accountMenu->addAction(sendPaymentAction);
    accountMenu->addSeparator();
    //accountMenu->addAction(deleteAccountAction);
    //accountMenu->addSeparator();
    accountMenu->addAction(importAccountAction);
    accountMenu->addAction(exportAccountAction);
    accountMenu->addSeparator();
    //accountMenu->addAction(viewAccountHistoryAction);
    accountMenu->addAction(viewScriptsAction);
    accountMenu->addSeparator();
    accountMenu->addAction(viewUnsignedTxsAction);

    txMenu = menuBar()->addMenu(tr("&Transactions"));
    txMenu->addAction(insertRawTxAction);
    //txMenu->addSeparator();
    //txMenu->addAction(signRawTxAction);
    //txMenu->addSeparator();
//    txMenu->addAction(createRawTxAction);
    //txMenu->addAction(createTxAction);
    //txMenu->addSeparator();
    txMenu->addAction(sendRawTxAction);

    networkMenu = menuBar()->addMenu(tr("&Network"));
    networkMenu->addAction(connectAction);
    networkMenu->addAction(disconnectAction);
    networkMenu->addSeparator();
    networkMenu->addAction(resyncAction);
    networkMenu->addAction(stopResyncAction);
    networkMenu->addSeparator();
    networkMenu->addAction(networkSettingsAction);

    menuBar()->addSeparator();

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAction);
}

void MainWindow::createToolBars()
{
    // TODO: rename toolbars more appropriately

    fileToolBar = addToolBar(tr("File"));
    fileToolBar->addAction(newVaultAction);
    fileToolBar->addAction(openVaultAction);
    fileToolBar->addAction(closeVaultAction);

    keychainToolBar = addToolBar(tr("Keychains"));
    keychainToolBar->addAction(quickNewAccountAction);
    keychainToolBar->addAction(newKeychainAction);
    keychainToolBar->addAction(newAccountAction);

    accountToolBar = addToolBar(tr("Accounts"));
    accountToolBar->addAction(requestPaymentAction);
    accountToolBar->addAction(sendPaymentAction);

    networkToolBar = addToolBar(tr("Network"));
    networkToolBar->addAction(connectAction);
    networkToolBar->addAction(disconnectAction);

/*
    editToolBar = addToolBar(tr("Edit"));
    editToolBar->addAction(newKeychainAction);
    editToolBar->addAction(newAccountAction);
    editToolBar->addAction(deleteAccountAction);

    txToolBar = addToolBar(tr("Transactions"));
    txToolBar->addAction(insertRawTxAction);
    txToolBar->addAction(createRawTxAction);
    txToolBar->addAction(signRawTxAction);
*/
}

void MainWindow::createStatusBar()
{
    syncLabel = new QLabel();
    updateSyncLabel();

    networkStateLabel = new QLabel();
    notConnectedIcon = new QPixmap(":/icons/vault_status_icons/36x22/nc-icon-36x22.gif");
    synchingMovie = new QMovie(":/icons/vault_status_icons/36x22/synching-icon-animated-36x22.gif");
    synchingMovie->start();
    synchedIcon = new QPixmap(":/icons/vault_status_icons/36x22/synched-icon-36x22.gif");
    networkStateLabel->setPixmap(*notConnectedIcon);

    statusBar()->addPermanentWidget(syncLabel);
    statusBar()->addPermanentWidget(networkStateLabel);

    updateStatusMessage(tr("Ready"));
}

void MainWindow::loadSettings()
{
    QSettings settings("Ciphrex", getDefaultSettings().getAppName());
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(800, 400)).toSize();
    resize(size);
    move(pos);

    licenseAccepted = settings.value("licenseaccepted", false).toBool();

    blockTreeFile = settings.value("blocktreefile", getDefaultSettings().getDataDir() + "/blocktree.dat").toString();
    host = settings.value("host", "localhost").toString();
    port = settings.value("port", getCoinParams().default_port()).toInt();
    autoConnect = settings.value("autoconnect", false).toBool();
    resyncHeight = settings.value("resyncheight", 0).toInt();

    lastVaultDir = settings.value("lastvaultdir", getDefaultSettings().getDocumentDir()).toString();
}

void MainWindow::saveSettings()
{
    QSettings settings("Ciphrex", getDefaultSettings().getAppName());
    settings.setValue("pos", pos());
    settings.setValue("size", size());
    settings.setValue("licenseaccepted", licenseAccepted);
    settings.setValue("blocktreefile", blockTreeFile);
    settings.setValue("host", host);
    settings.setValue("port", port);
    settings.setValue("autoconnect", autoConnect);
    settings.setValue("resyncheight", resyncHeight);
    settings.setValue("lastvaultdir", lastVaultDir);
}

void MainWindow::clearSettings()
{
    QSettings settings("Ciphrex", getDefaultSettings().getAppName());
    settings.clear();
    loadSettings();
}

void MainWindow::loadVault(const QString &fileName)
{
    accountModel->load(fileName);
    accountView->update();
    keychainModel->setVault(accountModel->getVault());
    keychainModel->update();
    keychainView->update();
    txModel->setVault(accountModel->getVault());
    txModel->update();
    txView->update();

    newKeychainAction->setEnabled(true);

    // TODO: Prompt user to unlock chain codes
    accountModel->getVault()->unlockChainCodes(uchar_vector("1234"));

    networkSync.setBloomFilter(accountModel->getBloomFilter(0.0001, 0, 0));
}
