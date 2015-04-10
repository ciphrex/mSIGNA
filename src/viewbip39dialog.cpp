///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// viewbip39dialog.cpp
//
// Copyright (c) 2015 Eric Lombrozo
//
// All Rights Reserved.

#include "viewbip39dialog.h"

#include <CoinCore/bip39.h>

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>

#include <stdexcept>

ViewBIP39Dialog::ViewBIP39Dialog(const QString& name, const secure_bytes_t& seed, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Wordlist for ") + name);

    QLineEdit* wordlistEdit = new QLineEdit();
    wordlistEdit->setReadOnly(true);
    wordlistEdit->setText(Coin::BIP39::toWordlist(seed).c_str());

    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
    mainLayout->addWidget(wordlistEdit);
    setLayout(mainLayout);

    resize(1000, 100);
}

