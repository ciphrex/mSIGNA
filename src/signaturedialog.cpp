///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// signaturedialog.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#include "signaturedialog.h"

// Models/Views
#include "signaturemodel.h"
#include "signatureview.h"
#include "signatureactions.h"

#include <CoinDB/SynchedVault.h>

#include <QLabel>
#include <QVBoxLayout>

SignatureDialog::SignatureDialog(CoinDB::SynchedVault& synchedVault, const bytes_t& txHash, QWidget* parent)
    : QDialog(parent), m_synchedVault(synchedVault)
{
    resize(QSize(700, 200));

    m_model = new SignatureModel(m_synchedVault, txHash, this);
    m_model->updateAll();

    m_view = new SignatureView(this);
    m_view->setModel(m_model);
    m_view->updateColumns();

    m_actions = new SignatureActions(m_synchedVault, *this);
    m_view->setMenu(m_actions->getMenu());

    if (m_actions) { m_view->setMenu(m_actions->getMenu()); }

    m_sigsNeededLabel = new QLabel();
    updateCaption();

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_sigsNeededLabel);
    mainLayout->addWidget(m_view);
    setLayout(mainLayout);
    setWindowTitle(QString::fromStdString(uchar_vector(txHash).getHex()));
}

SignatureDialog::~SignatureDialog()
{
    delete m_model;
    delete m_view;
    delete m_actions;
}

void SignatureDialog::updateTx()
{
    updateKeychains();
    updateCaption();
    emit txUpdated();
}

void SignatureDialog::updateKeychains()
{
    m_model->updateAll();
    m_view->updateColumns();
}

void SignatureDialog::updateCaption()
{
    unsigned int sigsNeeded = m_model->getSigsNeeded();

    QString caption;
    if (sigsNeeded == 0)
    {
        caption = tr("Transaction is signed.");
    }
    else
    {
        caption = tr("Additional signatures required: ") + QString::number(sigsNeeded);
    }

    m_sigsNeededLabel->setText(caption);
}
