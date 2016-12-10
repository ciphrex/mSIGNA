///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// aboutdialog.cpp
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "aboutdialog.h"
#include "settings.h"
#include "versioninfo.h"
#include "copyrightinfo.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QLabel>

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("About ") + getDefaultSettings().getAppName());
    setStyleSheet("font: bold 9pt");

    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));

    QLabel* versionLabel = new QLabel(getVersionText());
    versionLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);

    QLabel* schemaLabel = new QLabel(getSchemaVersionText());
    schemaLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);

    QLabel* commitLabel = new QLabel(getCommitHash());
    commitLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);

    QLabel* openSSLLabel = new QLabel(getOpenSSLVersionText());
    openSSLLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);

    QFormLayout* formLayout = new QFormLayout();
    formLayout->addRow(tr("Version:"), versionLabel);
    formLayout->addRow(tr("Schema:"), schemaLabel);
    formLayout->addRow(tr("Commit:"), commitLabel);
    formLayout->addRow(tr("OpenSSL:"), openSSLLabel);

    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
    mainLayout->addLayout(formLayout);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(new QLabel(getCopyrightText()));
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
}

