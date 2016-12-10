///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// viewbip39dialog.cpp
//
// Copyright (c) 2015 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

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
    init(QString(), name, seed);
}

ViewBIP39Dialog::ViewBIP39Dialog(const QString& prompt, const QString& name, const secure_bytes_t& seed, QWidget* parent)
    : QDialog(parent)
{
    init(prompt, name, seed);
}

void ViewBIP39Dialog::init(const QString& prompt, const QString& name, const secure_bytes_t& seed)
{
    setWindowTitle(tr("Wordlist for ") + name);

    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QLineEdit* wordlistEdit = new QLineEdit();
    wordlistEdit->setReadOnly(true);
    wordlistEdit->setText(Coin::BIP39::toWordlist(seed).c_str());

    QVBoxLayout* mainLayout = new QVBoxLayout();
    //mainLayout->setSizeConstraint(QLayout::SetNoConstraint);

    if (!prompt.isEmpty())
    {
        QLabel* promptLabel= new QLabel(prompt);
        mainLayout->addWidget(promptLabel);
    }

    mainLayout->addWidget(wordlistEdit);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    resize(1000, 100);
}

