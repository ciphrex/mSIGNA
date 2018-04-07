///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// newkeychaindialog.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

class QLineEdit;

#include <QDialog>

class NewKeychainDialog : public QDialog
{
    Q_OBJECT

public:
    NewKeychainDialog(QWidget* parent = NULL);

    static const int DEFAULT_NUMKEYS = 100;

    QString getName() const;
    int getNumKeys() const;

private:
    QLineEdit* nameEdit;
//    QLineEdit* numKeysEdit;
};

