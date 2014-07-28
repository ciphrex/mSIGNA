///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// currencyunitdialog.h
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

class QComboBox;

#include <QDialog>

class CurrencyUnitDialog : public QDialog
{
    Q_OBJECT

public:
    CurrencyUnitDialog(QWidget* parent = nullptr);

    QString getCurrencyUnit() const;
    QString getCurrencyUnitPrefix() const;

private:
    QComboBox* currencyUnitBox;

    void populateSelectorBox();
};

