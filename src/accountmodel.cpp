///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// accountmodel.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include "settings.h"

#include "accountmodel.h"

#include <CoinQ_netsync.h>

#include <QStandardItemModel>
#include <QFile>

#include "severitylogger.h"

using namespace CoinQ::Vault;
using namespace CoinQ::Script;
using namespace std;

AccountModel::AccountModel()
    : vault(NULL), numAccounts(0)
{
    QStringList columns;
    columns << tr("Account Name") << tr("Policy") << tr("Scripts Remaining") << tr("Balance") << "";
    setHorizontalHeaderLabels(columns);
}


void AccountModel::update()
{
    removeRows(0, rowCount());

    if (!vault) {
        numAccounts = 0;
        return;
    }

    QStringList accountNames;
    std::vector<AccountInfo> accounts = vault->getAccounts();
    for (auto& account: accounts) {
        QString accountName = QString::fromStdString(account.name());
        accountNames << accountName;
        QList<QStandardItem*> row;
        row.append(new QStandardItem(accountName));

        QString m_of_n = QString::number(account.minsigs()) + tr(" of ");
        QString keychainNames;
        bool first = true;
        for (auto& name: account.keychain_names()) {
            if (first)  { first = false; }
            else        { keychainNames += ", "; }
            keychainNames += QString::fromStdString(name);
            if (name.empty()) {
                keychainNames = QString::number(account.keychain_names().size());
                break;
            }
        }
        m_of_n += keychainNames;
        row.append(new QStandardItem(m_of_n));

        row.append(new QStandardItem(QString::number(account.scripts_remaining())));
        QString balance;
        row.append(new QStandardItem(QString::number(account.balance()/100000000.0, 'g', 8)));
        appendRow(row);
    }
    numAccounts = accountNames.size();

    emit updated(accountNames);
}

void AccountModel::create(const QString& fileName)
{
    close ();

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, fileName.toStdString().c_str());
    char* argv[] = {prog, opt, buf};
    vault = new Vault(argc, argv, true);

    emit updateSyncHeight(0);
}

void AccountModel::load(const QString& fileName)
{
    close();

    int argc = 3;
    char prog[] = "prog";
    char opt[] = "--database";
    char buf[255];
    std::strcpy(buf, fileName.toStdString().c_str());
    char* argv[] = {prog, opt, buf};
    vault = new Vault(argc, argv, false);
    update();

    emit updateSyncHeight(vault->getBestHeight());
}

void AccountModel::close()
{
    if (vault) {
        delete vault;
        vault = NULL;
        update();

        emit updateSyncHeight(0);
    }
}

Coin::BloomFilter AccountModel::getBloomFilter(double falsePositiveRate, uint32_t nTweak, uint32_t nFlags) const
{
    if (!vault) return Coin::BloomFilter();
    return vault->getBloomFilter(falsePositiveRate, nTweak, nFlags);
}

void AccountModel::newKeychain(const QString& name, unsigned long numkeys)
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    vault->newKeychain(name.toStdString(), numkeys);
    update();
}

void AccountModel::newAccount(const QString& name, unsigned int minsigs, const QList<QString>& keychainNames)
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    std::vector<std::string> keychain_names;
    for (auto& name: keychainNames) { keychain_names.push_back(name.toStdString()); }

    vault->newAccount(name.toStdString(), minsigs, keychain_names);
    update();
}

bool AccountModel::accountExists(const QString& name) const
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    return vault->accountExists(name.toStdString());
}

void AccountModel::exportAccount(const QString& name, const QString& filePath) const
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    vault->exportAccount(name.toStdString(), filePath.toStdString());
}

void AccountModel::importAccount(const QString& name, const QString& filePath)
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    vault->importAccount(name.toStdString(), filePath.toStdString());
    update();
}

void AccountModel::deleteAccount(const QString& name)
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    vault->eraseAccount(name.toStdString());
    update();
} 

QPair<QString, bytes_t> AccountModel::issueNewScript(const QString& accountName, const QString& label)
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    std::shared_ptr<TxOut> txout = vault->newTxOut(accountName.toStdString(), label.toStdString());
    QString address = QString::fromStdString(getAddressForTxOutScript(txout->script(), BASE58_VERSIONS));
    update();

    return qMakePair(address, txout->script());
}

uint32_t AccountModel::getFirstAccountTimeCreated() const
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    return vault->getFirstAccountTimeCreated();
}

bool AccountModel::insertRawTx(const bytes_t& rawTx)
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    std::shared_ptr<Tx> tx(new Tx());
    tx->set(rawTx);
    tx->timestamp(time(NULL));
    if (vault->addTx(tx)) {
        update();
        return true;
    }

    return false;
}

std::shared_ptr<Tx> AccountModel::insertTx(const Coin::Transaction& coinTx, Tx::status_t status, bool sign)
{
    if (!vault) {
        return false;
    }

    try {
        std::shared_ptr<Tx> tx(new Tx());
        tx->set(coinTx, time(NULL), status);

        if (sign) {
            tx = vault->signTx(tx);
        }

        if (vault->addTx(tx)) {
            update();
            emit newTx(tx->hash());
            return tx;
        }
    }
    catch (const std::exception& e) {
        emit error(QString::fromStdString(e.what()));
    }

    return NULL;
}

bytes_t AccountModel::createRawTx(const QString& accountName, const std::vector<TaggedOutput>& outputs, uint64_t fee)
{
    Coin::Transaction tx = createTx(accountName, outputs, fee);
    return tx.getSerialized();
}

Coin::Transaction AccountModel::createTx(const QString& accountName, const std::vector<TaggedOutput>& outputs, uint64_t fee)
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }
 
    Tx::txouts_t txouts;
    for (auto& output: outputs) {
        std::shared_ptr<TxOut> txout(new TxOut(output.value(), output.script()));
        txouts.push_back(txout);
        if (output.isTagged() && !vault->scriptTagExists(output.script())) {
            vault->addScriptTag(output.script(), output.tag());
        }
    }

    std::shared_ptr<Tx> tx = vault->newTx(accountName.toStdString(), 1, 0, txouts, fee,  1);
    update();
    return tx->toCoinClasses();
}

bytes_t AccountModel::signRawTx(const bytes_t& rawTx)
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    std::shared_ptr<Tx> tx(new Tx());
    tx->set(rawTx);
    tx = vault->signTx(tx);
    return tx->raw();
}

std::vector<bytes_t> AccountModel::getLocatorHashes() const
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    return vault->getLocatorHashes();
}

bool AccountModel::insertBlock(const ChainBlock& block)
{
    if (!vault) {
        return false;
    }

    try {
        if (vault->insertBlock(block)) {
            emit newBlock(block.blockHeader.getHashLittleEndian(), block.height);
            return true;
        }
    }
    catch (const std::exception& e) {
        emit error(QString::fromStdString(e.what()));
    }

    return false;
}

bool AccountModel::insertMerkleBlock(const ChainMerkleBlock& merkleBlock)
{
    if (!vault) {
        return false;
    }

    try {
        if (vault->insertMerkleBlock(merkleBlock)) {
            emit newBlock(merkleBlock.blockHeader.getHashLittleEndian(), merkleBlock.height);
            return true;
        }
    }
    catch (const std::exception& e) {
        emit error(QString::fromStdString(e.what()));
    }

    return false;
}

bool AccountModel::deleteMerkleBlock(const bytes_t& hash)
{
    if (!vault) {
        return false;
    }

    try {
        if (vault->deleteMerkleBlock(hash)) {
            emit updateSyncHeight(vault->getBestHeight());
            return true;
        }
    }
    catch (const std::exception& e) {
        emit error(QString::fromStdString(e.what()));
    }

    return false;
}

QVariant AccountModel::data(const QModelIndex& index, int role) const
{
    // Right-align numeric fields
    if (role == Qt::TextAlignmentRole && index.column() >= 2) {
        return Qt::AlignRight;
    }
    else {
        return QStandardItemModel::data(index, role);
    }
}

