///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// newtworkselectiondialog.cpp
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "networkselectiondialog.h"

#include <CoinQ/CoinQ_coinparams.h>

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>

#include <QMessageBox>

#include <stdexcept>

NetworkSelectionDialog::NetworkSelectionDialog(CoinQ::NetworkSelector& selector, QWidget* parent)
    : QDialog(parent)
{
    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    // Network drop down
    QLabel* networkLabel = new QLabel();
    networkLabel->setText(tr("Choose network:"));
    networkBox = new QComboBox();
    populateNetworkBox(selector);

    QHBoxLayout* networkLayout = new QHBoxLayout();
    networkLayout->setSizeConstraint(QLayout::SetNoConstraint);
    networkLayout->addWidget(networkLabel);
    networkLayout->addWidget(networkBox);

    // Main Layout 
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
    mainLayout->addLayout(networkLayout);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
}

QString NetworkSelectionDialog::getNetworkName() const
{
    return networkBox->currentText();
}

void NetworkSelectionDialog::populateNetworkBox(CoinQ::NetworkSelector& selector)
{
    for (auto& network: selector.getNetworkNames())
    {
        networkBox->addItem(QString::fromStdString(network));        
    }
}

