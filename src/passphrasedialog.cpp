///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// passphrasedialog.cpp
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.

#include "passphrasedialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>

PassphraseDialog::PassphraseDialog(const QString& prompt, QWidget* parent)
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

    // Text Edit
    passphraseEdit = new QLineEdit();
    passphraseEdit->setEchoMode(QLineEdit::Password);

    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
    mainLayout->addWidget(promptLabel);
    mainLayout->addWidget(passphraseEdit);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    resize(500, 140);
}

QString PassphraseDialog::getPassphrase() const
{
    if (passphraseEdit->text().isEmpty())
        throw std::runtime_error("Passphrase is empty.");

    return passphraseEdit->text();
}
