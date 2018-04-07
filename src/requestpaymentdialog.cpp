///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// requestpaymentdialog.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "requestpaymentdialog.h"
#include "ui_requestpaymentdialog.h"

#include "accountmodel.h"
#include "accountview.h"
#include "coinparams.h"
#include "numberformats.h"
#include "currencyvalidator.h"

#include <QMessageBox>
#include <QClipboard>
#include <QRegExpValidator>

#include <qrencode.h>

RequestPaymentDialog::RequestPaymentDialog(AccountModel* accountModel, AccountView* accountView, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RequestPaymentDialog),
    accountModel_(accountModel),
    accountView_(accountView)
{
    ui->setupUi(this);
    connect(accountModel_, SIGNAL(updated(const QStringList&)), this, SLOT(setAccounts(const QStringList&)));

    ui->invoiceAmountLabel->setText(tr("Amount") + " (" + getCurrencySymbol() + "):");
    ui->invoiceAmountLineEdit->setValidator(new CurrencyValidator(getCoinParams().currency_max(), getCoinParams().currency_decimals()));
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

void RequestPaymentDialog::setQRCode(const QString& address)
{
    QRcode* qrcode = QRcode_encodeString(address.toUtf8().data(), 0, QR_ECLEVEL_H, QR_MODE_8, 1);
    QImage qrImage(qrcode->width + 6, qrcode->width + 6, QImage::Format_RGB32);
    qrImage.fill(0xffffff);
    unsigned char* p = qrcode->data;
    for (int y = 0; y < qrcode->width; y++)
    {
        for (int x = 0; x < qrcode->width; x++)
        {
            qrImage.setPixel(x + 3, y + 3, ((*p & 1) ? 0x000000 : 0xffffff));
            p++;
        }
    }
    QRcode_free(qrcode);

    ui->qrLabel->setPixmap(QPixmap::fromImage(qrImage).scaled(ui->qrLabel->width(), ui->qrLabel->height()));
}

void RequestPaymentDialog::setCurrentAccount(const QString &accountName)
{
    ui->accountComboBox->setCurrentText(accountName);
    clearInvoice();
}

void RequestPaymentDialog::clearInvoice()
{
        ui->invoiceDetailsLabelLineEdit->clear();
        ui->invoiceDetailsAddressLineEdit->clear();
        ui->invoiceDetailsScriptLineEdit->clear();
        ui->invoiceDetailsUrlLineEdit->clear();
        ui->qrLabel->clear();
}

void RequestPaymentDialog::on_newInvoiceButton_clicked()
{
    try {
        QString accountName = ui->accountComboBox->currentText();
        QString invoiceLabel = ui->invoiceLineEdit->text();
        QString invoiceAmount = ui->invoiceAmountLineEdit->text();
        auto pair = accountModel_->issueNewScript(accountName, invoiceLabel);
        accountView_->updateAll();
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
        setQRCode(pair.first);
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

