///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// viewbip39dialog.h
//
// Copyright (c) 2013-2015 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

#include <CoinQ/CoinQ_typedefs.h>

#include <QDialog>

class ViewBIP39Dialog : public QDialog
{
    Q_OBJECT

public:
    ViewBIP39Dialog(const QString& name, const secure_bytes_t& seed, QWidget* parent = NULL);
};

