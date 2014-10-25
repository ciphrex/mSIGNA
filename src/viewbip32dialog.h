///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// viewbip32dialog.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

#include <CoinQ/CoinQ_typedefs.h>

#include <QDialog>

class ViewBIP32Dialog : public QDialog
{
    Q_OBJECT

public:
    ViewBIP32Dialog(const QString& name, const secure_bytes_t& extendedKey, QWidget* parent = NULL);
};

