#include "requestpaymentdialog.h"
#include "ui_requestpaymentdialog.h"

#include "accountmodel.h"

#include <QMessageBox>

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
        auto pair = accountModel_->issueNewScript(accountName, invoiceLabel);
        accountModel_->update();
        ui->invoiceDetailsLabelLineEdit->setText(invoiceLabel);
        ui->invoiceDetailsAddressLineEdit->setText(pair.first);
        ui->invoiceDetailsScriptLineEdit->setText(QString::fromStdString(uchar_vector(pair.second).getHex()));
    }
    catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Error"), QString::fromStdString(e.what()));
    }
}

void RequestPaymentDialog::on_closeButton_clicked()
{
    this->close();
}

