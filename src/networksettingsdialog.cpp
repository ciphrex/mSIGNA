///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// newtworksettingsdialog.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "networksettingsdialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>

#include <stdexcept>

NetworkSettingsDialog::NetworkSettingsDialog(const QString& host, int port, bool autoConnect, QWidget* parent)
    : QDialog(parent)
{
    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    // Host
    QLabel* hostLabel = new QLabel();
    hostLabel->setText(tr("Host:"));
    hostEdit = new QLineEdit();
    hostEdit->setText(host);

    QHBoxLayout* hostLayout = new QHBoxLayout();
    hostLayout->setSizeConstraint(QLayout::SetNoConstraint);
    hostLayout->addWidget(hostLabel);
    hostLayout->addWidget(hostEdit);

    // Port
    QLabel* portLabel = new QLabel();
    portLabel->setText(tr("Port:"));
    portEdit = new QLineEdit();
    portEdit->setText(QString::number(port));

    QHBoxLayout* portLayout = new QHBoxLayout();
    portLayout->setSizeConstraint(QLayout::SetNoConstraint);
    portLayout->addWidget(portLabel);
    portLayout->addWidget(portEdit);

    // Auto connect on startup
    autoConnectCheckBox = new QCheckBox(tr("Automatically connect on startup"));
    autoConnectCheckBox->setChecked(autoConnect);

    // Main Layout 
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
    mainLayout->addLayout(hostLayout);
    mainLayout->addLayout(portLayout);
    mainLayout->addWidget(autoConnectCheckBox);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
}

QString NetworkSettingsDialog::getHost() const
{
    return hostEdit->text();
}

int NetworkSettingsDialog::getPort() const
{
    return portEdit->text().toInt();
}

bool NetworkSettingsDialog::getAutoConnect() const
{
    return autoConnectCheckBox->isChecked();
}
