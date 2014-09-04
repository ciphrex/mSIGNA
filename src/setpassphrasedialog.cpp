///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// setpassphrasedialog.cpp
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.

#include "setpassphrasedialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>

SetPassphraseDialog::SetPassphraseDialog(const QString& prompt, const QString& additionalText, QWidget* parent)
    : QDialog(parent)
{
    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    // Prompt
    QLabel* promptLabel = new QLabel();
    promptLabel->setText(prompt);

    // Text Edits
    passphrase1Edit = new QLineEdit();
    passphrase1Edit->setEchoMode(QLineEdit::Password);
    passphrase2Edit = new QLineEdit();
    passphrase2Edit->setEchoMode(QLineEdit::Password);

    // Additional Text
    QLabel* additionalTextLabel = new QLabel();
    additionalTextLabel->setText(additionalText);

    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
    mainLayout->addWidget(promptLabel);
    mainLayout->addWidget(passphrase1Edit);
    mainLayout->addWidget(passphrase2Edit);
    mainLayout->addWidget(additionalTextLabel);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    resize(500, 140);
}

QString SetPassphraseDialog::getPassphrase() const
{
    if (passphrase1Edit->text() != passphrase2Edit->text())
        throw std::runtime_error("Passphrases do not match.");

    // TODO: check passphrase strength

    return passphrase1Edit->text();
}
