///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// keychainbackupdialog.h
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

