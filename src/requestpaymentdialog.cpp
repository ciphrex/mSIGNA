///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// requestpaymentdialogview.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include "requestpaymentdialog.h"
#include "accountmodel.h"

#include <QGroupBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>

#include <stdexcept>

RequestPaymentDialog::RequestPaymentDialog(AccountModel* accountModel, QWidget* parent)
    : QDialog(parent)
{
    this->accountModel = accountModel;
    connect(accountModel, SIGNAL(updated(const QStringList&)), this, SLOT(setAccounts(const QStringList&)));

    ////////
    // Input
    QGroupBox* inputGroupBox = new QGroupBox(tr("Request Payment"));

    // Account name
    QLabel* accountLabel = new QLabel(tr("Account:"));
    accountComboBox = new QComboBox();

    QHBoxLayout* accountLayout = new QHBoxLayout();
    accountLayout->addWidget(accountLabel);
    accountLayout->addWidget(accountComboBox);

    // Sender
    QLabel* senderLabel = new QLabel(tr("From:"));
    senderEdit = new QLineEdit();

    // Get Script button
    QPushButton* getScriptButton = new QPushButton(tr("Get Script"));
    connect(getScriptButton, SIGNAL(clicked()), this, SLOT(getScript()));

    QHBoxLayout* senderLayout = new QHBoxLayout();
    senderLayout->addWidget(senderLabel);
    senderLayout->addWidget(senderEdit);
    senderLayout->addWidget(getScriptButton);

    // Input Layout 
    QVBoxLayout *inputLayout = new QVBoxLayout();
    inputLayout->addLayout(accountLayout);
    inputLayout->addLayout(senderLayout);
    inputLayout->addWidget(getScriptButton);
    inputGroupBox->setLayout(inputLayout);

    /////////
    // Output
    QGroupBox* outputGroupBox = new QGroupBox(tr("Script Details"));

    // Label
    QLabel* labelLabel = new QLabel(tr("Label:"));
    labelEdit = new QLineEdit();
    labelEdit->setReadOnly(true);
    QHBoxLayout* labelLayout = new QHBoxLayout();
    labelLayout->addWidget(labelLabel);
    labelLayout->addWidget(labelEdit);

    // Address
    QLabel* addressLabel = new QLabel(tr("Address:"));
    addressEdit = new QLineEdit();
    addressEdit->setReadOnly(true);
    QHBoxLayout* addressLayout = new QHBoxLayout();
    addressLayout->addWidget(addressLabel);
    addressLayout->addWidget(addressEdit);

    // Script
    QLabel* scriptLabel = new QLabel(tr("Script:"));
    scriptEdit = new QLineEdit();
    scriptEdit->setReadOnly(true);
    QHBoxLayout* scriptLayout = new QHBoxLayout();
    scriptLayout->addWidget(scriptLabel);
    scriptLayout->addWidget(scriptEdit);

    // Output Layout
    QVBoxLayout *outputLayout = new QVBoxLayout();
    outputLayout->addLayout(labelLayout);
    outputLayout->addLayout(addressLayout);
    outputLayout->addLayout(scriptLayout);
    outputGroupBox->setLayout(outputLayout);

    ////////////
    // OK Button
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));

    //////////////
    // Main Layout
    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->addWidget(inputGroupBox);
    mainLayout->addWidget(outputGroupBox);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    resize(450, 300);
}

void RequestPaymentDialog::setAccounts(const QStringList& accountNames)
{
    accountComboBox->clear();
    accountComboBox->addItems(accountNames);
}

void RequestPaymentDialog::setCurrentAccount(const QString& accountName)
{
    accountComboBox->setCurrentText(accountName);
}

QString RequestPaymentDialog::getAccountName() const
{
    return accountComboBox->currentText();
}

QString RequestPaymentDialog::getSender() const
{
    return senderEdit->text();
}

void RequestPaymentDialog::getScript()
{
    try {
        QString accountName = getAccountName();
        QString label = getSender();
        auto pair = accountModel->issueNewScript(accountName, label);
        accountModel->update();
        labelEdit->setText(label);
        addressEdit->setText(pair.first);
        scriptEdit->setText(QString::fromStdString(uchar_vector(pair.second).getHex()));
    }
    catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Error"), QString::fromStdString(e.what()));
    }
}
