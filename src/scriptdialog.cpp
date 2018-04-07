///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// scriptdialog.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

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
