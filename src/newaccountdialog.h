///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// newaccountdialog.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

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

