///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// rawtxdialog.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

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

