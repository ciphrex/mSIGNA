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

SignatureActions::SignatureActions(CoinDB::SynchedVault& synchedVault, SignatureDialog& dialog)
    : m_synchedVault(synchedVault), m_dialog(dialog)
{
    createActions();
    createMenus();
    connect(m_dialog.getView()->selectionModel(), &QItemSelectionModel::currentChanged, this, &SignatureActions::updateCurrentKeychain);
}

void SignatureActions::updateCurrentKeychain(const QModelIndex& current, const QModelIndex& /*previous*/)
{
    int currentRow = current.row();
    if (currentRow != -1)
    {
        QStandardItem* keychainNameItem = m_dialog.getModel()->item(currentRow, 0);
        m_currentKeychain = keychainNameItem->text();

        QStandardItem* typeItem = m_dialog.getModel()->item(currentRow, 2);
        addSignatureAction->setEnabled(m_dialog.getModel()->getSigsNeeded() > 0 && typeItem->text() == tr("No")
            && m_synchedVault.isVaultOpen() && !m_synchedVault.getVault()->isKeychainPrivateKeyLocked(m_currentKeychain.toStdString()));
    }
    else
    {
        m_currentKeychain = "";
        addSignatureAction->setEnabled(false);
    }

}

void SignatureActions::addSignature()
{
    try
    {
        if (m_currentKeychain.isEmpty()) throw std::runtime_error("No keychain is selected.");
        if (!m_synchedVault.isVaultOpen()) throw std::runtime_error("No vault is open.");

        CoinDB::Vault* vault = m_synchedVault.getVault();
        if (vault->isKeychainPrivateKeyLocked(m_currentKeychain.toStdString()))
        {
            QMessageBox::critical(nullptr, tr("Error"), tr("Keychain is locked."));
        }

        std::vector<std::string> keychainNames;
        keychainNames.push_back(m_currentKeychain.toStdString());
        vault->signTx(m_dialog.getModel()->getTxHash(), keychainNames, true);

        if (keychainNames.empty())
        {
            QMessageBox::critical(nullptr, tr("Error"), tr("Signature could not be added."));
            return;
        }

        m_dialog.getModel()->update();
        emit m_dialog.txUpdated();
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

