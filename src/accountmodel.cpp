///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// accountmodel.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "settings.h"
#include "coinparams.h"

#include "accountmodel.h"

#include <CoinQ/CoinQ_coinparams.h>
#include <CoinQ/CoinQ_script.h>
#include <CoinQ/CoinQ_netsync.h>

#include <CoinCore/random.h>

#include <stdutils/stringutils.h>

#include <QStandardItemModel>
#include <QFile>

#include "severitylogger.h"

const bool USE_WITNESS_P2SH = true; // only used if segregated witness is enabled

using namespace CoinDB;
using namespace CoinQ::Script;
using namespace std;

AccountModel::AccountModel(CoinDB::SynchedVault& synchedVault)
    : m_synchedVault(synchedVault), numAccounts(0)
{
    setBase58Versions();
    currencySymbol = getCurrencySymbol();
    setColumns();
}

void AccountModel::setBase58Versions()
{
    base58_versions[0] = getCoinParams().pay_to_pubkey_hash_version();
#ifdef SUPPORT_OLD_ADDRESS_VERSIONS
    base58_versions[1] = getUseOldAddressVersions() ? getCoinParams().old_pay_to_script_hash_version() : getCoinParams().pay_to_script_hash_version();
#else
    base58_versions[1] = getCoinParams().pay_to_script_hash_version();
#endif
    base58_versions[2] = getCoinParams().pay_to_witness_pubkey_hash_version();
    base58_versions[3] = getCoinParams().pay_to_witness_script_hash_version();
}

void AccountModel::setColumns()
{
    QDateTime dateTime(QDateTime::currentDateTime());

    QStringList columns;
    columns << tr("Account") << tr("") << (tr("Confirmed") + " (" + currencySymbol + ")") << (tr("Pending") + " (" + currencySymbol + ")") << (tr("Total") + " (" + currencySymbol + ")") << tr("Policy") << (tr("Creation Time") + " (" + dateTime.timeZoneAbbreviation() + ")");// << "";
    setHorizontalHeaderLabels(columns);
}
/*
void AccountModel::setVault(CoinDB::Vault* vault)
{
    this->vault = vault;
}
*/
void AccountModel::update()
{
    setBase58Versions();

    QString newCurrencySymbol = getCurrencySymbol();
    if (newCurrencySymbol != currencySymbol)
    {
        currencySymbol = newCurrencySymbol;
        setColumns();
    }

    removeRows(0, rowCount());

    CoinDB::Vault* vault = m_synchedVault.getVault();
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

        uint64_t total = vault->getAccountBalance(account.name(), 0);
        uint64_t confirmed = vault->getAccountBalance(account.name(), 1);
        uint64_t pending = total - confirmed;
        QString confirmedBalance = getFormattedCurrencyAmount(confirmed);
        QString pendingBalance = tr("+") + getFormattedCurrencyAmount(pending);
        QString totalBalance = getFormattedCurrencyAmount(total);

        QDateTime dateTime;
        dateTime.setTime_t(account.time_created());
        QString creationTime = dateTime.toString("yyyy-MM-dd hh:mm:ss");

        accountNames << accountName;

        QStandardItem* segwitItem = new QStandardItem();
        if (account.use_witness())
        {
            segwitItem->setIcon(QIcon(":/icons/segwit_64x64.png"));
            segwitItem->setData(true, Qt::UserRole);
        }
        else
        {
            segwitItem->setData(false, Qt::UserRole);
        }
        
        QList<QStandardItem*> row;
        row.append(new QStandardItem(accountName));
        row.append(segwitItem);
        row.append(new QStandardItem(confirmedBalance));
        row.append(new QStandardItem(pendingBalance));
        row.append(new QStandardItem(totalBalance));
        row.append(new QStandardItem(policy));
        row.append(new QStandardItem(creationTime));
        appendRow(row);

    }
    numAccounts = accountNames.size();

    emit updated(accountNames);
}

CoinDB::Vault* AccountModel::getVault() const
{
    return m_synchedVault.getVault();
} 


void AccountModel::newAccount(const QString& name, unsigned int minsigs, const QList<QString>& keychainNames, qint64 msecsSinceEpoch, unsigned int unusedPoolSize)
{
    newAccount(getCoinParams().segwit_enabled(), true, name, minsigs, keychainNames, msecsSinceEpoch, unusedPoolSize);
}

void AccountModel::newAccount(bool enableSegwit, bool useSegwitP2SH, const QString& name, unsigned int minsigs, const QList<QString>& keychainNames, qint64 msecsSinceEpoch, unsigned int unusedPoolSize)
{
    CoinDB::Vault* vault = m_synchedVault.getVault();
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    std::vector<std::string> keychain_names;
    for (auto& name: keychainNames) { keychain_names.push_back(name.toStdString()); }

    uint64_t secsSinceEpoch = (uint64_t)msecsSinceEpoch / 1000;
    vault->newAccount(enableSegwit && getCoinParams().segwit_enabled(), useSegwitP2SH, name.toStdString(), minsigs, keychain_names, unusedPoolSize, secsSinceEpoch);
    update();
}

bool AccountModel::accountExists(const QString& name) const
{
    CoinDB::Vault* vault = m_synchedVault.getVault();
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    return vault->accountExists(name.toStdString());
}

void AccountModel::exportAccount(const QString& name, const QString& filePath, bool shared) const
{
    CoinDB::Vault* vault = m_synchedVault.getVault();
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    vault->exportAccount(name.toStdString(), filePath.toStdString(), !shared);
}

QString AccountModel::importAccount(const QString& filePath)
{
    CoinDB::Vault* vault = m_synchedVault.getVault();
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    unsigned int privkeysimported = 1;
    std::shared_ptr<Account> account = vault->importAccount(filePath.toStdString(), privkeysimported);
    update();
    return QString::fromStdString(account->name());
}

void AccountModel::deleteAccount(const QString& /*name*/)
{
    CoinDB::Vault* vault = m_synchedVault.getVault();
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
    CoinDB::Vault* vault = m_synchedVault.getVault();
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    std::shared_ptr<SigningScript> signingscript = vault->issueSigningScript(accountName.toStdString(), DEFAULT_BIN_NAME, label.toStdString());
    QString address = QString::fromStdString(getAddressForTxOutScript(signingscript->txoutscript(), base58_versions));
//    update();

    return qMakePair(address, signingscript->txoutscript());
}

uint32_t AccountModel::getMaxFirstBlockTimestamp() const
{
    CoinDB::Vault* vault = m_synchedVault.getVault();
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    return vault->getMaxFirstBlockTimestamp();
}

bool AccountModel::insertRawTx(const bytes_t& rawTx)
{
    CoinDB::Vault* vault = m_synchedVault.getVault();
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
    CoinDB::Vault* vault = m_synchedVault.getVault();
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
    CoinDB::Vault* vault = m_synchedVault.getVault();
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
    CoinDB::Vault* vault = m_synchedVault.getVault();
    if (!vault) throw std::runtime_error("No vault is loaded.");
 
    std::shared_ptr<Tx> tx = vault->createTx(accountName.toStdString(), 1, 0, txouts, fee,  1);
    return tx;
}

std::shared_ptr<CoinDB::Tx> AccountModel::createTx(const QString& accountName, const std::vector<unsigned long>& coinids, std::vector<std::shared_ptr<CoinDB::TxOut>> txouts, uint64_t fee)
{
    CoinDB::Vault* vault = m_synchedVault.getVault();
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
    CoinDB::Vault* vault = m_synchedVault.getVault();
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

bytes_t AccountModel::signRawTx(const bytes_t& /*rawTx*/)
{
    CoinDB::Vault* vault = m_synchedVault.getVault();
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
    CoinDB::Vault* vault = m_synchedVault.getVault();
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    return vault->getBestHeight();
}

std::vector<bytes_t> AccountModel::getLocatorHashes() const
{
    CoinDB::Vault* vault = m_synchedVault.getVault();
    if (!vault) {
        throw std::runtime_error("No vault is loaded.");
    }

    return vault->getLocatorHashes();
}

bool AccountModel::insertBlock(const ChainBlock& /*block*/)
{
    CoinDB::Vault* vault = m_synchedVault.getVault();
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
    CoinDB::Vault* vault = m_synchedVault.getVault();
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
    CoinDB::Vault* vault = m_synchedVault.getVault();
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
    if (role == Qt::TextAlignmentRole)
    {
        // Right-align numeric fields
        if (index.column() >= 2 && index.column() <= 4) return Qt::AlignRight;
    }
    else if (role == Qt::FontRole)
    {
        QFont font;
        if (index.column() == 0)
        {
            font.setBold(true);
            return font;
        }
    }

/*
    else if (role == Qt::ForegroundRole)
    {
        ...
    }
    else if (role == Qt::BackgroundRole)
    {
        switch (index.column())
        {
            case 1: return QBrush(Qt::green);
        }
    }
*/

    return QStandardItemModel::data(index, role);
}

bool AccountModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (role == Qt::EditRole) {
        if (index.column() == 0) {
            // Account name edited.
            CoinDB::Vault* vault = m_synchedVault.getVault();
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
