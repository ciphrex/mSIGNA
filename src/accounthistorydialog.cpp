///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// accounthistorydialog.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "settings.h"

#include "accounthistorydialog.h"

// Models/Views
#include "txmodel.h"
#include "txview.h"

// Subdialogs
#include "rawtxdialog.h"

#include <QAction>
#include <QMenu>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QUrl>
#include <QDesktopServices>

#include <CoinDB/Vault.h>
#include <CoinQ/CoinQ_netsync.h>

AccountHistoryDialog::AccountHistoryDialog(CoinDB::Vault* vault, const QString& accountName, CoinQ::Network::NetworkSync* networkSync, QWidget* parent)
    : QDialog(parent), currentRow(-1)
{
    resize(QSize(800, 400));

    createActions();
    createMenus();

    accountHistoryModel = new TxModel(vault, accountName, this);
    accountHistoryModel->update();

    accountHistoryView = new TxView(this);
    accountHistoryView->setModel(accountHistoryModel);
    accountHistoryView->setMenu(menu);
    accountHistoryView->update();

    txSelectionModel = accountHistoryView->selectionModel();
    connect(txSelectionModel, &QItemSelectionModel::currentChanged,
            this, &AccountHistoryDialog::updateCurrentTx);
/*
    connect(txSelectionModel, &QItemSelectionModel::selectionChanged,
            this, &AccountHistoryDialog::updateSelectedTxs);
*/

    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->addWidget(accountHistoryView);
    setLayout(mainLayout);
    setWindowTitle(getDefaultSettings().getAppName() + " - " + accountName + tr(" account history"));

    this->networkSync = networkSync;
}

void AccountHistoryDialog::updateCurrentTx(const QModelIndex& current, const QModelIndex& /*previous*/)
{
    currentRow = current.row();
    if (currentRow != -1) {
        QStandardItem* typeItem = accountHistoryModel->item(currentRow, 6);
        int type = typeItem->data(Qt::UserRole).toInt();
        if (type == CoinDB::Tx::UNSIGNED) {
            signTxAction->setEnabled(true);
        }
        else {
            signTxAction->setEnabled(false);
        }

        if (networkSync && networkSync->connected() && type == CoinDB::Tx::UNSENT) {
            sendTxAction->setEnabled(true);
        }
        else {
            sendTxAction->setEnabled(false);
        }

        if (type == CoinDB::Tx::PROPAGATED) {
            viewTxOnWebAction->setEnabled(true);
        }
        else {
            viewTxOnWebAction->setEnabled(false);
        }

        viewRawTxAction->setEnabled(true);
        deleteTxAction->setEnabled(true);
        return;
    }
    signTxAction->setEnabled(false);
    viewRawTxAction->setEnabled(false);
    viewTxOnWebAction->setEnabled(false);
    deleteTxAction->setEnabled(false);
}

void AccountHistoryDialog::signTx()
{
    try {
        accountHistoryModel->signTx(currentRow);
        accountHistoryView->update();
    }
    catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Error"), e.what());
    }
}

void AccountHistoryDialog::sendTx()
{
    try {
        throw std::runtime_error("Unsupported operation.");
        //accountHistoryModel->sendTx(currentRow, networkSync);
        //accountHistoryView->update();
    }
    catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Error"), e.what());
    }
}

void AccountHistoryDialog::viewRawTx()
{
    try {
        std::shared_ptr<CoinDB::Tx> tx = accountHistoryModel->getTx(currentRow);
        RawTxDialog rawTxDlg(tr("Raw Transaction"));
        rawTxDlg.setRawTx(tx->raw());
        rawTxDlg.exec();
    }
    catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Error"), e.what());
    }
}

void AccountHistoryDialog::viewTxOnWeb()
{
    const QString URL_PREFIX("https://blockchain.info/tx/");

    try {
        std::shared_ptr<CoinDB::Tx> tx = accountHistoryModel->getTx(currentRow);
        if (!QDesktopServices::openUrl(QUrl(URL_PREFIX + QString::fromStdString(uchar_vector(tx->hash()).getHex())))) {
            throw std::runtime_error(tr("Unable to open browser.").toStdString());
        }
    }
    catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Error"), e.what());
    }
}

void AccountHistoryDialog::deleteTx()
{
    try {
        accountHistoryModel->deleteTx(currentRow);
        accountHistoryView->update();
        emit txDeleted();
    }
    catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Error"), e.what());
    } 
}

void AccountHistoryDialog::createActions()
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

    viewTxOnWebAction = new QAction(tr("View At Blockchain.info"), this);
    viewTxOnWebAction->setEnabled(false);
    connect(viewTxOnWebAction, SIGNAL(triggered()), this, SLOT(viewTxOnWeb()));

    deleteTxAction = new QAction(tr("Delete Transaction"), this);
    deleteTxAction->setEnabled(false);
    connect(deleteTxAction, SIGNAL(triggered()), this, SLOT(deleteTx()));
}

void AccountHistoryDialog::createMenus()
{
    menu = new QMenu();
    menu->addAction(signTxAction);
    menu->addAction(sendTxAction);
    menu->addAction(viewRawTxAction);
    menu->addAction(viewTxOnWebAction);
    menu->addAction(deleteTxAction);
}
