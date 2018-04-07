///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// txsearchdialog.cpp
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "txsearchdialog.h"
#include "txmodel.h"

#include "hexvalidator.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>

TxSearchDialog::TxSearchDialog(const TxModel& txModel, QWidget* parent)
    : QDialog(parent), m_txModel(txModel)
{
    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    // Prompt

    QLabel* promptLabel = new QLabel();
    promptLabel->setText(tr("Transaction Hash:"));

    // Transaction Hash Edit
    m_txHashEdit = new QLineEdit();
    m_txHashEdit->setValidator(new HexValidator(1, 32, HexValidator::ACCEPT_LOWER | HexValidator::ACCEPT_UPPER, this));

    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
    mainLayout->addWidget(promptLabel);
    mainLayout->addWidget(m_txHashEdit);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    resize(500, 140);
}

QString TxSearchDialog::getTxHash() const
{
    return m_txHashEdit->text().toLower();
}
