///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// scriptdialog.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

#include <CoinDB/Vault.h>

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

