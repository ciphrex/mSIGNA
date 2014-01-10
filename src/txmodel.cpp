///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// txmodel.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include "txmodel.h"

#include <CoinQ_netsync.h>

#include <QStandardItemModel>
#include <QDateTime>
#include <QMessageBox>

#include "settings.h"

#include "severitylogger.h"

using namespace CoinQ::Vault;
using namespace CoinQ::Script;
using namespace std;

TxModel::TxModel(QObject* parent)
    : QStandardItemModel(parent)
{
    initColumns();
}

TxModel::TxModel(CoinQ::Vault::Vault* vault, const QString& accountName, QObject* parent)
    : QStandardItemModel(parent)
{
    initColumns();
    setVault(vault);
    setAccount(accountName);
}

void TxModel::initColumns()
{
    QStringList columns;
    columns << tr("Time") << tr("Description") << tr("Type") << tr("Amount") << tr("Fee") << tr("Balance") << tr("Confirmations") << tr("Address") << tr("Transaction ID");
    setHorizontalHeaderLabels(columns);
}

void TxModel::setVault(CoinQ::Vault::Vault* vault)
{
    this->vault = vault;
    accountName.clear();
    update();
}

void TxModel::setAccount(const QString& accountName)
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

void TxModel::update()
{
    removeRows(0, rowCount());

    if (!vault || accountName.isEmpty()) return;

    std::shared_ptr<BlockHeader> bestHeader = vault->getBestBlockHeader();

    std::vector<std::shared_ptr<AccountTxOutView>> history = vault->getAccountHistory(accountName.toStdString(), TxOut::DEBIT | TxOut::CREDIT, Tx::ALL);
    bytes_t last_txhash;
    typedef std::pair<unsigned long, uint32_t> sorting_info_t;
    typedef std::pair<QList<QStandardItem*>, int64_t> value_row_t; // used to associate output values with rows for computing balance
    typedef std::pair<sorting_info_t, value_row_t> sortable_row_t;
    QList<sortable_row_t> rows;
    for (auto& item: history) {
        QList<QStandardItem*> row;

        QString time;
        if (item->block_timestamp) {
            // If in block, use block timestamp.
            QDateTime utc;
            utc.setTime_t(item->block_timestamp.get());
            time = utc.toLocalTime().toString();
        }
        else if (item->txtimestamp != 0xffffffff) {
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
        int64_t value = 0;
        switch (item->type) {
        case TxOut::NONE:
            type = tr("None");
            break;

        case TxOut::CHANGE:
            type = tr("Change");
            break;

        case TxOut::DEBIT:
            type = tr("Send");
            amount = "-";
            value -= item->value;
            if (item->have_fee && item->fee > 0) {
                if (item->txhash != last_txhash) {
                    fee = "-";
                    fee += QString::number(item->fee/100000000.0, 'g', 8);
                    value -= item->fee;
                    last_txhash = item->txhash;
                }
                else {
                    fee = "||";
                }
            }
            break;

        case TxOut::CREDIT:
            type = tr("Receive");
            amount = "+";
            value += item->value;
            break;

        default:
            type = tr("Unknown");
        }

        amount += QString::number(item->value/100000000.0, 'g', 8);

        uint32_t nConfirmations = 0;
        QString confirmations;
        if (item->txstatus == Tx::RECEIVED) {
            if (bestHeader && item->height) {
                nConfirmations = bestHeader->height() + 1 - item->height.get();
                confirmations = QString::number(nConfirmations);
            }
            else {
                confirmations = "0";
            }
        }
        else if (item->txstatus == Tx::UNSIGNED) {
            confirmations = tr("Unsigned");
        }
        else if (item->txstatus == Tx::UNSENT) {
            confirmations = tr("Unsent");
        }
        QStandardItem* confirmationsItem = new QStandardItem(confirmations);
        confirmationsItem->setData(item->txstatus, Qt::UserRole);

        QString address = QString::fromStdString(getAddressForTxOutScript(item->script, BASE58_VERSIONS));
        QString txhash = QString::fromStdString(uchar_vector(item->txhash).getHex());

        row.append(new QStandardItem(time));
        row.append(new QStandardItem(description));
        row.append(new QStandardItem(type));
        row.append(new QStandardItem(amount));
        row.append(new QStandardItem(fee));
        row.append(new QStandardItem("")); // placeholder for balance, once sorted
        row.append(confirmationsItem);
        row.append(new QStandardItem(address));
        row.append(new QStandardItem(txhash));

        rows.append(std::make_pair(std::make_pair(item->txid, nConfirmations), std::make_pair(row, value)));
    }

    qSort(rows.begin(), rows.end(), [](const sortable_row_t& a, const sortable_row_t& b) {
        sorting_info_t a_info = a.first;
        sorting_info_t b_info = b.first;

        // if confirmation counts are equal
        if (a_info.second == b_info.second) {
            // if one value is positive and the other is negative, sort so that running balance remains positive
            if (a.second.second < 0 && b.second.second > 0) return true;
            if (a.second.second > 0 && b.second.second < 0) return false;

            // otherwise sort by descending index
            return (a_info.first > b_info.first);
        }

        // otherwise sort by ascending confirmation count
        return (a_info.second < b_info.second);
    });

    // iterate in reverse order to compute running balance
    int64_t balance = 0;
    for (int i = rows.size() - 1; i >= 0; i--) {
        balance += rows[i].second.second;
        (rows[i].second.first)[5]->setText(QString::number(balance/100000000.0, 'g', 8));
    }

    // iterate in forward order to display
    for (auto& row: rows) appendRow(row.second.first);
}

void TxModel::signTx(int row)
{
    if (row == -1 || row >= rowCount()) {
        throw std::runtime_error(tr("Invalid row.").toStdString());
    }

    QStandardItem* typeItem = item(row, 6);
    int type = typeItem->data(Qt::UserRole).toInt();
    if (type != CoinQ::Vault::Tx::UNSIGNED) {
        throw std::runtime_error(tr("Transaction is already signed.").toStdString());
    }

    QStandardItem* txHashItem = item(row, 8);
    uchar_vector txhash;
    txhash.setHex(txHashItem->text().toStdString());

    std::shared_ptr<Tx> tx = vault->getTx(txhash);
    tx = vault->signTx(tx);

    LOGGER(trace) << uchar_vector(tx->raw()).getHex() << std::endl;

    vault->addTx(tx, true);

    update();
}

void TxModel::sendTx(int row, CoinQ::Network::NetworkSync* networkSync)
{
    if (row == -1 || row >= rowCount()) {
        throw std::runtime_error(tr("Invalid row.").toStdString());
    }

    if (!networkSync || !networkSync->isConnected()) {
        throw std::runtime_error(tr("Must be connected to network to send.").toStdString());
    }

    QStandardItem* typeItem = item(row, 6);
    int type = typeItem->data(Qt::UserRole).toInt();
    if (type == CoinQ::Vault::Tx::UNSIGNED) {
        throw std::runtime_error(tr("Transaction must be fully signed before sending.").toStdString());
    }
    else if (type == CoinQ::Vault::Tx::RECEIVED) {
        throw std::runtime_error(tr("Transaction already sent.").toStdString());
    }

    QStandardItem* txHashItem = item(row, 8);
    uchar_vector txhash;
    txhash.setHex(txHashItem->text().toStdString());

    std::shared_ptr<CoinQ::Vault::Tx> tx = vault->getTx(txhash);
    Coin::Transaction coin_tx = tx->toCoinClasses();
    networkSync->sendTx(coin_tx);

    // TODO: Check transaction has propagated before changing status to RECEIVED
    tx->status(CoinQ::Vault::Tx::RECEIVED);
    vault->addTx(tx, true);

    update();
//    networkSync->getTx(txhash);
}

std::shared_ptr<Tx> TxModel::getTx(int row)
{
    if (row == -1 || row >= rowCount()) {
        throw std::runtime_error(tr("Invalid row.").toStdString());
    }

    QStandardItem* txHashItem = item(row, 8);
    uchar_vector txhash;
    txhash.setHex(txHashItem->text().toStdString());

    return vault->getTx(txhash);
}

void TxModel::deleteTx(int row)
{
    if (row == -1 || row >= rowCount()) {
        throw std::runtime_error(tr("Invalid row.").toStdString());
    }

    QStandardItem* txHashItem = item(row, 8);
    uchar_vector txhash;
    txhash.setHex(txHashItem->text().toStdString());

    vault->deleteTx(txhash);
    update();

    emit txDeleted();
}

QVariant TxModel::data(const QModelIndex& index, int role) const
{
    // Right-align numeric fields
    if (role == Qt::TextAlignmentRole && index.column() >= 3 && index.column() <= 6) {
        return Qt::AlignRight;
    }
    else {
        return QStandardItemModel::data(index, role);
    }
}

Qt::ItemFlags TxModel::flags(const QModelIndex& index) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

