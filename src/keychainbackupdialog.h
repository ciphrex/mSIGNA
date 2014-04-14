///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// keychainbackupdialog.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef COINVAULT_KEYCHAINBACKUPDIALOG_H
#define COINVAULT_KEYCHAINBACKUPDIALOG_H

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

#endif // COINVAULT_KEYCHAINBACKUPDIALOG_H
