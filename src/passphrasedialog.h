///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// passphrasedialog.h
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

class QLineEdit;

#include <QDialog>

#include <CoinQ/CoinQ_typedefs.h>

class PassphraseDialog : public QDialog
{
    Q_OBJECT

public:
    PassphraseDialog(const QString& prompt, QWidget* parent = NULL);

    QString getPassphrase() const;

private:
    QLineEdit* passphraseEdit;
};
