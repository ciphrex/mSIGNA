///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// txsearchdialog.h
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.

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
