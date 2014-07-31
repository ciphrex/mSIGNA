///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// txactions.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#include "txactions.h"

#include "txmodel.h"
#include "txview.h"
#include "accountmodel.h"

#include "rawtxdialog.h"
#include "signaturedialog.h"

#include "docdir.h"

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

TxActions::TxActions(TxModel* txModel, TxView* txView, AccountModel* accountModel, CoinDB::SynchedVault* synchedVault)
    : m_txModel(txModel), m_txView(txView), m_accountModel(accountModel), m_synchedVault(synchedVault), currentRow(-1)
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

        if (type == CoinDB::Tx::PROPAGATED || type == CoinDB::Tx::CONFIRMED) {
            viewTxOnWebAction->setEnabled(true);
        }
        else {
            viewTxOnWebAction->setEnabled(false);
        }

        copyTxHashToClipboardAction->setEnabled(true);
        copyRawTxToClipboardAction->setEnabled(true);
        saveRawTxToFileAction->setEnabled(true);
        viewRawTxAction->setEnabled(true);
        deleteTxAction->setEnabled(true);
    }
    else {
        signTxAction->setEnabled(false);
        sendTxAction->setEnabled(false);
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
    insertRawTxFromFileAction->setEnabled(m_accountModel && m_accountModel->isOpen());
}

void TxActions::showSignatureDialog()
{
    try
    {
        SignatureDialog dlg(m_txModel->getVault(), m_txModel->getTxHash(currentRow));
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
        m_txModel->signTx(currentRow);
        m_txView->update();
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
        m_txView->update();
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
        QString fileName = QString::fromStdString(uchar_vector(tx->hash()).getHex()) + ".tx";
        fileName = QFileDialog::getSaveFileName(
            nullptr,
            tr("Save Raw Transaction"),
            getDocDir() + "/" + fileName,
            tr("Transactions (*.tx)"));
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

void TxActions::insertRawTxFromFile()
{
    try {
        if (!m_accountModel || !m_accountModel->isOpen()) throw std::runtime_error("You must create a vault or open an existing vault before inserting transactions.");

        QString fileName = QFileDialog::getOpenFileName(
            nullptr,
            tr("Insert Raw Transaction"),
            getDocDir(),
            tr("Transactions (*.tx)"));
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
        m_txView->update();
    }
    catch (const std::exception& e) {
        emit error(e.what());
    }
}

void TxActions::createActions()
{
    signaturesAction = new QAction(tr("Signatures..."), this);
    signaturesAction->setEnabled(false);
    connect(signaturesAction, SIGNAL(triggered()), this, SLOT(showSignatureDialog()));

    signTxAction = new QAction(tr("Sign Transaction"), this);
    signTxAction->setEnabled(false);
    connect(signTxAction, SIGNAL(triggered()), this, SLOT(signTx()));

    sendTxAction = new QAction(tr("Send Transaction"), this);
    sendTxAction->setEnabled(false);
    connect(sendTxAction, SIGNAL(triggered()), this, SLOT(sendTx()));

    viewRawTxAction = new QAction(tr("View Raw Transaction"), this);
    viewRawTxAction->setEnabled(false);
    connect(viewRawTxAction, SIGNAL(triggered()), this, SLOT(viewRawTx()));

    copyTxHashToClipboardAction = new QAction(tr("Copy Transaction Hash To Clipboard"), this);
    copyTxHashToClipboardAction->setEnabled(false);
    connect(copyTxHashToClipboardAction, SIGNAL(triggered()), this, SLOT(copyTxHashToClipboard()));

    copyRawTxToClipboardAction = new QAction(tr("Copy Raw Transaction To Clipboard"), this);
    copyRawTxToClipboardAction->setEnabled(false);
    connect(copyRawTxToClipboardAction, SIGNAL(triggered()), this, SLOT(copyRawTxToClipboard()));

    saveRawTxToFileAction = new QAction(tr("Save Raw Transaction To File"), this);
    saveRawTxToFileAction->setEnabled(false);
    connect(saveRawTxToFileAction, SIGNAL(triggered()), this, SLOT(saveRawTxToFile()));

    insertRawTxFromFileAction = new QAction(tr("Insert Raw Transaction From File"), this);
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
    menu = new QMenu();
    menu->addAction(signaturesAction);
    menu->addAction(signTxAction);
    menu->addAction(sendTxAction);
    menu->addSeparator();
    //menu->addAction(viewRawTxAction);
    menu->addAction(copyTxHashToClipboardAction);
    menu->addAction(copyRawTxToClipboardAction);
    menu->addAction(saveRawTxToFileAction);
    menu->addAction(insertRawTxFromFileAction);
    menu->addAction(viewTxOnWebAction);
    menu->addSeparator();
    menu->addAction(deleteTxAction);
}

