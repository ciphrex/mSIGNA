///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// signatureactions.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#include "signatureactions.h"

#include "signaturemodel.h"
#include "signatureview.h"
#include "signaturedialog.h"

#include <CoinDB/SynchedVault.h>

#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>

#include <logger/logger.h>

SignatureActions::SignatureActions(CoinDB::SynchedVault& synchedVault, SignatureModel& signatureModel, SignatureView& signatureView)
    : m_synchedVault(synchedVault), m_signatureModel(signatureModel), m_signatureView(signatureView)
{
    createActions();
    createMenus();
    connect(m_signatureView.selectionModel(), &QItemSelectionModel::currentChanged, this, &SignatureActions::updateCurrentKeychain);
}

void SignatureActions::updateCurrentKeychain(const QModelIndex& current, const QModelIndex& /*previous*/)
{
    int currentRow = current.row();
    if (currentRow != -1)
    {
        QStandardItem* keychainNameItem = m_signatureModel.item(currentRow, 0);
        currentKeychain = keychainNameItem->text();

        QStandardItem* typeItem = m_signatureModel.item(currentRow, 2);
        addSignatureAction->setEnabled(typeItem->text() == tr("Yes"));
    }
    else
    {
        currentKeychain = "";
        addSignatureAction->setEnabled(false);
    }

}

void SignatureActions::addSignature()
{
    try
    {
        if (currentKeychain.isEmpty()) throw std::runtime_error("No keychain is selected.");
        if (!m_synchedVault.isVaultOpen()) throw std::runtime_error("No vault is open.");

        CoinDB::Vault* vault = m_synchedVault.getVault();

        std::vector<std::string> keychainNames;
        keychainNames.push_back(currentKeychain.toStdString());
        vault->signTx(m_signatureModel.getTxHash(), keychainNames, true);

        m_signatureModel.update();
    }
    catch (const std::exception& e)
    {
        emit error(e.what());
    }
}

void SignatureActions::createActions()
{
    addSignatureAction = new QAction(tr("Add signature"), this);
    addSignatureAction->setEnabled(false);
    connect(addSignatureAction, SIGNAL(triggered()), this, SLOT(addSignature()));
}

void SignatureActions::createMenus()
{
    menu = new QMenu();
    menu->addAction(addSignatureAction);
}

