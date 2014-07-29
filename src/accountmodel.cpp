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
#include "coinparams.h"

#include "accountmodel.h"

#include <CoinQ/CoinQ_script.h>
#include <CoinQ/CoinQ_netsync.h>

#include <CoinCore/random.h>

#include <stdutils/stringutils.h>

#include <QStandardItemModel>
#include <QFile>

#include "severitylogger.h"

using namespace CoinDB;
using namespace CoinQ::Script;
using namespace std;

AccountModel::AccountModel()
    : vault(NULL), numAccounts(0)
{
    base58_versions[0] = getCoinParams().pay_to_pubkey_hash_version();
    base58_versions[1] = getCoinParams().pay_to_script_hash_version();

    currencySymbol = getCurrencySymbol();
    setColumns();
}

void AccountModel::setColumns()
{
    QStringList columns;
    columns << tr("Account") << tr("Policy") << (tr("Balance") + " (" + currencySymbol + ")") << "";
    setHorizontalHeaderLabels(columns);
}

void AccountModel::setVault(CoinDB::Vault* vault)
{
    this->vault = vault;
    update();
}

void AccountModel::update()
{
    QString newCurrencySymbol = getCurrencySymbol();
    if (newCurrencySymbol != currencySymbol)
    {
        currencySymbol = newCurrencySymbol;
        setColumns();
    }

    removeRows(0, rowCount());

    if (!vault) {
        numAccounts = 0;
        return;
    }

    QStringList accountNames;
    std::vector<AccountInfo> accounts = vault->getAllAccountInfo();
    for (auto& account: accounts) {
        QString accountName = QString::fromStdString(account.name());
        QString policy = QString::number(account.minsigs()) + tr(" of ") + QString::fromStdString(stdutils::delimited_list(account.keychain_names(), ", "));
        //QString balance = QString::number(vault->getAccountBalance(account.name(), 0)/(1.0 * currency_divisor), 'g', 8);
        QString balance = getFormattedCurrencyAmount(vault->getAccountBalance(account.name(), 0));
        accountNames << accountName;

        QList<QStandardItem*> row;
        row.append(new QStandardItem(accountName));
        row.append(new QStandardItem(policy));
        row.append(new QStandardItem(balance));
        appendRow(row);

    }
    numAccounts = accountNames.size();

    emit updated(accountNames);
}

void AccountModel::create(const QString& fileName)
{
    close ();
    vault = new Vault(fileName.toStdString(), true);
    emit updateSyncHeight(0);
}

void AccountModel::load(const QString& fileName)
{
    close();
    vault = new Vault(fileName.toStdString(), false);
    update();
    emit updateSyncHeight(vault->getBestHeight());
}

void AccountModel::exportVault(const QString& fileName, bool exportPrivKeys) const
{
    if (!vault)
    {
        throw std::runtime_error("No vault is loaded.");
    }

    vault->exportVault(fileName.toStdString(), exportPrivKeys);
}

void AccountModel::importVault(const QString& fileName)
{
    if (!vault)
    {
        throw std::runtime_error("No vault is loaded.");
    }

    vault->importVault(fileName.toStdString());
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

void AccountModel::newKeychain(const QString& name, const secure_bytes_t& entropy)
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    vault->newKeychain(name.toStdString(), entropy);
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

void AccountModel::exportAccount(const QString& name, const QString& filePath, bool shared) const
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    vault->exportAccount(name.toStdString(), filePath.toStdString(), !shared);
}

void AccountModel::importAccount(const QString& /*name*/, const QString& filePath)
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    unsigned int privkeysimported = 1;
    vault->importAccount(filePath.toStdString(), privkeysimported);
    update();
}

void AccountModel::deleteAccount(const QString& name)
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    throw std::runtime_error("Not implemented.");
/*
    vault->eraseAccount(name.toStdString());
    update();
*/
} 

QPair<QString, bytes_t> AccountModel::issueNewScript(const QString& accountName, const QString& label)
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    std::shared_ptr<SigningScript> signingscript = vault->issueSigningScript(accountName.toStdString(), DEFAULT_BIN_NAME, label.toStdString());
    QString address = QString::fromStdString(getAddressForTxOutScript(signingscript->txoutscript(), base58_versions));
    update();

    return qMakePair(address, signingscript->txoutscript());
}

uint32_t AccountModel::getMaxFirstBlockTimestamp() const
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    return vault->getMaxFirstBlockTimestamp();
}

bool AccountModel::insertRawTx(const bytes_t& rawTx)
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    std::shared_ptr<Tx> tx(new Tx());
    tx->set(rawTx);
    tx->timestamp(time(NULL));
    if (vault->insertTx(tx)) {
        update();
        return true;
    }

    return false;
}

std::shared_ptr<Tx> AccountModel::insertTx(std::shared_ptr<Tx> tx, bool sign)
{
    if (!vault) return nullptr;

    tx = vault->insertTx(tx);
    if (!tx) throw std::runtime_error("Transaction not inserted.");

    if (tx->status() == Tx::UNSIGNED && sign)
    {
        LOGGER(trace) << "Attempting to sign tx " << uchar_vector(tx->unsigned_hash()).getHex() << std::endl;
        std::vector<std::string> keychain_names;
        tx = vault->signTx(tx->unsigned_hash(), keychain_names, true);
    }

    update();
    emit newTx(tx->hash());

    return tx;
}

std::shared_ptr<Tx> AccountModel::insertTx(const Coin::Transaction& coinTx, Tx::status_t status, bool sign)
{
    if (!vault) {
        return std::shared_ptr<Tx>();
    }

    std::shared_ptr<Tx> tx(new Tx());
    tx->set(coinTx, time(NULL), status);
    tx = vault->insertTx(tx);
    if (!tx) throw std::runtime_error("Transaction not inserted.");

    if (tx->status() == Tx::UNSIGNED && sign)
    {
        LOGGER(trace) << "Attempting to sign tx " << uchar_vector(tx->unsigned_hash()).getHex() << std::endl;
        std::vector<std::string> keychain_names;
        vault->signTx(tx->unsigned_hash(), keychain_names, true);
    }

    update();
    emit newTx(tx->hash());

    return tx;
}

std::shared_ptr<CoinDB::Tx> AccountModel::createTx(const QString& accountName, std::vector<std::shared_ptr<CoinDB::TxOut>> txouts, uint64_t fee)
{
    if (!vault) throw std::runtime_error("No vault is loaded.");
 
    std::shared_ptr<Tx> tx = vault->createTx(accountName.toStdString(), 1, 0, txouts, fee,  1);
    return tx;
}

std::shared_ptr<CoinDB::Tx> AccountModel::createTx(const QString& accountName, const std::vector<unsigned long>& coinids, std::vector<std::shared_ptr<CoinDB::TxOut>> txouts, uint64_t fee)
{
    if (!vault) throw std::runtime_error("No vault is loaded.");
 
    std::shared_ptr<Tx> tx = vault->createTx(accountName.toStdString(), 1, 0, coinids, txouts, fee,  1);
    return tx;
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
 
    txouts_t txouts;
    for (auto& output: outputs) {
        std::shared_ptr<TxOut> txout(new TxOut(output.value(), output.script()));
        txouts.push_back(txout);
/*
        if (output.isTagged() && !vault->scriptTagExists(output.script())) {
            vault->addScriptTag(output.script(), output.tag());
        }
*/
    }

    std::shared_ptr<Tx> tx = vault->createTx(accountName.toStdString(), 1, 0, txouts, fee,  1);
    update();
    return tx->toCoinCore();
}

bytes_t AccountModel::signRawTx(const bytes_t& rawTx)
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    throw std::runtime_error("Not implemented.");
/*
    std::shared_ptr<Tx> tx(new Tx());
    tx->set(rawTx);
    tx = vault->signTx(tx);
    return tx->raw();
*/
    return bytes_t();
}

uint32_t AccountModel::getBestHeight() const
{
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    return vault->getBestHeight();
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

    throw std::runtime_error("Not implemented.");
/* 
    try {
        if (vault->insertBlock(block)) {
            emit newBlock(block.blockHeader.getHashLittleEndian(), block.height);
            return true;
        }
    }
    catch (const std::exception& e) {
        emit error(QString::fromStdString(e.what()));
    }
*/
    return false;
}

bool AccountModel::insertMerkleBlock(const ChainMerkleBlock& merkleBlock)
{
    if (!vault) {
        return false;
    }

    try {
        std::shared_ptr<MerkleBlock> merkleblock(new MerkleBlock(merkleBlock));
        if (vault->insertMerkleBlock(merkleblock)) {
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
    if (role == Qt::TextAlignmentRole && index.column() >= 2) {
        // Right-align numeric fields
        return Qt::AlignRight;
    }
    
    return QStandardItemModel::data(index, role);
}

bool AccountModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (role == Qt::EditRole) {
        if (index.column() == 0) {
            // Account name edited.
            if (!vault) return false;

            try {
                vault->renameAccount(index.data().toString().toStdString(), value.toString().toStdString());
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

Qt::ItemFlags AccountModel::flags(const QModelIndex& index) const
{
    if (index.column() == 0) {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
    }

    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}
