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

#include "entropysource.h"

#include <CoinDB/SynchedVault.h>

#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>

#include <logger/logger.h>

SignatureActions::SignatureActions(CoinDB::SynchedVault& synchedVault, SignatureDialog& dialog)
    : m_synchedVault(synchedVault), m_dialog(dialog), m_currentRow(-1)
{
    createActions();
    createMenus();
    connect(m_dialog.getView()->selectionModel(), &QItemSelectionModel::currentChanged, this, &SignatureActions::updateCurrentKeychain);
}

SignatureActions::~SignatureActions()
{
    delete addSignatureAction;
    delete unlockKeychainAction;
    delete lockKeychainAction;
    delete menu;
}

void SignatureActions::updateCurrentKeychain(const QModelIndex& current, const QModelIndex& /*previous*/)
{
    m_currentRow = current.row();
    refreshCurrentKeychain();
}

void SignatureActions::refreshCurrentKeychain()
{
    if (m_currentRow != -1)
    {
        QStandardItem* keychainNameItem = m_dialog.getModel()->item(m_currentRow, 0);
        m_currentKeychain = keychainNameItem->text();

        if (m_synchedVault.isVaultOpen())
        {
            int keychainState = m_dialog.getModel()->getKeychainState(m_currentRow);
            bool hasSigned = m_dialog.getModel()->getKeychainHasSigned(m_currentRow);
            int sigsNeeded = m_dialog.getModel()->getSigsNeeded();

            addSignatureAction->setEnabled(keychainState != SignatureModel::PUBLIC && !hasSigned && sigsNeeded > 0);
            unlockKeychainAction->setEnabled(keychainState == SignatureModel::LOCKED);
            lockKeychainAction->setEnabled(keychainState == SignatureModel::UNLOCKED);
            return;
        }
    }
    else
    {
        m_currentKeychain = "";
    }

    addSignatureAction->setEnabled(false);
    unlockKeychainAction->setEnabled(false);
    lockKeychainAction->setEnabled(false);
}

void SignatureActions::addSignature()
{
    try
    {
        if (m_currentKeychain.isEmpty()) throw std::runtime_error("No keychain is selected.");
        if (!m_synchedVault.isVaultOpen()) throw std::runtime_error("No vault is open.");

        CoinDB::Vault* vault = m_synchedVault.getVault();
        if (!vault->isKeychainPrivate(m_currentKeychain.toStdString()))
        {
            QMessageBox::critical(nullptr, tr("Error"), tr("Keychain is public."));
            return;
        }

        if (vault->isKeychainPrivateKeyLocked(m_currentKeychain.toStdString()))
        {
            unlockKeychain();
        }

        std::vector<std::string> keychainNames;
        keychainNames.push_back(m_currentKeychain.toStdString());

        seedEntropySource(false, &m_dialog);
        vault->signTx(m_dialog.getModel()->getTxHash(), keychainNames, true);

        if (keychainNames.empty())
        {
            QMessageBox::critical(nullptr, tr("Error"), tr("Signature could not be added."));
            return;
        }

        m_dialog.updateTx();
    }
    catch (const std::exception& e)
    {
        emit error(e.what());
    }
}

void SignatureActions::unlockKeychain()
{
    try
    {
        if (m_currentKeychain.isEmpty()) throw std::runtime_error("No keychain is selected.");
        if (!m_synchedVault.isVaultOpen()) throw std::runtime_error("No vault is open.");

        CoinDB::Vault* vault = m_synchedVault.getVault();
        std::string keychainName = m_currentKeychain.toStdString();
        if (!vault->isKeychainPrivateKeyLocked(keychainName))
        {
            QMessageBox::critical(nullptr, tr("Error"), tr("Keychain is already unlocked."));
            return;
        }

        // TODO: Prompt for passphrase
        vault->unlockKeychain(keychainName, secure_bytes_t());
        refreshCurrentKeychain();
        m_dialog.updateKeychains();
    }
    catch (const std::exception& e)
    {
        emit error(e.what());
    }
}

void SignatureActions::lockKeychain()
{
    try
    {
        if (m_currentKeychain.isEmpty()) throw std::runtime_error("No keychain is selected.");
        if (!m_synchedVault.isVaultOpen()) throw std::runtime_error("No vault is open.");

        CoinDB::Vault* vault = m_synchedVault.getVault();
        std::string keychainName = m_currentKeychain.toStdString();
        if (vault->isKeychainPrivateKeyLocked(keychainName))
        {
            QMessageBox::critical(nullptr, tr("Error"), tr("Keychain is already locked."));
            return;
        }

        vault->lockKeychain(keychainName);
        refreshCurrentKeychain();
        m_dialog.updateKeychains();
    }
    catch (const std::exception& e)
    {
        emit error(e.what());
    }
}

void SignatureActions::createActions()
{
    addSignatureAction = new QAction(tr("Add signature..."), this);
    addSignatureAction->setEnabled(false);
    connect(addSignatureAction, SIGNAL(triggered()), this, SLOT(addSignature()));

    unlockKeychainAction = new QAction(tr("Unlock keychain..."), this);
    unlockKeychainAction->setEnabled(false);
    connect(unlockKeychainAction, SIGNAL(triggered()), this, SLOT(unlockKeychain()));

    lockKeychainAction = new QAction(tr("Lock keychain"), this);
    lockKeychainAction->setEnabled(false);
    connect(lockKeychainAction, SIGNAL(triggered()), this, SLOT(lockKeychain()));
}

void SignatureActions::createMenus()
{
    menu = new QMenu();
    menu->addAction(addSignatureAction);
    menu->addSeparator();
    menu->addAction(unlockKeychainAction);
    menu->addAction(lockKeychainAction);
}

