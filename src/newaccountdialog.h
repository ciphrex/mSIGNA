///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// newaccountdialog.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef COINVAULT_NEWACCOUNTDIALOG_H
#define COINVAULT_NEWACCOUNTDIALOG_H

class QLineEdit;
class QComboBox;

#include <QDialog>

class NewAccountDialog : public QDialog
{
    Q_OBJECT

public:
    NewAccountDialog(const QList<QString>& keychainNames, QWidget* parent = NULL);

    QString getName() const;
    const QList<QString>& getKeychainNames() const { return keychainNames; }
    int getMinSigs() const;

private:
    QLineEdit* nameEdit;
    QLineEdit* keychainEdit;
    QComboBox* minSigComboBox;
    QList<QString> keychainNames;
};

#endif // COINVAULT_NEWACCOUNTDIALOG_H
