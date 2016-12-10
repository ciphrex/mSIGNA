///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// newkeychaindialogview.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "newkeychaindialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QIntValidator>

#include <stdexcept>

NewKeychainDialog::NewKeychainDialog(QWidget* parent)
    : QDialog(parent)
{
    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    // Keychain Name
    QLabel* nameLabel = new QLabel();
    nameLabel->setText(tr("Keychain Name:"));
    nameEdit = new QLineEdit();

    QHBoxLayout* nameLayout = new QHBoxLayout();
    nameLayout->setSizeConstraint(QLayout::SetNoConstraint);
    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(nameEdit);

    // NumKeys
/*
    QLabel* numKeysLabel = new QLabel();
    numKeysLabel->setText(tr("Number of Keys:"));
    numKeysEdit = new QLineEdit();
    numKeysEdit->setValidator(new QIntValidator(1, 10000));
    numKeysEdit->setText(QString::number(DEFAULT_NUMKEYS));

    QHBoxLayout* numKeysLayout = new QHBoxLayout();
    numKeysLayout->setSizeConstraint(QLayout::SetNoConstraint);
    numKeysLayout->addWidget(numKeysLabel);
    numKeysLayout->addWidget(numKeysEdit);
*/
 
    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
    mainLayout->addLayout(nameLayout);
    //mainLayout->addLayout(numKeysLayout);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
}

QString NewKeychainDialog::getName() const
{
    return nameEdit->text();
}

int NewKeychainDialog::getNumKeys() const
{
    return DEFAULT_NUMKEYS; //numKeysEdit->text().toInt();
}
