#ifndef REQUESTPAYMENTDIALOG_H
#define REQUESTPAYMENTDIALOG_H

#include <QDialog>

namespace Ui {
class RequestPaymentDialog;
}

class RequestPaymentDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit RequestPaymentDialog(QWidget *parent = 0);
    ~RequestPaymentDialog();

    void setCurrentAccount(const QString& /*accountName*/) { }

private:
    Ui::RequestPaymentDialog *ui;
};

#endif // REQUESTPAYMENTDIALOG_H
