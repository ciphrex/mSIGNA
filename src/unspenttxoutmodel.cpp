///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// unspenttxoutmodel.cpp
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "unspenttxoutmodel.h"

#include <CoinQ/CoinQ_script.h>
#include <CoinQ/CoinQ_netsync.h>

#include <QStandardItemModel>

#include "settings.h"
#include "coinparams.h"

#include "severitylogger.h"

using namespace CoinDB;
using namespace CoinQ::Script;
using namespace std;

UnspentTxOutModel::UnspentTxOutModel(QObject* parent)
    : QStandardItemModel(parent)
{
    base58_versions[0] = getCoinParams().pay_to_pubkey_hash_version();
    base58_versions[1] = getCoinParams().pay_to_script_hash_version();

    //currency_divisor = getCoinParams().currency_divisor();
    //currency_symbol = getCoinParams().currency_symbol();

    initColumns();
}

UnspentTxOutModel::UnspentTxOutModel(CoinDB::Vault* vault, const QString& accountName, QObject* parent)
    : QStandardItemModel(parent)
{
    base58_versions[0] = getCoinParams().pay_to_pubkey_hash_version();
    base58_versions[1] = getCoinParams().pay_to_script_hash_version();

    //currency_divisor = getCurrencyDivisor();
    //currency_symbol = getCurrencySymbol();

    initColumns();
    setVault(vault);
    setAccount(accountName);
}

void UnspentTxOutModel::initColumns()
{
    QStringList columns;
    columns << (tr("Amount") + " (" + getCurrencySymbol() + ")") << tr("Address") << tr("Confirmations");
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

        //QString amount(QString::number(item.value/(1.0 * currency_divisor), 'g', 8));
        QString amount(getFormattedCurrencyAmount(item.value));
        QStandardItem* amountItem = new QStandardItem(amount);
        std::stringstream strAmount;
        strAmount << item.value;
        amountItem->setData(QString::fromStdString(strAmount.str()), Qt::UserRole);

        QString address(QString::fromStdString(getAddressForTxOutScript(item.script, base58_versions)));
        QStandardItem* addressItem = new QStandardItem(address);
        addressItem->setData((int)item.id, Qt::UserRole);

        uint32_t nConfirmations = 0;
        if (bestHeight && item.height) { nConfirmations = bestHeight + 1 - item.height; }
        QString confirmations(QString::number(nConfirmations));

        row.append(amountItem);
        row.append(addressItem);
        row.append(new QStandardItem(confirmations));
        appendRow(row);
    }
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

