///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// txactions.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "txactions.h"

#include "txmodel.h"
#include "txview.h"
#include "accountmodel.h"
#include "keychainmodel.h"

#include "rawtxdialog.h"
#include "txsearchdialog.h"
#include "signaturedialog.h"

#include "docdir.h"

#include "entropysource.h"

#include <CoinDB/SynchedVault.h>

#include <QAction>
#include <QMenu>
#include <QUrl>
#include <QDesktopServices>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>

#include <logger/logger.h>

#include <fstream>

TxActions::TxActions(TxModel* txModel, TxView* txView, AccountModel* accountModel, KeychainModel* keychainModel, CoinDB::SynchedVault* synchedVault, QWidget* parent)
    : m_parent(parent), m_txModel(txModel), m_txView(txView), m_accountModel(accountModel), m_keychainModel(keychainModel), m_synchedVault(synchedVault), currentRow(-1)
{
    createActions();
    createMenus();
    connect(m_txView->selectionModel(), &QItemSelectionModel::currentChanged, this, &TxActions::updateCurrentTx);
}

void TxActions::updateCurrentTx(const QModelIndex& current, const QModelIndex& /*previous*/)
{
    currentRow = current.row();
    if (m_txModel && currentRow != -1)
    {
        QStandardItem* typeItem = m_txModel->item(currentRow, 2);
        signaturesAction->setEnabled(typeItem->text() == tr("Send"));

        typeItem = m_txModel->item(currentRow, 6);
        int type = typeItem->data(Qt::UserRole).toInt();
        if (type == CoinDB::Tx::UNSIGNED) {
            signTxAction->setEnabled(true);
        }
        else {
            signTxAction->setEnabled(false);
        }

        if (type == CoinDB::Tx::UNSENT) {
            sendTxAction->setText(tr("Send Transaction"));
            sendTxAction->setEnabled(m_synchedVault && m_synchedVault->isConnected());
        }
        else {
            sendTxAction->setText(tr("Resend Transaction"));
            sendTxAction->setEnabled(m_synchedVault && m_synchedVault->isConnected() && typeItem->text() == "0");
        }

        exportTxToFileAction->setEnabled(true);
        copyAddressToClipboardAction->setEnabled(true);
        copyTxHashToClipboardAction->setEnabled(true);
        copyRawTxToClipboardAction->setEnabled(true);
        saveRawTxToFileAction->setEnabled(true);
        viewRawTxAction->setEnabled(true);
        viewTxOnWebAction->setEnabled(type == CoinDB::Tx::PROPAGATED || type == CoinDB::Tx::CONFIRMED);
        deleteTxAction->setEnabled(type != CoinDB::Tx::CONFIRMED);
    }
    else {
        signTxAction->setEnabled(false);
        sendTxAction->setEnabled(false);
        exportTxToFileAction->setEnabled(false);
        copyAddressToClipboardAction->setEnabled(false);
        copyTxHashToClipboardAction->setEnabled(false);
        copyRawTxToClipboardAction->setEnabled(false);
        saveRawTxToFileAction->setEnabled(false);
        viewRawTxAction->setEnabled(false);
        viewTxOnWebAction->setEnabled(false);
        deleteTxAction->setEnabled(false);
    }

}

void TxActions::updateVaultStatus()
{
    bool bEnabled = (m_accountModel && m_accountModel->isOpen());
    searchTxAction->setEnabled(bEnabled);
    importTxFromFileAction->setEnabled(bEnabled);
    importTxFromClipboardAction->setEnabled(bEnabled);
    importTxsFromFileAction->setEnabled(bEnabled);
    exportAllTxsToFileAction->setEnabled(bEnabled);
    insertRawTxFromClipboardAction->setEnabled(bEnabled);
    insertRawTxFromFileAction->setEnabled(bEnabled);
}

void TxActions::searchTx()
{
    try
    {
        // Sanity checks
        if (!m_txModel) throw std::runtime_error("No transaction model loaded.");
        if (!m_txView) throw std::runtime_error("No transaction view loaded.");

        TxSearchDialog dlg(m_txModel, m_parent);
        if (dlg.exec())
        {
            QString txhash = dlg.getTxHash();

            // TODO: faster search
            QStandardItem* hashItem = nullptr;
            int row = 0;
            for (; row < m_txModel->rowCount(); row++)
            {
                QStandardItem* item = m_txModel->item(row, 8);
                if (item->text().left(txhash.size()) == txhash)
                {
                    hashItem = item;
                    break;
                }
            }

            if (!hashItem) throw std::runtime_error("Transaction not found.");

            emit setCurrentWidget(m_txView);
            QItemSelection selection(m_txModel->index(row, 0), m_txModel->index(row, 0));//m_txModel->columnCount() - 1));
            m_txView->clearSelection();
            m_txView->scrollTo(hashItem->index(), QAbstractItemView::PositionAtCenter);
            m_txView->selectionModel()->select(selection, QItemSelectionModel::SelectCurrent);
        }
    }
    catch (const std::exception& e)
    {
        emit error(e.what());
    }
}

void TxActions::showSignatureDialog()
{
    try
    {
        SignatureDialog dlg(*m_synchedVault, m_txModel->getTxHash(currentRow), m_parent);
        connect(&dlg, &SignatureDialog::error, [this](const QString& msg) { emit error(msg); });

        if (m_txModel)
        {
            connect(&dlg, &SignatureDialog::txUpdated, [this]() { m_txModel->update(); });
        }

        if (m_keychainModel)
        {
            connect(&dlg, &SignatureDialog::keychainsUpdated, [this]() { m_keychainModel->update(); });
        }

        dlg.exec();
    }
    catch (const std::exception& e)
    {
        emit error(e.what());
    }
}

void TxActions::signTx()
{
    try
    {
        seedEntropySource(false);
        m_txModel->signTx(currentRow);
        m_txView->updateColumns();
        emit setCurrentWidget(m_txView);
    }
    catch (const std::exception& e)
    {
        emit error(e.what());
    }
}

void TxActions::sendTx()
{
    try {
        m_txModel->sendTx(currentRow, m_synchedVault);
        m_txView->updateColumns();
    }
    catch (const std::exception& e) {
        emit error(e.what());
    }
}

void TxActions::exportTxToFile()
{
    try {
        if (!m_synchedVault || !m_synchedVault->isVaultOpen()) throw std::runtime_error("You must create a vault or open an existing vault before exporting transactions.");

        std::shared_ptr<CoinDB::Tx> tx = m_txModel->getTx(currentRow);
        QString fileName;
        if (m_txModel->getTxOutType(currentRow) == TxModel::SEND)
        {
            fileName = QString::fromStdString(uchar_vector(tx->hash()).getHex()).left(16);
            CoinDB::SignatureInfo signatureInfo = m_synchedVault->getVault()->getSignatureInfo(tx->hash());
            for (auto& keychain: signatureInfo.signingKeychains())
            {
                fileName += keychain.hasSigned() ? "+" : "-";
                fileName += QString::fromStdString(keychain.name());
            }
        }
        else
        {
            fileName = QString::fromStdString(uchar_vector(tx->hash()).getHex());
        }
        fileName += ".tx";        
        fileName = QFileDialog::getSaveFileName(
            nullptr,
            tr("Export Transaction"),
            getDocDir() + "/" + fileName,
            tr("Transactions (*.tx)"));
        if (fileName.isEmpty()) return;

        fileName = QFileInfo(fileName).absoluteFilePath();

        QFileInfo fileInfo(fileName);
        setDocDir(fileInfo.dir().absolutePath());

        // TODO: emit settings changed signal

        m_accountModel->getVault()->exportTx(tx, fileName.toStdString()); 
    }
    catch (const std::exception& e) {
        emit error(e.what());
    }
}

void TxActions::exportAllTxsToFile()
{
    try {
        if (!m_synchedVault || !m_synchedVault->isVaultOpen()) throw std::runtime_error("You must create a vault or open an existing vault before exporting transactions.");

        CoinDB::Vault* vault = m_synchedVault->getVault();
        QString fileName = QString::fromStdString(vault->getName());

        // Strip .vault extension and add .txs extension
        if (fileName.endsWith(".vault", Qt::CaseInsensitive)) { fileName = fileName.left(fileName.size() - 6); }
        fileName += ".txs";

        fileName = QFileDialog::getSaveFileName(
            nullptr,
            tr("Export All Transactions"),
            getDocDir() + "/" + fileName,
            tr("Transaction Set (*.txs)"));
        if (fileName.isEmpty()) return;

        fileName = QFileInfo(fileName).absoluteFilePath();

        QFileInfo fileInfo(fileName);
        setDocDir(fileInfo.dir().absolutePath());

        // TODO: emit settings changed signal

        m_accountModel->getVault()->exportTxs(fileName.toStdString()); 
    }
    catch (const std::exception& e) {
        emit error(e.what());
    }
}

void TxActions::importTxFromFile()
{
    try {
        if (!m_accountModel || !m_accountModel->isOpen()) throw std::runtime_error("You must create a vault or open an existing vault before importing transactions.");

        QString fileName = QFileDialog::getOpenFileName(
            nullptr,
            tr("Import Transaction"),
            getDocDir(),
            tr("Transactions (*.tx)"));
        if (fileName.isEmpty()) return;

        fileName = QFileInfo(fileName).absoluteFilePath();

        QFileInfo fileInfo(fileName);
        setDocDir(fileInfo.dir().absolutePath());

        // TODO: emit settings changed signal

        std::shared_ptr<CoinDB::Tx> tx = m_accountModel->getVault()->importTx(fileName.toStdString());
        if (!tx) throw std::runtime_error("Transaction not inserted.");
        m_accountModel->update();
        m_txModel->update();
        m_txView->updateColumns();
        emit setCurrentWidget(m_txView);
    }
    catch (const std::exception& e) {
        emit error(e.what());
    } 
}

void TxActions::importTxFromClipboard()
{
    try
    {
        if (!m_accountModel || !m_accountModel->isOpen()) throw std::runtime_error("You must create a vault or open an existing vault before importing transactions.");

        QClipboard* clipboard = QApplication::clipboard();
        std::string serializedTx = clipboard->text().toStdString();

        std::shared_ptr<CoinDB::Tx> tx = m_accountModel->getVault()->importTxFromString(serializedTx);
        if (!tx) throw std::runtime_error("Transaction not inserted.");
        m_accountModel->update();
        m_txModel->update();
        m_txView->updateColumns();
        emit setCurrentWidget(m_txView);
    }
    catch (const std::exception& e)
    {
        emit error(e.what());
    } 
}

void TxActions::importTxsFromFile()
{
    try
    {
        if (!m_accountModel || !m_accountModel->isOpen()) throw std::runtime_error("You must create a vault or open an existing vault before importing transactions.");

        QString fileName = QFileDialog::getOpenFileName(
            nullptr,
            tr("Import Transactions"),
            getDocDir(),
            tr("Transaction Set (*.txs)"));
        if (fileName.isEmpty()) return;

        fileName = QFileInfo(fileName).absoluteFilePath();

        QFileInfo fileInfo(fileName);
        setDocDir(fileInfo.dir().absolutePath());

        // TODO: emit settings changed signal

        if (m_accountModel->getVault()->importTxs(fileName.toStdString()) == 0) throw std::runtime_error("No transactions imported.");
        m_accountModel->update();
        m_txModel->update();
        m_txView->updateColumns();
        emit setCurrentWidget(m_txView);
    }
    catch (const std::exception& e) {
        emit error(e.what());
    } 
}

void TxActions::viewRawTx()
{
    try {
        std::shared_ptr<CoinDB::Tx> tx = m_txModel->getTx(currentRow);
        RawTxDialog rawTxDlg(tr("Raw Transaction"));
        rawTxDlg.setRawTx(tx->raw());
        rawTxDlg.exec();
    }
    catch (const std::exception& e) {
        emit error(e.what());
    }
}

void TxActions::copyAddressToClipboard()
{
    try {
        QString address = m_txModel->getTxOutAddress(currentRow);
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText(address);
    }
    catch (const std::exception& e) {
        emit error(e.what());
    }
}

void TxActions::copyTxHashToClipboard()
{
    try {
        std::shared_ptr<CoinDB::Tx> tx = m_txModel->getTx(currentRow);
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText(QString::fromStdString(uchar_vector(tx->hash()).getHex()));
    }
    catch (const std::exception& e) {
        emit error(e.what());
    }
}

void TxActions::copyRawTxToClipboard()
{
    try {
        std::shared_ptr<CoinDB::Tx> tx = m_txModel->getTx(currentRow);
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText(QString::fromStdString(uchar_vector(tx->raw()).getHex()));
    }
    catch (const std::exception& e) {
        emit error(e.what());
    }
}

void TxActions::saveRawTxToFile()
{
    try {
        std::shared_ptr<CoinDB::Tx> tx = m_txModel->getTx(currentRow);
        QString fileName = QString::fromStdString(uchar_vector(tx->hash()).getHex()) + ".rawtx";
        fileName = QFileDialog::getSaveFileName(
            nullptr,
            tr("Save Raw Transaction"),
            getDocDir() + "/" + fileName,
            tr("Transactions (*.rawtx)"));
        if (fileName.isEmpty()) return;

        fileName = QFileInfo(fileName).absoluteFilePath();

        QFileInfo fileInfo(fileName);
        setDocDir(fileInfo.dir().absolutePath());

        // TODO: emit settings changed signal

        std::ofstream ofs(fileName.toStdString(), std::ofstream::out);
        ofs << uchar_vector(tx->raw()).getHex() << std::endl;
        ofs.close();
    }
    catch (const std::exception& e) {
        emit error(e.what());
    }
}

void TxActions::insertRawTxFromClipboard()
{
    try {
        if (!m_accountModel || !m_accountModel->isOpen()) throw std::runtime_error("You must create a vault or open an existing vault before inserting transactions.");

        QClipboard* clipboard = QApplication::clipboard();
        std::string rawhex = clipboard->text().toStdString();

        std::shared_ptr<CoinDB::Tx> tx(new CoinDB::Tx());
        tx->set(uchar_vector(rawhex));
        tx = m_accountModel->insertTx(tx);
        m_txModel->update();
        m_txView->updateColumns();
        emit setCurrentWidget(m_txView);
    }
    catch (const std::exception& e) {
        emit error(e.what());
    } 
}

void TxActions::insertRawTxFromFile()
{
    try {
        if (!m_accountModel || !m_accountModel->isOpen()) throw std::runtime_error("You must create a vault or open an existing vault before inserting transactions.");

        QString fileName = QFileDialog::getOpenFileName(
            nullptr,
            tr("Insert Raw Transaction"),
            getDocDir(),
            tr("Transactions (*.rawtx)"));
        if (fileName.isEmpty()) return;

        fileName = QFileInfo(fileName).absoluteFilePath();

        QFileInfo fileInfo(fileName);
        setDocDir(fileInfo.dir().absolutePath());

        // TODO: emit settings changed signal

        std::string rawhex;
        std::ifstream ifs(fileName.toStdString(), std::ifstream::in);
        if (ifs.bad()) throw std::runtime_error("Error opening file.");
        ifs >> rawhex;
        if (!ifs.good()) throw std::runtime_error("Error reading file.");

        std::shared_ptr<CoinDB::Tx> tx(new CoinDB::Tx());
        tx->set(uchar_vector(rawhex));
        tx = m_accountModel->insertTx(tx);
        m_txModel->update();
        m_txView->updateColumns();
        emit setCurrentWidget(m_txView);
    }
    catch (const std::exception& e) {
        emit error(e.what());
    } 
}

void TxActions::viewTxOnWeb()
{
    const QString URL_PREFIX("https://blockchain.info/tx/");

    try {
        std::shared_ptr<CoinDB::Tx> tx = m_txModel->getTx(currentRow);
        if (!QDesktopServices::openUrl(QUrl(URL_PREFIX + QString::fromStdString(uchar_vector(tx->hash()).getHex())))) {
            throw std::runtime_error(tr("Unable to open browser.").toStdString());
        }
    }
    catch (const std::exception& e) {
        emit error(e.what());
    }
}

void TxActions::deleteTx()
{
    QMessageBox msgBox;
    msgBox.setText(tr("Delete confirmation."));
    msgBox.setInformativeText(tr("Are you sure you want to delete this transaction?"));
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    if (msgBox.exec() == QMessageBox::Cancel) return;

    try {
        m_txModel->deleteTx(currentRow);
        m_txView->updateColumns();
        emit txsChanged();
        //m_accountModel->update();
    }
    catch (const std::exception& e) {
        emit error(e.what());
    }
}

void TxActions::createActions()
{
    searchTxAction = new QAction(QIcon(":/icons/search_32x32.png"), tr("Search For Transaction..."), this);
    searchTxAction->setEnabled(false);
    connect(searchTxAction, SIGNAL(triggered()), this, SLOT(searchTx()));

    signaturesAction = new QAction(QIcon(":/icons/glossy-black-signature_32x32.png"), tr("Signatures..."), this);
    signaturesAction->setEnabled(false);
    connect(signaturesAction, SIGNAL(triggered()), this, SLOT(showSignatureDialog()));

    signTxAction = new QAction(tr("Sign Transaction"), this);
    signTxAction->setEnabled(false);
    connect(signTxAction, SIGNAL(triggered()), this, SLOT(signTx()));

    sendTxAction = new QAction(QIcon(":/icons/send_32x32.png"), tr("Send Transaction"), this);
    sendTxAction->setEnabled(false);
    connect(sendTxAction, SIGNAL(triggered()), this, SLOT(sendTx()));

    exportTxToFileAction = new QAction(tr("To File..."), this);
    exportTxToFileAction->setEnabled(false);
    connect(exportTxToFileAction, SIGNAL(triggered()), this, SLOT(exportTxToFile()));

    importTxFromFileAction = new QAction(tr("From File..."), this);
    importTxFromFileAction->setEnabled(false);
    connect(importTxFromFileAction, SIGNAL(triggered()), this, SLOT(importTxFromFile()));

    importTxFromClipboardAction = new QAction(tr("From Clipboard"), this);
    importTxFromClipboardAction->setEnabled(false);
    connect(importTxFromClipboardAction, SIGNAL(triggered()), this, SLOT(importTxFromClipboard()));

    exportAllTxsToFileAction = new QAction(tr("All To File..."), this);
    exportAllTxsToFileAction->setEnabled(false);
    connect(exportAllTxsToFileAction, SIGNAL(triggered()), this, SLOT(exportAllTxsToFile()));

    importTxsFromFileAction = new QAction(tr("Multiple From File..."), this);
    importTxsFromFileAction->setEnabled(false);
    connect(importTxsFromFileAction, SIGNAL(triggered()), this, SLOT(importTxsFromFile()));

    viewRawTxAction = new QAction(tr("View Raw Transaction"), this);
    viewRawTxAction->setEnabled(false);
    connect(viewRawTxAction, SIGNAL(triggered()), this, SLOT(viewRawTx()));

    copyAddressToClipboardAction = new QAction(tr("Copy Address To Clipboard"), this);
    copyAddressToClipboardAction->setEnabled(false);
    connect(copyAddressToClipboardAction, SIGNAL(triggered()), this, SLOT(copyAddressToClipboard()));

    copyTxHashToClipboardAction = new QAction(tr("Copy Transaction Hash To Clipboard"), this);
    copyTxHashToClipboardAction->setEnabled(false);
    connect(copyTxHashToClipboardAction, SIGNAL(triggered()), this, SLOT(copyTxHashToClipboard()));

    copyRawTxToClipboardAction = new QAction(tr("To Clipboard (raw)"), this);
    copyRawTxToClipboardAction->setEnabled(false);
    connect(copyRawTxToClipboardAction, SIGNAL(triggered()), this, SLOT(copyRawTxToClipboard()));

    insertRawTxFromClipboardAction = new QAction(tr("From Clipboard (raw)"), this);
    insertRawTxFromClipboardAction->setEnabled(false);
    connect(insertRawTxFromClipboardAction, SIGNAL(triggered()), this, SLOT(insertRawTxFromClipboard()));

    saveRawTxToFileAction = new QAction(tr("To File (raw)..."), this);
    saveRawTxToFileAction->setEnabled(false);
    connect(saveRawTxToFileAction, SIGNAL(triggered()), this, SLOT(saveRawTxToFile()));

    insertRawTxFromFileAction = new QAction(tr("From File (raw)..."), this);
    insertRawTxFromFileAction->setEnabled(false);
    connect(insertRawTxFromFileAction, SIGNAL(triggered()), this, SLOT(insertRawTxFromFile()));

    viewTxOnWebAction = new QAction(tr("View At Blockchain.info"), this);
    viewTxOnWebAction->setEnabled(false);
    connect(viewTxOnWebAction, SIGNAL(triggered()), this, SLOT(viewTxOnWeb()));

    deleteTxAction = new QAction(tr("Delete Transaction"), this);
    deleteTxAction->setEnabled(false);
    connect(deleteTxAction, SIGNAL(triggered()), this, SLOT(deleteTx()));
}

void TxActions::createMenus()
{
    menu = new QMenu(tr("&Transactions"));
    menu->addAction(signaturesAction);
    menu->addAction(sendTxAction);

    menu->addSeparator();
    //menu->addAction(signTxAction);

    QMenu* importTxMenu = menu->addMenu(tr("Import Transaction"));
    importTxMenu->addAction(importTxFromClipboardAction);
    importTxMenu->addAction(importTxFromFileAction);
    importTxMenu->addAction(importTxsFromFileAction);
    importTxMenu->addSeparator();
    importTxMenu->addAction(insertRawTxFromClipboardAction);
    importTxMenu->addAction(insertRawTxFromFileAction);

    QMenu* exportTxMenu = menu->addMenu(tr("Export Transaction"));
    exportTxMenu->addAction(exportTxToFileAction);
    exportTxMenu->addAction(exportAllTxsToFileAction);
    exportTxMenu->addSeparator();
    exportTxMenu->addAction(copyRawTxToClipboardAction);
    exportTxMenu->addAction(saveRawTxToFileAction);

    menu->addSeparator();

    //menu->addAction(exportTxToFileAction);
    //menu->addAction(importTxFromFileAction);
    //menu->addAction(importTxFromClipboardAction);
    //menu->addSeparator();
    //menu->addAction(exportAllTxsToFileAction);
    //menu->addAction(importTxsFromFileAction);
    //menu->addSeparator();
    //menu->addAction(viewRawTxAction);
    menu->addAction(copyAddressToClipboardAction);
    menu->addAction(copyTxHashToClipboardAction);
    //menu->addAction(copyRawTxToClipboardAction);
    //menu->addAction(insertRawTxFromClipboardAction);
    //menu->addSeparator();
    //menu->addAction(saveRawTxToFileAction);
    //menu->addAction(insertRawTxFromFileAction);
    menu->addSeparator();
    menu->addAction(viewTxOnWebAction);
    menu->addSeparator();
    menu->addAction(searchTxAction);
    menu->addSeparator();
    menu->addAction(deleteTxAction);
}

