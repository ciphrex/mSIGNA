///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// keychainbackupdialog.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

class QTextEdit;

#include <QDialog>

#include <CoinQ/CoinQ_typedefs.h>

class KeychainBackupDialog : public QDialog
{
    Q_OBJECT

public:
    KeychainBackupDialog(const QString& prompt, QWidget* parent = NULL);

    bytes_t getExtendedKey() const { return extendedKey; }
    void setExtendedKey(const bytes_t& extendedKey);

private:
    bytes_t extendedKey;
    QTextEdit* keychainBackupEdit;
};

