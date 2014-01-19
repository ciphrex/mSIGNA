///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// requestpaymentdialog.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#ifndef COINVAULT_REQUESTPAYMENTDIALOG_H
#define COINVAULT_REQUESTPAYMENTDIALOG_H

class AccountModel;

#include <QDialog>

namespace Ui {
class RequestPaymentDialog;
}

class RequestPaymentDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit RequestPaymentDialog(AccountModel* accountModel, QWidget *parent = 0);
    ~RequestPaymentDialog();

public slots:
    void setCurrentAccount(const QString& accountName);

private slots:
    void setAccounts(const QStringList& accountNames);

    void on_newInvoiceButton_clicked();
    void on_addressClipboardButton_clicked();
    void on_scriptClipboardButton_clicked();
    void on_urlClipboardButton_clicked();
    void on_closeButton_clicked();

private:
    Ui::RequestPaymentDialog *ui;

    AccountModel* accountModel_;
};

#endif // COINVAULT_REQUESTPAYMENTDIALOG_H
