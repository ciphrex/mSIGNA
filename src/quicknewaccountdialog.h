///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// quicknewaccountdialog.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

class QLineEdit;
class QComboBox;

#include <QDialog>

class QuickNewAccountDialog : public QDialog
{
    Q_OBJECT

public:
    QuickNewAccountDialog(QWidget* parent = NULL);

    QString getName() const;
    int getMinSigs() const;
    int getMaxSigs() const;

private:
    QLineEdit* nameEdit;
    QComboBox* minSigComboBox;
    QComboBox* maxSigComboBox;
};

