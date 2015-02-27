///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// scriptdialog.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include "settings.h"

#include "scriptdialog.h"

// Models/Views
#include "scriptmodel.h"
#include "scriptview.h"

#include <QVBoxLayout>

ScriptDialog::ScriptDialog(CoinDB::Vault* vault, const QString& accountName, QWidget* parent)
    : QDialog(parent)
{
    resize(QSize(800, 400));

    scriptModel = new ScriptModel(vault, accountName, this);
    scriptModel->update();

    scriptView = new ScriptView(this);
    scriptView->setModel(scriptModel);
    scriptView->update();

    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->addWidget(scriptView);
    setLayout(mainLayout);
    setWindowTitle(getDefaultSettings().getAppName() + " - " + accountName + tr(" addresses"));
}
