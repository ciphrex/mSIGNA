///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// txoutmodel.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "txoutmodel.h"

#include <QStandardItemModel>

using namespace CoinDB;
using namespace std;

TxOutModel::TxOutModel()
    : vault(NULL)
{
    QStringList columns;
    columns << tr("Transaction Hash") << tr("From") << tr("Amount") << tr("Spent");
    setHorizontalHeaderLabels(columns);
}

void TxOutModel::setVault(CoinDB::Vault* vault)
{
    this->vault = vault;
    update();
}

void TxOutModel::setAccount(const QString& accountName)
{
    if (!vault) {
        throw std::runtime_error("Vault is not open.");
    }

    if (!vault->accountExists(accountName.toStdString())) {
        throw std::runtime_error("Account not found.");
    }

    this->accountName = accountName;
    update();
}

void TxOutModel::update()
{
    removeRows(0, rowCount());

    if (!vault || accountName.isEmpty()) return;

    std::vector<std::shared_ptr<TxOutView>> txouts = vault->getTxOutViews(accountName.toStdString(), false);
    for (auto& txout: txouts) {
        QList<QStandardItem*> row;
        QString txhash = QString::fromStdString(uchar_vector(txout->txhash).getHex());
        QString from = QString::fromStdString(txout->signingscript_label);
        QString amount = QString::number(txout->value/100000000.0, 'g', 8);

        row.append(new QStandardItem(txhash));
        row.append(new QStandardItem(from));
        row.append(new QStandardItem(amount));
        row.append(new QStandardItem(""));

        appendRow(row);
    }    
}

/*
    #pragma db column(Account::id_)
    unsigned long account_id;
 
    #pragma db column(Account::name_)
    std::string account_name;
 
    #pragma db column(SigningScript::id_)
    unsigned long signingscript_id;
 
    #pragma db column(SigningScript::label_)
    std::string signingscript_label;
 
    #pragma db column(SigningScript::status_)
    int signingscript_status;
 
    #pragma db column(SigningScript::txinscript_)
    bytes_t signingscript_txinscript;
 
    #pragma db column(TxOut::script_)
    bytes_t script;
 
    #pragma db column(TxOut::value_)
    uint64_t value;
 
    #pragma db column(Tx::hash_)
    bytes_t txhash;
 
    #pragma db column(TxOut::txindex_)
    uint32_t txindex;
*/
