///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// setpassphrasedialog.h
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.

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
