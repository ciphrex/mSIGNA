///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// newkeychaindialog.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef COINVAULT_NEWKEYCHAINDIALOG_H
#define COINVAULT_NEWKEYCHAINDIALOG_H

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

#endif // COINVAULT_NEWKEYCHAINDIALOG_H
