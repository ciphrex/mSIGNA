///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// accountpendingtxmodel.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "accountpendingtxmodel.h"

#include <QStandardItemModel>

#include <QDateTime>

#include "settings.h"

using namespace CoinDB;
using namespace CoinQ::Script;
using namespace std;

AccountPendingTxModel::AccountPendingTxModel(QObject* parent)
    : QStandardItemModel(parent)
{
    initColumns();
}

AccountPendingTxModel::AccountPendingTxModel(CoinDB::Vault* vault, const QString& accountName, QObject* parent)
    : QStandardItemModel(parent)
{
    initColumns();
    setVault(vault);
    setAccount(accountName);
}

void AccountPendingTxModel::initColumns()
{
    QStringList columns;
    columns << tr("Time") << tr("Description") << tr("Type") << tr("Amount") << tr("Fee") << tr("Balance") << tr("Confirmations") << tr("Address") << tr("Transaction ID");
    setHorizontalHeaderLabels(columns);
}

void AccountPendingTxModel::setVault(CoinDB::Vault* vault)
{
    this->vault = vault;
    accountName.clear();
}

void AccountPendingTxModel::setAccount(const QString& accountName)
{
    if (!vault) {
        throw std::runtime_error("Vault is not open.");
    }

    if (!vault->accountExists(accountName.toStdString())) {
        throw std::runtime_error("Account not found.");
    }

    this->accountName = accountName;
}

void AccountPendingTxModel::update()
{
    removeRows(0, rowCount());

    if (!vault || accountName.isEmpty()) return;

    std::shared_ptr<BlockHeader> bestHeader = vault->getBestBlockHeader();

    std::vector<std::shared_ptr<AccountTxOutView>> pendingtx = vault->getAccountPendingTx(accountName.toStdString(), TxOut::DEBIT | TxOut::CREDIT, Tx::SENT | Tx::RECEIVED);
    int64_t balance = 0;
    bytes_t last_txhash;
    for (auto& item: pendingtx) {
        QList<QStandardItem*> row;

        QDateTime utc;
        utc.setTime_t(item->txtimestamp);
        QString time;
        if (item->txtimestamp != 0xffffffff) {
            QDateTime utc;
            utc.setTime_t(item->txtimestamp);
            time = utc.toLocalTime().toString();
        }
        else {
            time = tr("Not Available");
        }

        QString description;
        if (item->description) {
            description = QString::fromStdString(item->description.get());
        }
        else {
            description = tr("Not Available");
        }

        // The type stuff is just to test the new db schema. It's all wrong, we're not going to use TxOutViews for this.
        QString type;
        QString amount;
        QString fee;
        switch (item->type) {
        case TxOut::NONE:
            type = tr("None");
            break;

        case TxOut::CHANGE:
            type = tr("Change");
            break;

        case TxOut::DEBIT:
            type = tr("Sent");
            amount = "-";
            balance -= item->value;
            if (item->have_fee && item->fee > 0) {
                if (item->txhash != last_txhash) {
                    fee = "-";
                    fee += QString::number(item->fee/100000000.0, 'g', 8);
                    balance -= item->fee;
                    last_txhash = item->txhash;
                }
                else {
                    fee = "||";
                }
            }
            break;

        case TxOut::CREDIT:
            type = tr("Received");
            amount = "+";
            balance += item->value;
            break;

        default:
            type = tr("Unknown");
        }

        amount += QString::number(item->value/100000000.0, 'g', 8);
        QString runningBalance = QString::number(balance/100000000.0, 'g', 8);

        QString confirmations;
        if (bestHeader && item->height) {
            confirmations = QString::number(bestHeader->height() + 1 - item->height.get());
        }
        else {
            confirmations = "0";
        }

        QString address = QString::fromStdString(getAddressForTxOutScript(item->script, getDefaultSettings().getBase58Versions()));
        QString txid = QString::fromStdString(uchar_vector(item->txhash).getHex());

        row.append(new QStandardItem(time));
        row.append(new QStandardItem(description));
        row.append(new QStandardItem(type));
        row.append(new QStandardItem(amount));
        row.append(new QStandardItem(fee));
        row.append(new QStandardItem(runningBalance));
        row.append(new QStandardItem(confirmations));
        row.append(new QStandardItem(address));
        row.append(new QStandardItem(txid));

        appendRow(row);
    }    
}
