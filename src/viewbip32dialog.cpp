///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// viewbip32dialog.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "viewbip32dialog.h"

#include <stdutils/uchar_vector.h>
#include <CoinCore/Base58Check.h>

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>

#include <stdexcept>

ViewBIP32Dialog::ViewBIP32Dialog(const QString& name, const secure_bytes_t& extendedKey, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Extended Master Key for ") + name);

    QLabel* promptLabel = new QLabel(tr("BIP32 Extended Key:"));

    QLineEdit* base58Edit = new QLineEdit();
    base58Edit->setReadOnly(true);
    base58Edit->setText(toBase58Check(extendedKey).c_str());

    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
    mainLayout->addWidget(promptLabel);
    mainLayout->addWidget(base58Edit);
    setLayout(mainLayout);

    resize(1000, 100);
}

