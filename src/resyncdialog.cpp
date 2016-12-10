///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// resyncdialog.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "resyncdialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QIntValidator>

#include <stdexcept>

ResyncDialog::ResyncDialog(int resyncHeight, int maxHeight, QWidget* parent)
    : QDialog(parent)
{
    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    // Validator
    QValidator* validator = new QIntValidator(-maxHeight, maxHeight);

    QLabel* promptLabel = new QLabel(tr("Enter the height of the block to sync from. Use negative numbers to subtract from the current best chain height."));

    // Resync Height
    if (resyncHeight < 0) resyncHeight += maxHeight;
    if (resyncHeight < 0) {
        resyncHeight = 0;
    }
    else if (resyncHeight > maxHeight) {
        resyncHeight = maxHeight;
    }
    QLabel* resyncHeightLabel = new QLabel(tr("Resync Height (0 - ") + QString::number(maxHeight) + "): ");
    resyncHeightLabel->setText(tr("Resync Height:"));
    resyncHeightEdit = new QLineEdit();
    resyncHeightEdit->setText(QString::number(resyncHeight));
    resyncHeightEdit->setValidator(validator);

    QHBoxLayout* resyncHeightLayout = new QHBoxLayout();
    resyncHeightLayout->setSizeConstraint(QLayout::SetNoConstraint);
    resyncHeightLayout->addWidget(resyncHeightLabel);
    resyncHeightLayout->addWidget(resyncHeightEdit);

    // Main Layout 
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
    mainLayout->addWidget(promptLabel);
    mainLayout->addLayout(resyncHeightLayout);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
}

int ResyncDialog::getResyncHeight() const
{
    return resyncHeightEdit->text().toInt();
}
