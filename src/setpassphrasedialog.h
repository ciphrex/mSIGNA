///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// setpassphrasedialog.h
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

class QLineEdit;

#include <QDialog>

#include <CoinQ/CoinQ_typedefs.h>

class SetPassphraseDialog : public QDialog
{
    Q_OBJECT

public:
    SetPassphraseDialog(const QString& objectName, const QString& additionalText, QWidget* parent = NULL);

    QString getPassphrase() const;

private:
    QLineEdit* passphrase1Edit;
    QLineEdit* passphrase2Edit;
};
