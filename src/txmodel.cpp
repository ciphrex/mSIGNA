///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// txmodel.cpp
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "txmodel.h"

#include <CoinDB/SynchedVault.h>

#include <CoinQ/CoinQ_script.h>

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
    setBase58Versions();
    currencySymbol = getCurrencySymbol();

    setColumns();
}

TxModel::TxModel(CoinDB::Vault* vault, const QString& accountName, QObject* parent)
    : QStandardItemModel(parent)
{
    setBase58Versions();
    currencySymbol = getCurrencySymbol();

    setColumns();
    setVault(vault);
    setAccount(accountName);
}

void TxModel::setBase58Versions()
{
    base58_versions[0] = getCoinParams().pay_to_pubkey_hash_version();
#ifdef SUPPORT_OLD_ADDRESS_VERSIONS
    base58_versions[1] = getUseOldAddressVersions() ? getCoinParams().old_pay_to_script_hash_version() : getCoinParams().pay_to_script_hash_version();
#else
    base58_versions[1] = getCoinParams().pay_to_script_hash_version();
#endif
}

void TxModel::setColumns()
{
    QStringList columns;
    columns
        << tr("Time")
        << tr("Description")
        << tr("Type")
        << (tr("Amount") + " (" + getCurrencySymbol() + ")")
        << (tr("Fee") + " (" + getCurrencySymbol() + ")")
        << (tr("Balance") + " (" + getCurrencySymbol() + ")")
        << tr("Confirmations")
        << tr("Address")
        << tr("Transaction Hash")
        << tr("Size")
        << tr("VSize");
    setHorizontalHeaderLabels(columns);
}

void TxModel::setVault(CoinDB::Vault* vault)
{
    this->vault = vault;
    accountName.clear();
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

class SortableRow
{
public:
    SortableRow(const QList<QStandardItem*>& row, int status, uint32_t nConfirmations, int64_t value, uint32_t txindex) :
        row_(row), status_(status), nConfirmations_(nConfirmations), value_(value), txindex_(txindex) { }

    QList<QStandardItem*>& row() { return row_; }

    int status() const { return status_; }
    uint32_t nConfirmations() const { return nConfirmations_; }
    int64_t value() const { return value_; }
    uint32_t txindex() const { return txindex_; }

private:
    QList<QStandardItem*> row_;

    int status_;
    uint32_t nConfirmations_;
    int64_t value_;
    uint32_t txindex_;
};

void TxModel::update()
{
    setBase58Versions();

    QString newCurrencySymbol = getCurrencySymbol();
    if (newCurrencySymbol != currencySymbol)
    {
        currencySymbol = newCurrencySymbol;
        setColumns();
    }

    removeRows(0, rowCount());

    if (!vault || accountName.isEmpty()) return;

    std::shared_ptr<BlockHeader> bestHeader = vault->getBestBlockHeader();

    std::vector<TxOutView> txoutviews = vault->getTxOutViews(accountName.toStdString(), "", TxOut::ROLE_BOTH, TxOut::BOTH, Tx::ALL, true);
    bytes_t last_txhash;
    QList<SortableRow> rows;
    for (auto& item: txoutviews) {
        QList<QStandardItem*> row;

        QDateTime utc;
        utc.setTime_t(item.tx_timestamp);
        QString time = utc.toLocalTime().toString();

        QString description = QString::fromStdString(item.role_label());

        // The type stuff is just to test the new db schema. It's all wrong, we're not going to use TxOutViews for this.
        TxType txType;
        QString type;
        QString amount;
        QString fee;
        int64_t value = 0;
        bytes_t this_txhash = item.tx_status == Tx::UNSIGNED ? item.tx_unsigned_hash : item.tx_hash;
        switch (item.role_flags) {
        case TxOut::ROLE_NONE:
            txType = NONE;
            type = tr("None");
            break;

        case TxOut::ROLE_SENDER:
            txType = SEND;
            type = tr("Send");
            amount = "-";
            value -= item.value;
            if (item.tx_has_all_outpoints && item.tx_fee() > 0) {
                if (this_txhash != last_txhash) {
                    fee = "-";
                    //fee += QString::number(item.tx_fee()/(1.0 * currency_divisor), 'g', 8);
                    fee += getFormattedCurrencyAmount(item.tx_fee());
                    value -= item.tx_fee();
                    last_txhash = this_txhash;
                }
                else {
                    fee = "||";
                }
            }
            break;

        case TxOut::ROLE_RECEIVER:
            txType = RECEIVE;
            type = tr("Receive");
            amount = "+";
            value += item.value;
            break;

        default:
            txType = UNKNOWN;
            type = tr("Unknown");
        }

        //amount += QString::number(item.value/(1.0 * currency_divisor), 'g', 8);
        amount += getFormattedCurrencyAmount(item.value);

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
        confirmationsItem->setData((int)nConfirmations, Qt::UserRole + 1);

        QString address = QString::fromStdString(getAddressForTxOutScript(item.script, base58_versions));
        QString hash = QString::fromStdString(uchar_vector(this_txhash).getHex());

        row.append(new QStandardItem(time));
        row.append(new QStandardItem(description));

        QStandardItem* typeItem = new QStandardItem(type);
        typeItem->setData(txType, Qt::UserRole);
        row.append(typeItem);

        row.append(new QStandardItem(amount));
        row.append(new QStandardItem(fee));
        row.append(new QStandardItem("")); // placeholder for balance, once sorted
        row.append(confirmationsItem);
        row.append(new QStandardItem(address));

        // Store the tx hash and tx index to uniquely identify the output.
        QStandardItem* hashItem = new QStandardItem(hash);
        hashItem->setData(item.tx_index, Qt::UserRole);
        row.append(hashItem);

        // Add size and vsize
        std::shared_ptr<Tx> tx = vault->getTx(item.tx_id);
        Coin::Transaction core_tx = tx->toCoinCore();
LOGGER(trace) << "Size: " << core_tx.getSize(true) << endl;
LOGGER(trace) << "VSize: " << core_tx.getVSize() << endl;
        QStandardItem* sizeItem = new QStandardItem(QString::number(core_tx.getSize(true)));
        QStandardItem* vsizeItem = new QStandardItem(QString::number(core_tx.getVSize()));
        row.append(sizeItem);
        row.append(vsizeItem);

        rows.append(SortableRow(row, item.tx_status, nConfirmations, value, item.tx_index));
    }

    qSort(rows.begin(), rows.end(), [](const SortableRow& a, const SortableRow& b) {
        // order by status first (unsigned, then propagated, then confirmed)
        if (a.status() < b.status()) return true;
        if (a.status() > b.status()) return false;

        // if confirmation counts are equal
        if (a.nConfirmations() == b.nConfirmations()) {
            // if one value is positive and the other is negative, sort so that running balance remains positive
            if (a.value() < 0 && b.value() > 0) return true;
            if (a.value() > 0 && b.value() < 0) return false;

            // otherwise sort by ascending tx index
            return (a.txindex() < b.txindex());
        }

        // otherwise sort by ascending confirmation count
        return (a.nConfirmations() < b.nConfirmations());
    });

    // iterate in reverse order to compute running balance
    int64_t balance = 0;
    for (int i = rows.size() - 1; i >= 0; i--) {
        balance += rows[i].value();
        (rows[i].row())[5]->setText(getFormattedCurrencyAmount(balance));
    }

    // iterate in forward order to display
    for (auto& row: rows) appendRow(row.row());
}

bytes_t TxModel::getTxHash(int row) const
{
    if (row == -1 || row >= rowCount()) throw std::runtime_error(tr("Invalid row.").toStdString());

    QStandardItem* txHashItem = item(row, 8);
    uchar_vector txhash;
    txhash.setHex(txHashItem->text().toStdString());
    return txhash;
}

int TxModel::getTxStatus(int row) const
{
    if (row == -1 || row >= rowCount()) throw std::runtime_error(tr("Invalid row.").toStdString());

    QStandardItem* confirmationsItem = item(row, 6);
    return confirmationsItem->data(Qt::UserRole).toInt();
}

int TxModel::getTxConfirmations(int row) const
{
    if (row == -1 || row >= rowCount()) throw std::runtime_error(tr("Invalid row.").toStdString());

    QStandardItem* confirmationsItem = item(row, 6);
    return confirmationsItem->data(Qt::UserRole + 1).toInt();
}

int TxModel::getTxOutType(int row) const
{
    if (row == -1 || row >= rowCount()) throw std::runtime_error(tr("Invalid row.").toStdString());

    QStandardItem* typeItem = item(row, 2);
    return typeItem->data(Qt::UserRole).toInt();
}

QString TxModel::getTxOutAddress(int row) const
{
    if (row == -1 || row >= rowCount()) throw std::runtime_error(tr("Invalid row.").toStdString());

    QStandardItem* typeItem = item(row, 7);
    return typeItem->text();
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

    QString msg;
    if (keychainNames.empty())
    {
        msg = tr("No new signatures were added.");
    }
    else
    {
        msg = tr("Signatures added using keychain(s) ") + QString::fromStdString(stdutils::delimited_list(keychainNames, ", ")) + tr(".");
    }
    emit txSigned(msg);
}

void TxModel::sendTx(int row, CoinDB::SynchedVault* synchedVault)
{
    if (row == -1 || row >= rowCount()) {
        throw std::runtime_error(tr("Invalid row.").toStdString());
    }

    if (!synchedVault->isConnected()) {
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
    synchedVault->sendTx(coin_tx);

    // TODO: Check transaction has propagated before changing status to PROPAGATED
//    tx->updateStatus(CoinDB::Tx::PROPAGATED);
//    vault->insertTx(tx);

//    update();
    //networkSync->getTx(txhash);
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
    if (role == Qt::TextAlignmentRole && index.column() >= 3 && index.column() <= 6)
    {
        return Qt::AlignRight;
    }
    else if (role == Qt::BackgroundRole)
    {
        QBrush brush;

        int txStatus = getTxStatus(index.row());
        int txOutType = getTxOutType(index.row());

        if (txStatus == Tx::UNSIGNED && txOutType == SEND)
        {
            return QBrush(QColor(190, 220, 200), Qt::SolidPattern); // teal
        }
        else
        {
            QBrush brush;

            switch (txOutType)
            {
            case RECEIVE:
                brush.setColor(QColor(230, 230, 230)); // light gray
                break;

            case SEND:
                brush.setColor(QColor(184, 207, 233)); // light blue
                break;
            }


            if (txStatus >= Tx::CONFIRMED)
            {
                switch (getTxConfirmations(index.row()))
                {
                case 0:
                    brush.setStyle(Qt::Dense6Pattern); // this should never happen - case 0 is unconfirmed. Just here to be safe.
                    break;
                case 1:
                    brush.setStyle(Qt::Dense5Pattern);
                    break;
                case 2:
                    brush.setStyle(Qt::Dense4Pattern);
                    break;
                case 3:
                    brush.setStyle(Qt::Dense3Pattern);
                    break;
                case 4:
                    brush.setStyle(Qt::Dense2Pattern);
                    break;
                case 5:
                    brush.setStyle(Qt::Dense1Pattern);
                    break;
                default:
                    brush.setStyle(Qt::SolidPattern);
                }
            }
            else if (txStatus == Tx::PROPAGATED)
            {
                brush.setStyle(Qt::Dense6Pattern);
            } 
            else if (txStatus == Tx::UNSENT)
            {
                brush.setStyle(Qt::Dense7Pattern);
            }
            else
            {
                brush.setStyle(Qt::NoBrush);
            }

            return brush;
        }
    }
 
    return QStandardItemModel::data(index, role);
}

bool TxModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (role == Qt::EditRole) {
        if (index.column() == 1) {
            // Keychain name edited.
            if (!vault) return false;
 
            try {
                // Get account role (sender/receiver)
                QStandardItem* typeItem = item(index.row(), 2);
                int txtype = typeItem->data(Qt::UserRole).toInt();                
                if (txtype != SEND && txtype != RECEIVE) return false;

                // Get outpoint information
                QStandardItem* txHashItem = item(index.row(), 8);
                uchar_vector txhash;
                txhash.setHex(txHashItem->text().toStdString());
                uint32_t txindex = (uint32_t)txHashItem->data(Qt::UserRole).toInt();

                if (txtype == SEND)
                {
                    vault->setSendingLabel(txhash, txindex, value.toString().toStdString());
                }
                else
                {
                    vault->setReceivingLabel(txhash, txindex, value.toString().toStdString());
                }

                setItem(index.row(), index.column(), new QStandardItem(value.toString()));
                return true;
            }
            catch (const std::exception& e) {
                emit error(QString::fromStdString(e.what()));
            }
        }
        return false;
    }
 
    return true;
}

Qt::ItemFlags TxModel::flags(const QModelIndex& index) const
{
    if (index.column() == 1) {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
    }
 
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

