///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// scriptdialog.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef COINVAULT_SCRIPTDIALOG_H
#define COINVAULT_SCRIPTDIALOG_H

#include <Vault.h>

class ScriptModel;
class ScriptView;

#include <QDialog>

class ScriptDialog : public QDialog
{
    Q_OBJECT

public:
    ScriptDialog(CoinDB::Vault* vault, const QString& accountName, QWidget* parent = NULL);

private:
    ScriptModel* scriptModel;
    ScriptView* scriptView;
};

#endif // COINVAULT_SCRIPTDIALOG_H
