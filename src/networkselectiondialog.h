///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// networkselectiondialog.h
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

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

