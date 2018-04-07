///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// importbip32dialog.h
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

