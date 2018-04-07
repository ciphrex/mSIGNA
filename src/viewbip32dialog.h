///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// viewbip32dialog.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include <CoinQ/CoinQ_typedefs.h>

#include <QDialog>

class ViewBIP32Dialog : public QDialog
{
    Q_OBJECT

public:
    ViewBIP32Dialog(const QString& name, const secure_bytes_t& extendedKey, QWidget* parent = NULL);
};

