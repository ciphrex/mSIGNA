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

#include <QLabel>
#include <QVBoxLayout>

SignatureDialog::SignatureDialog(CoinDB::Vault* vault, const bytes_t& txHash, QWidget* parent)
    : QDialog(parent)
{
    resize(QSize(600, 200));

    m_model = new SignatureModel(vault, txHash, this);
    m_model->update();

    m_view = new SignatureView(this);
    m_view->setModel(m_model);
    m_view->update();

    QString sigsNeededCaption = tr("Additional signatures required: ") + QString::number(m_model->getSigsNeeded());
    m_sigsNeededLabel = new QLabel(sigsNeededCaption);

    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->addWidget(m_sigsNeededLabel);
    mainLayout->addWidget(m_view);
    setLayout(mainLayout);
    setWindowTitle(QString::fromStdString(uchar_vector(txHash).getHex()));
}
