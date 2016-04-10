///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// importbip32dialog.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

#include <CoinQ/CoinQ_typedefs.h>

#include <QDialog>

class QLineEdit;

class ImportBIP32Dialog : public QDialog
{
    Q_OBJECT

public:
    ImportBIP32Dialog(QWidget* parent = NULL);

    QString getName() const;
    secure_bytes_t getExtendedKey() const;

private:
    QLineEdit* m_nameEdit;
    QLineEdit* m_base58Edit;
};

