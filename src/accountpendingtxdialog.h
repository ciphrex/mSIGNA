///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// accounthistorydialog.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef COINVAULT_ACCOUNTHISTORYDIALOG_H
#define COINVAULT_ACCOUNTHISTORYDIALOG_H

#include <CoinQ_vault.h>

class TxModel;
class TxView;

#include <QDialog>

class AccountHistoryDialog : public QDialog
{
    Q_OBJECT

public:
    AccountHistoryDialog(CoinQ::Vault::Vault* vault, const QString& accountName, QWidget* parent = NULL);

private:
    TxModel* accountHistoryModel;
    TxView* accountHistoryView;
};

#endif // COINVAULT_ACCOUNTHISTORYDIALOG_H
