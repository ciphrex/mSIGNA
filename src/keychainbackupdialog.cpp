///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// keychainbackupdialog.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include "keychainbackupdialog.h"

#include <uchar_vector.h>
#include <Base58Check.h>

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QLabel>

#include <stdexcept>

KeychainBackupDialog::KeychainBackupDialog(const QString& prompt, QWidget* parent)
    : QDialog(parent)
{
/*
    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
*/
    // Prompt
    QLabel* promptLabel = new QLabel();
    promptLabel->setText(prompt);

    // Text Edit
    keychainBackupEdit = new QTextEdit();

    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
    mainLayout->addWidget(promptLabel);
    mainLayout->addWidget(keychainBackupEdit);
    //mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    resize(1000, 100);
}

void KeychainBackupDialog::setExtendedKey(const bytes_t& extendedKey)
{
    this->extendedKey = extendedKey;
    keychainBackupEdit->setText(toBase58Check(extendedKey).c_str());
}
