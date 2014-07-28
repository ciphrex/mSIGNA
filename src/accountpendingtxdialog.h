///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// accounthistorydialog.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

#include <Vault.h>

class TxModel;
class TxView;

#include <QDialog>

class AccountHistoryDialog : public QDialog
{
    Q_OBJECT

public:
    AccountHistoryDialog(CoinDB::Vault* vault, const QString& accountName, QWidget* parent = NULL);

private:
    TxModel* accountHistoryModel;
    TxView* accountHistoryView;
};

