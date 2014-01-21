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
