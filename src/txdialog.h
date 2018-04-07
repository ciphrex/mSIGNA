///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// txdialog.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include <CoinDB/Vault.h>

class TxOutModel;
class TxOutView;

#include <QDialog>

class TxDialog : public QDialog
{
    Q_OBJECT

public:
    TxDialog(CoinDB::Vault* vault, const QString& accountName, QWidget* parent = NULL);

    void setVault(CoinDB::Vault* vault);
    void setAccount(const QString& accountName);

protected:
    void showError(const QString& errorMsg);

private:
    TxOutModel* txOutModel;
    TxOutView* txOutView;
};

