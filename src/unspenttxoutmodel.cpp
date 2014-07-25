///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// unspenttxoutmodel.cpp
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.

#include "unspenttxoutmodel.h"

#include <CoinQ/CoinQ_script.h>
#include <CoinQ/CoinQ_netsync.h>

#include <QStandardItemModel>

#include "settings.h"
#include "coinparams.h"

#include "severitylogger.h"

using namespace CoinDB;
//using namespace CoinQ::Script;
using namespace std;

UnspentTxOutModel::UnspentTxOutModel(QObject* parent)
    : QStandardItemModel(parent)
{
    base58_versions[0] = getCoinParams().pay_to_pubkey_hash_version();
    base58_versions[1] = getCoinParams().pay_to_script_hash_version();

    currency_divisor = getCoinParams().currency_divisor();
    currency_symbol = getCoinParams().currency_symbol();

    initColumns();
}

UnspentTxOutModel::UnspentTxOutModel(CoinDB::Vault* vault, const QString& accountName, QObject* parent)
    : QStandardItemModel(parent)
{
    base58_versions[0] = getCoinParams().pay_to_pubkey_hash_version();
    base58_versions[1] = getCoinParams().pay_to_script_hash_version();

    initColumns();
    setVault(vault);
    setAccount(accountName);
}

void UnspentTxOutModel::initColumns()
{
    QStringList columns;
    columns << tr("Amount") << tr("Confirmations");
    setHorizontalHeaderLabels(columns);
}

void UnspentTxOutModel::setVault(CoinDB::Vault* vault)
{
    this->vault = vault;
    accountName.clear();
}

void UnspentTxOutModel::setAccount(const QString& accountName)
{
    if (!vault) throw std::runtime_error("Vault is not open.");

    if (!vault->accountExists(accountName.toStdString()))
        throw std::runtime_error("Account not found.");

    this->accountName = accountName;
}

void UnspentTxOutModel::update()
{
    removeRows(0, rowCount());

    if (!vault || accountName.isEmpty()) return;

    uint32_t bestHeight = vault->getBestHeight();

    std::vector<TxOutView> txoutviews = vault->getUnspentTxOutViews(accountName.toStdString());
    for (auto& item: txoutviews) {
        QList<QStandardItem*> row;
        QString id(QString::number(item.id));
        QString amount(QString::number(item.value/(1.0 * currency_divisor), 'g', 8));

        uint32_t nConfirmations = 0;
        if (bestHeight && item.height) { nConfirmations = bestHeight + 1 - item.height; }
        QString confirmations(QString::number(nConfirmations));

        //QString address = QString::fromStdString(getAddressForTxOutScript(item.script, base58_versions));

        row.append(new QStandardItem(id));
        row.append(new QStandardItem(amount));
        row.append(new QStandardItem(confirmations));
        //row.append(new QStandardItem(address));
        //rows.append(std::make_pair(std::make_pair(item.id, nConfirmations), std::make_pair(row, item.value)));
        appendRow(row);
    }

    // iterate in forward order to display
    //for (auto& row: rows) appendRow(row.second.first);
}

QVariant UnspentTxOutModel::data(const QModelIndex& index, int role) const
{
    // Right-align numeric fields
    if (role == Qt::TextAlignmentRole) {// && index.column() >= 3 && index.column() <= 6) {
        return Qt::AlignRight;
    }
    else {
        return QStandardItemModel::data(index, role);
    }
}

Qt::ItemFlags UnspentTxOutModel::flags(const QModelIndex& /*index*/) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

