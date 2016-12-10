///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// accounthistorydialog.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "settings.h"

#include "accounthistorydialog.h"

// Models/Views
#include "txmodel.h"
#include "txview.h"

#include <QVBoxLayout>

AccountHistoryDialog::AccountHistoryDialog(CoinDB::Vault* vault, const QString& accountName, QWidget* parent)
    : QDialog(parent)
{
    resize(QSize(800, 400));

    accountHistoryModel = new TxModel(vault, accountName, this);
    accountHistoryModel->update();

    accountHistoryView = new TxView(this);
    accountHistoryView->setModel(accountHistoryModel);
    accountHistoryView->update();

    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->addWidget(accountHistoryView);
    setLayout(mainLayout);
    setWindowTitle(getDefaultSettings().getAppName() + " - " + accountName + tr(" account history"));
}
