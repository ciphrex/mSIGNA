///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// accounthistorydialog.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

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

