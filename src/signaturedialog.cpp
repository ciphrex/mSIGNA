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
    resize(QSize(600, 200));

    m_model = new SignatureModel(m_synchedVault, txHash, this);
    m_model->update();

    m_view = new SignatureView(this);
    m_view->setModel(m_model);
    m_view->update();

    m_actions = new SignatureActions(m_synchedVault, *this);
    m_view->setMenu(m_actions->getMenu());

    if (m_actions) { m_view->setMenu(m_actions->getMenu()); }

    QString sigsNeededCaption = tr("Additional signatures required: ") + QString::number(m_model->getSigsNeeded());
    m_sigsNeededLabel = new QLabel(sigsNeededCaption);

    QVBoxLayout* mainLayout = new QVBoxLayout();
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

