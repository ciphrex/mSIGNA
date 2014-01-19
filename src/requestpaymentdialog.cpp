#include "requestpaymentdialog.h"
#include "ui_requestpaymentdialog.h"

RequestPaymentDialog::RequestPaymentDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RequestPaymentDialog)
{
    ui->setupUi(this);
}

RequestPaymentDialog::~RequestPaymentDialog()
{
    delete ui;
}
