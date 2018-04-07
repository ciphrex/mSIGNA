///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// signatureactions.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "signatureactions.h"

#include "signaturemodel.h"
#include "signatureview.h"
#include "signaturedialog.h"
#include "passphrasedialog.h"

#include "entropysource.h"

#include <CoinDB/SynchedVault.h>
#include <CoinDB/Passphrase.h>

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

        QString currentKeychain = m_currentKeychain;
        bool bKeychainLocked = vault->isKeychainLocked(m_currentKeychain.toStdString());
        if (bKeychainLocked && !unlockKeychain(false)) return;

        std::vector<std::string> keychainNames;
        keychainNames.push_back(currentKeychain.toStdString());

        seedEntropySource(false, &m_dialog);
        bytes_t txhash = m_dialog.getModel()->getTxHash();
        try
        {
            vault->signTx(txhash, keychainNames, true);
        }
        catch (const std::exception& e)
        {
            if (bKeychainLocked) lockKeychain();
            throw;
        }

LOGGER(debug) << "calling lockKeychain()" << std::endl;
        if (bKeychainLocked) lockKeychain(); // Relock keychain if it was locked prior to adding this signature.
LOGGER(debug) << "lockKeychain() returned" << std::endl;

        if (keychainNames.empty())
        {
            QMessageBox::critical(nullptr, tr("Error"), tr("Signature could not be added."));
            return;
        }

        m_dialog.updateTx();
        if (m_synchedVault.isConnected() && m_dialog.getModel()->getSigsNeeded() == 0)
        {
            QMessageBox sendPrompt;
            sendPrompt.setText(tr("Transaction is fully signed. Would you like to send?"));
            sendPrompt.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            sendPrompt.setDefaultButton(QMessageBox::Yes);
            if (sendPrompt.exec() == QMessageBox::Yes)
            {
                if (!m_synchedVault.isConnected())
                {
                    QMessageBox::critical(nullptr, tr("Error"), tr("Connection was lost."));
                    return;
                }
                std::shared_ptr<CoinDB::Tx> tx = vault->getTx(txhash);
                Coin::Transaction coin_tx = tx->toCoinCore();
                m_synchedVault.sendTx(coin_tx);
            }
        }
    }
    catch (const std::exception& e)
    {
        emit error(e.what());
    }
}

bool SignatureActions::unlockKeychain(bool bUpdateKeychains)
{
    try
    {
        if (m_currentKeychain.isEmpty()) throw std::runtime_error("No keychain is selected.");
        if (!m_synchedVault.isVaultOpen()) throw std::runtime_error("No vault is open.");

        CoinDB::Vault* vault = m_synchedVault.getVault();
        std::string keychainName = m_currentKeychain.toStdString();
        if (!vault->isKeychainLocked(keychainName))
        {
            QMessageBox::critical(nullptr, tr("Error"), tr("Keychain is already unlocked."));
            return true;
        }

        secure_bytes_t lockKey;
        if (vault->isKeychainEncrypted(keychainName))
        {
            PassphraseDialog dlg(tr("Please enter the decryption passphrase for ") + m_currentKeychain + ":");
            if (!dlg.exec()) return false;
            lockKey = passphraseHash(dlg.getPassphrase().toStdString());
        }

        vault->unlockKeychain(keychainName, lockKey);
        refreshCurrentKeychain();
        if (bUpdateKeychains) m_dialog.updateKeychains();
    }
    catch (const CoinDB::KeychainPrivateKeyUnlockFailedException& e)
    {
        emit error(tr("Keychain decryption failed."));
	return false;
    }
    catch (const std::exception& e)
    {
        emit error(e.what());
        return false;
    }

    return true;
}

void SignatureActions::lockKeychain()
{
    try
    {
        if (m_currentKeychain.isEmpty()) throw std::runtime_error("No keychain is selected.");
        if (!m_synchedVault.isVaultOpen()) throw std::runtime_error("No vault is open.");

        CoinDB::Vault* vault = m_synchedVault.getVault();
        std::string keychainName = m_currentKeychain.toStdString();
        if (vault->isKeychainLocked(keychainName))
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
    addSignatureAction = new QAction(QIcon(":/icons/glossy-black-signature_32x32.png"), tr("Add Signature..."), this);
    addSignatureAction->setEnabled(false);
    connect(addSignatureAction, SIGNAL(triggered()), this, SLOT(addSignature()));

    unlockKeychainAction = new QAction(QIcon(":/icons/unlocked.png"), tr("Unlock Keychain..."), this);
    unlockKeychainAction->setEnabled(false);
    connect(unlockKeychainAction, SIGNAL(triggered()), this, SLOT(unlockKeychain()));

    lockKeychainAction = new QAction(QIcon(":/icons/locked.png"), tr("Lock Keychain"), this);
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

