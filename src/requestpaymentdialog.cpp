///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// requestpaymentdialogview.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#include "requestpaymentdialog.h"
#include "ui_requestpaymentdialog.h"

#include "accountmodel.h"
#include "coinparams.h"

#include <QMessageBox>
#include <QClipboard>

RequestPaymentDialog::RequestPaymentDialog(AccountModel* accountModel, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RequestPaymentDialog),
    accountModel_(accountModel)
{
    ui->setupUi(this);
    connect(accountModel_, SIGNAL(updated(const QStringList&)), this, SLOT(setAccounts(const QStringList&)));
}

RequestPaymentDialog::~RequestPaymentDialog()
{
    delete ui;
}

void RequestPaymentDialog::setAccounts(const QStringList& accountNames)
{
    ui->accountComboBox->clear();
    ui->accountComboBox->addItems(accountNames);
}

void RequestPaymentDialog::setCurrentAccount(const QString &accountName)
{
    ui->accountComboBox->setCurrentText(accountName);
}

void RequestPaymentDialog::on_newInvoiceButton_clicked()
{
    try {
        QString accountName = ui->accountComboBox->currentText();
        QString invoiceLabel = ui->invoiceLineEdit->text();
        QString invoiceAmount = ui->invoiceAmountLineEdit->text();
        auto pair = accountModel_->issueNewScript(accountName, invoiceLabel);
        accountModel_->update();
        ui->invoiceDetailsLabelLineEdit->setText(invoiceLabel);
        ui->invoiceDetailsAddressLineEdit->setText(pair.first);
        ui->invoiceDetailsScriptLineEdit->setText(QString::fromStdString(uchar_vector(pair.second).getHex()));
        QString url = QString(getCoinParams().url_prefix()) + ":" + pair.first;
        int nParams = 0;
        if (!invoiceLabel.isEmpty()) {
            url += (nParams == 0) ? "?" : "&";
            url += "label=";
            url += invoiceLabel;
            nParams++;
        }
        if (!invoiceAmount.isEmpty()) {
            url += (nParams == 0) ? "?" : "&";
            url += "amount=";
            url += invoiceAmount;
            nParams++;
        }
        ui->invoiceDetailsUrlLineEdit->setText(url);
    }
    catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Error"), QString::fromStdString(e.what()));
    }
}

void RequestPaymentDialog::on_addressClipboardButton_clicked()
{
    QApplication::clipboard()->setText(ui->invoiceDetailsAddressLineEdit->text());
}

void RequestPaymentDialog::on_scriptClipboardButton_clicked()
{
    QApplication::clipboard()->setText(ui->invoiceDetailsScriptLineEdit->text());
}

void RequestPaymentDialog::on_urlClipboardButton_clicked()
{
    QApplication::clipboard()->setText(ui->invoiceDetailsUrlLineEdit->text());
}

void RequestPaymentDialog::on_closeButton_clicked()
{
    this->close();
}

