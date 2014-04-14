///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// passphrasedialog.h
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.

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
