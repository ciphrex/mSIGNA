///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// currencyunitdialog.cpp
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "currencyunitdialog.h"

#include "coinparams.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>

#include <QMessageBox>

#include <stdexcept>

CurrencyUnitDialog::CurrencyUnitDialog(QWidget* parent)
    : QDialog(parent)
{
    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    // Currency unit drop down
    QLabel* label = new QLabel();
    label->setText(tr("Choose currency units:"));
    currencyUnitBox = new QComboBox();
    populateSelectorBox();

    QHBoxLayout* currencyUnitLayout = new QHBoxLayout();
    currencyUnitLayout->setSizeConstraint(QLayout::SetNoConstraint);
    currencyUnitLayout->addWidget(label);
    currencyUnitLayout->addWidget(currencyUnitBox);

    // Main Layout 
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
    mainLayout->addLayout(currencyUnitLayout);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
}

QString CurrencyUnitDialog::getCurrencyUnit() const
{
    QString unit = currencyUnitBox->currentText();
    return unit;
}

QString CurrencyUnitDialog::getCurrencyUnitPrefix() const
{
    QString unit = currencyUnitBox->currentText();
    return unit.left(unit.size() - strlen(getCoinParams().currency_symbol()));
}

void CurrencyUnitDialog::populateSelectorBox()
{
    for (auto& prefix: getValidCurrencyPrefixes())
    {
        currencyUnitBox->addItem(prefix + getCoinParams().currency_symbol());        
    }

    currencyUnitBox->setCurrentText(getCurrencySymbol());
}

