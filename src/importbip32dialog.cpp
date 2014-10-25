///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// importbip32dialog.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include "importbip32dialog.h"

#include <stdutils/uchar_vector.h>
#include <CoinCore/Base58Check.h>

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>

#include <stdexcept>

ImportBIP32Dialog::ImportBIP32Dialog(QWidget* parent)
    : QDialog(parent)
{
    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QLabel* nameLabel = new QLabel(tr("Name:"));
    m_nameEdit = new QLineEdit();
    QHBoxLayout* nameLayout = new QHBoxLayout();
    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(m_nameEdit);

    QLabel* base58Label = new QLabel(tr("BIP32 Extended Key:"));
    m_base58Edit = new QLineEdit();

    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
    mainLayout->addLayout(nameLayout);
    mainLayout->addWidget(base58Label);
    mainLayout->addWidget(m_base58Edit);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    resize(1000, 300);
}

QString ImportBIP32Dialog::getName() const
{
    return m_nameEdit->text();
}

secure_bytes_t ImportBIP32Dialog::getExtendedKey() const
{
    return secure_bytes_t();
}

