///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// txactions.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include "txactions.h"

#include "txmodel.h"
#include "txview.h"

#include "rawtxdialog.h"

#include <CoinQ_netsync.h>

#include <QAction>
#include <QMenu>
#include <QUrl>
#include <QDesktopServices>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>

TxActions::TxActions(TxModel* model, TxView* view, CoinQ::Network::NetworkSync* sync)
    : txModel(model), txView(view), networkSync(sync), currentRow(-1)
{
    createActions();
    createMenus();
    connect(txView->selectionModel(), &QItemSelectionModel::currentChanged, this, &TxActions::updateCurrentTx);
}

void TxActions::updateCurrentTx(const QModelIndex& current, const QModelIndex& /*previous*/)
{
    currentRow = current.row();
    if (txModel && currentRow != -1) {
        QStandardItem* typeItem = txModel->item(currentRow, 6);
        int type = typeItem->data(Qt::UserRole).toInt();
        if (type == CoinDB::Tx::UNSIGNED) {
            signTxAction->setEnabled(true);
        }
        else {
            signTxAction->setEnabled(false);
        }

        if (type == CoinDB::Tx::UNSENT) {
            sendTxAction->setText(tr("Send Transaction"));
            sendTxAction->setEnabled(networkSync && networkSync->isConnected());
        }
        else {
            sendTxAction->setText(tr("Resend Transaction"));
            sendTxAction->setEnabled(networkSync && networkSync->isConnected() && typeItem->text() == "0");
        }

        if (type == CoinDB::Tx::PROPAGATED) {
            viewTxOnWebAction->setEnabled(true);
        }
        else {
            viewTxOnWebAction->setEnabled(false);
        }

        copyTxIDToClipboardAction->setEnabled(true);
        copyRawTxToClipboardAction->setEnabled(true);
        viewRawTxAction->setEnabled(true);
        deleteTxAction->setEnabled(true);
    }
    else {
        signTxAction->setEnabled(false);
        sendTxAction->setEnabled(false);
        copyTxIDToClipboardAction->setEnabled(false);
        copyRawTxToClipboardAction->setEnabled(false);
        viewRawTxAction->setEnabled(false);
        viewTxOnWebAction->setEnabled(false);
        deleteTxAction->setEnabled(false);
    }
}

void TxActions::signTx()
{
    try {
        txModel->signTx(currentRow);
        txView->update();
    }
    catch (const std::exception& e) {
        emit error(e.what());
    }
}

void TxActions::sendTx()
{
    try {
        txModel->sendTx(currentRow, networkSync);
        txView->update();
    }
    catch (const std::exception& e) {
        emit error(e.what());
    }
}

void TxActions::viewRawTx()
{
    try {
        std::shared_ptr<CoinDB::Tx> tx = txModel->getTx(currentRow);
        RawTxDialog rawTxDlg(tr("Raw Transaction"));
        rawTxDlg.setRawTx(tx->raw());
        rawTxDlg.exec();
    }
    catch (const std::exception& e) {
        emit error(e.what());
    }
}

void TxActions::copyTxIDToClipboard()
{
    try {
        std::shared_ptr<CoinDB::Tx> tx = txModel->getTx(currentRow);
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
        std::shared_ptr<CoinDB::Tx> tx = txModel->getTx(currentRow);
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText(QString::fromStdString(uchar_vector(tx->raw()).getHex()));
    }
    catch (const std::exception& e) {
        emit error(e.what());
    }
}

void TxActions::viewTxOnWeb()
{
    const QString URL_PREFIX("https://blockchain.info/tx/");

    try {
        std::shared_ptr<CoinDB::Tx> tx = txModel->getTx(currentRow);
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
        txModel->deleteTx(currentRow);
        txView->update();
    }
    catch (const std::exception& e) {
        emit error(e.what());
    }
}

void TxActions::createActions()
{
    signTxAction = new QAction(tr("Sign Transaction"), this);
    signTxAction->setEnabled(false);
    connect(signTxAction, SIGNAL(triggered()), this, SLOT(signTx()));

    sendTxAction = new QAction(tr("Send Transaction"), this);
    sendTxAction->setEnabled(false);
    connect(sendTxAction, SIGNAL(triggered()), this, SLOT(sendTx()));

    viewRawTxAction = new QAction(tr("View Raw Transaction"), this);
    viewRawTxAction->setEnabled(false);
    connect(viewRawTxAction, SIGNAL(triggered()), this, SLOT(viewRawTx()));

    copyTxIDToClipboardAction = new QAction(tr("Copy Transaction ID To Clipboard"), this);
    copyTxIDToClipboardAction->setEnabled(false);
    connect(copyTxIDToClipboardAction, SIGNAL(triggered()), this, SLOT(copyTxIDToClipboard()));

    copyRawTxToClipboardAction = new QAction(tr("Copy Raw Transaction To Clipboard"), this);
    copyRawTxToClipboardAction->setEnabled(false);
    connect(copyRawTxToClipboardAction, SIGNAL(triggered()), this, SLOT(copyRawTxToClipboard()));

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
    menu->addAction(signTxAction);
    menu->addAction(sendTxAction);
    menu->addSeparator();
    //menu->addAction(viewRawTxAction);
    menu->addAction(copyTxIDToClipboardAction);
    menu->addAction(copyRawTxToClipboardAction);
    menu->addAction(viewTxOnWebAction);
    menu->addSeparator();
    menu->addAction(deleteTxAction);
}

