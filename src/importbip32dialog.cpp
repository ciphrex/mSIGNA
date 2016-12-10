///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// importbip32dialog.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

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
    setWindowTitle(tr("Import BIP32 Master Key"));

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

    resize(1000, 150);
}

QString ImportBIP32Dialog::getName() const
{
    return m_nameEdit->text();
}

secure_bytes_t ImportBIP32Dialog::getExtendedKey() const
{
    secure_bytes_t extkey;
    if (!fromBase58Check(m_base58Edit->text().toStdString(), extkey)) throw std::runtime_error("Invalid BIP32.");
    return extkey;
}

