///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// quicknewaccountdialog.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#include "quicknewaccountdialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>

#include <stdexcept>

QuickNewAccountDialog::QuickNewAccountDialog(QWidget* parent)
    : QDialog(parent)
{
    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    // Account Name
    QLabel* nameLabel = new QLabel();
    nameLabel->setText(tr("Account Name:"));
    nameEdit = new QLineEdit();

    QHBoxLayout* nameLayout = new QHBoxLayout();
    nameLayout->setSizeConstraint(QLayout::SetNoConstraint);
    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(nameEdit);

    // Policy
    QLabel *policyLabel = new QLabel();
    policyLabel->setText(tr("Policy:"));
    QLabel *ofLabel = new QLabel();
    ofLabel->setText(tr("of"));

    minSigComboBox = new QComboBox();
    maxSigComboBox = new QComboBox();
    for (int i = 1; i <= MAX_SIG_COUNT; i++)
        minSigComboBox->addItem(QString::number(i));
        maxSigComboBox->addItem(QString::number(i));
    }

    QHBoxLayout* policyLayout = new QHBoxLayout();
    policyLayout->setSizeConstraint(QLayout::SetNoConstraint);
    policyLayout->addWidget(policyLabel);
    policyLayout->addWidget(minSigLabel);
    policyLayout->addWidget(minSigComboBox);
    policyLayout->addWidget(ofLabel);
    policyLayout->addWidget(maxSigComboBox);

    // Main Layout 
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
    mainLayout->addLayout(nameLayout);
    mainLayout->addLayout(policyLayout);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
}

QString QuickNewAccountDialog::getName() const
{
    return nameEdit->text();
}

int NewAccountDialog::getMinSigs() const
{
    return minSigComboBox->currentIndex() + 1;
}

int QuickNewAccountDialog::getMaxSigs() const
{
    return maxSigComboBox->currentIndex() + 1;
}
