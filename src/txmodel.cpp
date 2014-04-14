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

#include <CoinQ/CoinQ_script.h>
#include <CoinQ/CoinQ_netsync.h>

#include <stdutils/stringutils.h>

#include <QStandardItemModel>
#include <QDateTime>
#include <QMessageBox>

#include "settings.h"
#include "coinparams.h"

#include "severitylogger.h"

using namespace CoinDB;
using namespace CoinQ::Script;
using namespace std;

TxModel::TxModel(QObject* parent)
    : QStandardItemModel(parent)
{
    base58_versions[0] = getCoinParams().pay_to_pubkey_hash_version();
    base58_versions[1] = getCoinParams().pay_to_script_hash_version();

    initColumns();
}

TxModel::TxModel(CoinDB::Vault* vault, const QString& accountName, QObject* parent)
    : QStandardItemModel(parent)
{
    base58_versions[0] = getCoinParams().pay_to_pubkey_hash_version();
    base58_versions[1] = getCoinParams().pay_to_script_hash_version();

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

void TxModel::setVault(CoinDB::Vault* vault)
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

    std::vector<TxOutView> txoutviews = vault->getTxOutViews(accountName.toStdString(), "", TxOut::ROLE_BOTH, TxOut::BOTH, Tx::ALL, true);
    bytes_t last_txhash;
    typedef std::pair<unsigned long, uint32_t> sorting_info_t;
    typedef std::pair<QList<QStandardItem*>, int64_t> value_row_t; // used to associate output values with rows for computing balance
    typedef std::pair<sorting_info_t, value_row_t> sortable_row_t;
    QList<sortable_row_t> rows;
    for (auto& item: txoutviews) {
        QList<QStandardItem*> row;

        QDateTime utc;
        utc.setTime_t(item.tx_timestamp);
        QString time = utc.toLocalTime().toString();

        QString description = QString::fromStdString(item.role_label());
        if (description.isEmpty()) description = "Not available";

        // The type stuff is just to test the new db schema. It's all wrong, we're not going to use TxOutViews for this.
        QString type;
        QString amount;
        QString fee;
        int64_t value = 0;
        bytes_t this_txhash = item.tx_status == Tx::UNSIGNED ? item.tx_unsigned_hash : item.tx_hash;
        switch (item.role_flags) {
        case TxOut::ROLE_NONE:
            type = tr("None");
            break;

        case TxOut::ROLE_SENDER:
            type = tr("Send");
            amount = "-";
            value -= item.value;
            if (item.have_fee && item.fee > 0) {
                if (this_txhash != last_txhash) {
                    fee = "-";
                    fee += QString::number(item.fee/100000000.0, 'g', 8);
                    value -= item.fee;
                    last_txhash = this_txhash;
                }
                else {
                    fee = "||";
                }
            }
            break;

        case TxOut::ROLE_RECEIVER:
            type = tr("Receive");
            amount = "+";
            value += item.value;
            break;

        default:
            type = tr("Unknown");
        }

        amount += QString::number(item.value/100000000.0, 'g', 8);

        uint32_t nConfirmations = 0;
        QString confirmations;
        if (item.tx_status >= Tx::PROPAGATED) {
            if (bestHeader && item.height) {
                nConfirmations = bestHeader->height() + 1 - item.height;
                confirmations = QString::number(nConfirmations);
            }
            else {
                confirmations = "0";
            }
        }
        else if (item.tx_status == Tx::UNSIGNED) {
            confirmations = tr("Unsigned");
        }
        else if (item.tx_status == Tx::UNSENT) {
            confirmations = tr("Unsent");
        }
        QStandardItem* confirmationsItem = new QStandardItem(confirmations);
        confirmationsItem->setData(item.tx_status, Qt::UserRole);

        QString address = QString::fromStdString(getAddressForTxOutScript(item.script, base58_versions));
        QString hash = QString::fromStdString(uchar_vector(this_txhash).getHex());

        row.append(new QStandardItem(time));
        row.append(new QStandardItem(description));
        row.append(new QStandardItem(type));
        row.append(new QStandardItem(amount));
        row.append(new QStandardItem(fee));
        row.append(new QStandardItem("")); // placeholder for balance, once sorted
        row.append(confirmationsItem);
        row.append(new QStandardItem(address));
        row.append(new QStandardItem(hash));

        rows.append(std::make_pair(std::make_pair(item.tx_id, nConfirmations), std::make_pair(row, value)));
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
    LOGGER(trace) << "TxModel::signTx(" << row << ")" << std::endl;

    if (row == -1 || row >= rowCount()) {
        throw std::runtime_error(tr("Invalid row.").toStdString());
    }

    QStandardItem* typeItem = item(row, 6);
    int type = typeItem->data(Qt::UserRole).toInt();
    if (type != CoinDB::Tx::UNSIGNED) {
        throw std::runtime_error(tr("Transaction is already signed.").toStdString());
    }

    QStandardItem* txHashItem = item(row, 8);
    uchar_vector txhash;
    txhash.setHex(txHashItem->text().toStdString());

    std::vector<std::string> keychainNames;
    std::shared_ptr<Tx> tx = vault->signTx(txhash, keychainNames, true);
    if (!tx) throw std::runtime_error(tr("No new signatures were added.").toStdString());

    LOGGER(trace) << "TxModel::signTx - signature(s) added. raw tx: " << uchar_vector(tx->raw()).getHex() << std::endl;
    update();

    QString msg = tr("Signatures added using keychain(s) ") + QString::fromStdString(stdutils::delimited_list(keychainNames, ", ")) + tr(".");
    emit txSigned(msg);
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
    if (type == CoinDB::Tx::UNSIGNED) {
        throw std::runtime_error(tr("Transaction must be fully signed before sending.").toStdString());
    }
    else if (type == CoinDB::Tx::PROPAGATED) {
        throw std::runtime_error(tr("Transaction already sent.").toStdString());
    }

    QStandardItem* txHashItem = item(row, 8);
    uchar_vector txhash;
    txhash.setHex(txHashItem->text().toStdString());

    std::shared_ptr<CoinDB::Tx> tx = vault->getTx(txhash);
    Coin::Transaction coin_tx = tx->toCoinCore();
    networkSync->sendTx(coin_tx);

    // TODO: Check transaction has propagated before changing status to PROPAGATED
    tx->updateStatus(CoinDB::Tx::PROPAGATED);
    vault->insertTx(tx);

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

Qt::ItemFlags TxModel::flags(const QModelIndex& /*index*/) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

