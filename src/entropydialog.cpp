///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// entropydialog.cpp
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.

#include "entropydialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>

EntropyDialog::EntropyDialog(QWidget* parent)
    : QDialog(parent)
{
    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel);

    //connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    // Prompt
    QLabel* promptLabel = new QLabel(tr("Getting entropy from system. This might take a while, please wait..."));

    QHBoxLayout* mainLayout = new QHBoxLayout();
    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
    mainLayout->addWidget(promptLabel);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
}

