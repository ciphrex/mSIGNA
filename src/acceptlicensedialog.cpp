///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// acceptlicensedialog.cpp
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

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
