///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// setpassphrasedialog.cpp
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "setpassphrasedialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>

SetPassphraseDialog::SetPassphraseDialog(const QString& objectName, const QString& additionalText, QWidget* parent)
    : QDialog(parent)
{
    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    // Prompts
    QLabel* promptLabel = new QLabel(tr("Enter passphrase for ") + objectName + ":");
    QLabel* repromptLabel = new QLabel(tr("Enter it again:"));

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
    mainLayout->addWidget(repromptLabel);
    mainLayout->addWidget(passphrase2Edit);
    mainLayout->addWidget(additionalTextLabel);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    setMinimumWidth(500);
}

QString SetPassphraseDialog::getPassphrase() const
{
    if (passphrase1Edit->text() != passphrase2Edit->text())
        throw std::runtime_error("Passphrases do not match.");

    // TODO: check passphrase strength

    return passphrase1Edit->text();
}
