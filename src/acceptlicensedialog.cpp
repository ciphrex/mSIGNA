#include "acceptlicensedialog.h"
#include "ui_acceptlicensedialog.h"

AcceptLicenseDialog::AcceptLicenseDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AcceptLicenseDialog)
{
    ui->setupUi(this);
    ui->licenseTextBrowser->setSource(QUrl("qrc:/docs/eula.html"));
}

AcceptLicenseDialog::~AcceptLicenseDialog()
{
    delete ui;
}
