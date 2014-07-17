///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// networkselectiondialog.h
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

namespace CoinQ { class NetworkSelector; }
class QComboBox;

#include <QDialog>

class NetworkSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    NetworkSelectionDialog(CoinQ::NetworkSelector& selector, QWidget* parent = NULL);

    QString getNetworkName() const;

private:
    QComboBox* networkBox;
    void populateNetworkBox(CoinQ::NetworkSelector& selector);
};

