///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// txsearchdialog.h
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

class QLineEdit;

class TxModel;

#include <QDialog>

#include <CoinQ/CoinQ_typedefs.h>

class TxSearchDialog : public QDialog
{
    Q_OBJECT

public:
    TxSearchDialog(const TxModel& txModel, QWidget* parent = NULL);

    QString getTxHash() const;

private:
    QLineEdit* m_txHashEdit;

    const TxModel& m_txModel;
};
