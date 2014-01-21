#include "acceptlicensedialog.h"
#include "ui_acceptlicensedialog.h"

AcceptLicenseDialog::AcceptLicenseDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AcceptLicenseDialog)
{
    ui->setupUi(this);
}

AcceptLicenseDialog::~AcceptLicenseDialog()
{
    delete ui;
}
