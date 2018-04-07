///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// importbip39dialog.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "importbip39dialog.h"

#include <CoinCore/bip39.h>

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>

#include <stdexcept>

ImportBIP39Dialog::ImportBIP39Dialog(QWidget* parent)
    : QDialog(parent)
{
    init(); 
}

ImportBIP39Dialog::ImportBIP39Dialog(const QString& name, QWidget* parent)
    : QDialog(parent)
{
    init(name);
}

void ImportBIP39Dialog::init(const QString& name)
{
    setWindowTitle(tr("Import Wordlist"));

    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QLabel* nameLabel = new QLabel(tr("Name:"));
    m_nameEdit = new QLineEdit();

    if (!name.isEmpty())
    {
        m_nameEdit->setText(name);
        m_nameEdit->setReadOnly(true);
    }

    QHBoxLayout* nameLayout = new QHBoxLayout();
    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(m_nameEdit);

    QLabel* wordlistLabel = new QLabel(tr("Wordlist:"));
    m_wordlistEdit = new QLineEdit();

    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
    mainLayout->addLayout(nameLayout);
    mainLayout->addWidget(wordlistLabel);
    mainLayout->addWidget(m_wordlistEdit);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    resize(1000, 150);
}

QString ImportBIP39Dialog::getName() const
{
    return m_nameEdit->text();
}

secure_bytes_t ImportBIP39Dialog::getSeed() const
{
    return Coin::BIP39::fromWordlist(m_wordlistEdit->text().toStdString());
}

