///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// requestpaymentdialog.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef COINVAULT_REQUESTPAYMENTDIALOG_H
#define COINVAULT_REQUESTPAYMENTDIALOG_H

class AccountModel;

class QComboBox;
class QLineEdit;

#include <QDialog>

class RequestPaymentDialog : public QDialog
{
    Q_OBJECT

public:
    RequestPaymentDialog(AccountModel* accountModel, QWidget* parent = NULL);

    QString getAccountName() const;
    QString getSender() const;

public slots:
    void setCurrentAccount(const QString& accountName);

private slots:
    void setAccounts(const QStringList& accountNames);

    void getScript();

private:
    AccountModel* accountModel;

    QComboBox* accountComboBox;
    QLineEdit* senderEdit;

    QLineEdit* labelEdit;
    QLineEdit* addressEdit;
    QLineEdit* scriptEdit;
};

#endif // COINVAULT_REQUESTPAYMENTDIALOG_H
