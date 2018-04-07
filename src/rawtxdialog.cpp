///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// rawtxdialogview.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "rawtxdialog.h"

#include <stdutils/uchar_vector.h>

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QLabel>

#include <stdexcept>

RawTxDialog::RawTxDialog(const QString& prompt, QWidget* parent)
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
    rawTxEdit = new QTextEdit();

    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
    mainLayout->addWidget(promptLabel);
    mainLayout->addWidget(rawTxEdit);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
}

bytes_t RawTxDialog::getRawTx() const
{
    uchar_vector rawTx;
    rawTx.setHex(rawTxEdit->toPlainText().toStdString());
    return rawTx;    
}

void RawTxDialog::setRawTx(const bytes_t& rawTx)
{
    rawTxEdit->setText(uchar_vector(rawTx).getHex().c_str());
}
