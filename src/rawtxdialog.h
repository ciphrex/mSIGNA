///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// rawtxdialog.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

class QTextEdit;

#include <QDialog>

#include <CoinQ/CoinQ_typedefs.h>

class RawTxDialog : public QDialog
{
    Q_OBJECT

public:
    RawTxDialog(const QString& prompt, QWidget* parent = NULL);

    bytes_t getRawTx() const;
    void setRawTx(const bytes_t& rawTx);

private:
    QTextEdit* rawTxEdit;
};

