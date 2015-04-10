///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// importbip39dialog.h
//
// Copyright (c) 2013-2015 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

#include <CoinCore/typedefs.h>

#include <QDialog>

class QLineEdit;

class ImportBIP39Dialog : public QDialog
{
    Q_OBJECT

public:
    ImportBIP39Dialog(QWidget* parent = NULL);

    QString getName() const;
    secure_bytes_t getSeed() const;

private:
    QLineEdit* m_nameEdit;
    QLineEdit* m_wordlistEdit;
};

