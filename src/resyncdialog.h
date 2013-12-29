///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// resyncdialog.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef COINVAULT_RESYNCDIALOG_H
#define COINVAULT_RESYNCDIALOG_H

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

#endif // COINVAULT_RESYNCDIALOG_H
