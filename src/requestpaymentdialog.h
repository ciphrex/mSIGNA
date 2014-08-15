///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// requestpaymentdialog.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

class AccountModel;
class AccountView;

#include <QDialog>

namespace Ui {
class RequestPaymentDialog;
}

class RequestPaymentDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit RequestPaymentDialog(AccountModel* accountModel, AccountView* accountView, QWidget *parent = 0);
    ~RequestPaymentDialog();

public slots:
    void setCurrentAccount(const QString& accountName);
    void clearInvoice();

private slots:
    void setAccounts(const QStringList& accountNames);
    void setQRCode(const QString& address);

    void on_newInvoiceButton_clicked();
    void on_addressClipboardButton_clicked();
    void on_scriptClipboardButton_clicked();
    void on_urlClipboardButton_clicked();
    void on_closeButton_clicked();

private:
    Ui::RequestPaymentDialog *ui;

    AccountModel* accountModel_;
    AccountView* accountView_;
};

