///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// currencyunitdialog.h
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

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

