///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// viewbip39dialog.h
//
// Copyright (c) 2013-2015 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include <CoinQ/CoinQ_typedefs.h>

#include <QDialog>

class ViewBIP39Dialog : public QDialog
{
    Q_OBJECT

public:
    ViewBIP39Dialog(const QString& name, const secure_bytes_t& seed, QWidget* parent = NULL);
    ViewBIP39Dialog(const QString& prompt, const QString& name, const secure_bytes_t& seed, QWidget* parent = NULL);

protected:
    void init(const QString& prompt, const QString& name, const secure_bytes_t& seed);
};

