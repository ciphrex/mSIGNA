///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// resyncdialog.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

class QLineEdit;

#include <QDialog>

class ResyncDialog : public QDialog
{
    Q_OBJECT

public:
    ResyncDialog(int resyncHeight, int maxHeight, QWidget* parent = NULL);

    int getResyncHeight() const;

private:
    QLineEdit* resyncHeightEdit;
};

