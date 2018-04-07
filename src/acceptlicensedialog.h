///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// acceptlicensedialog.h
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#ifndef ACCEPTLICENSEDIALOG_H
#define ACCEPTLICENSEDIALOG_H

#include <QDialog>

namespace Ui {
class AcceptLicenseDialog;
}

class AcceptLicenseDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit AcceptLicenseDialog(QWidget *parent = 0);
    ~AcceptLicenseDialog();
    
private:
    Ui::AcceptLicenseDialog *ui;
};

#endif // ACCEPTLICENSEDIALOG_H
