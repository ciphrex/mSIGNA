///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// accountmodel.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include <QStandardItemModel>

#include <QPair>
#include <QDateTime>

#include <CoinQ/CoinQ_typedefs.h>

#include <CoinDB/SynchedVault.h>

class TaggedOutput
{
public:
    TaggedOutput(const bytes_t& script, uint64_t value, const std::string& tag)
        : script_(script), value_(value), tag_(tag) { }

    const bytes_t& script() const { return script_; }
    uint64_t value() const { return value_; }
    const std::string& tag() const { return tag_; }
    bool isTagged() const { return !tag_.empty(); }

private:
    bytes_t script_;
    uint64_t value_;
    std::string tag_;
};

class AccountModel : public QStandardItemModel
{
    Q_OBJECT

public:
    AccountModel(CoinDB::SynchedVault& synchedVault);
    ~AccountModel() { };

    CoinDB::Vault* getVault() const;
    bool isOpen() const { return m_synchedVault.isVaultOpen(); }

    static const unsigned int DEFAULT_LOOKAHEAD = 25;

    // UI operations
    void setBase58Versions();

    // Account operations
    void newAccount(const QString& name, unsigned int minsigs, const QList<QString>& keychainNames, qint64 msecsSinceEpoch = QDateTime::currentDateTime().toMSecsSinceEpoch(), unsigned int unusedPoolSize = DEFAULT_LOOKAHEAD);
    void newAccount(bool enableSegwit, bool useSegwitP2SH, const QString& name, unsigned int minsigs, const QList<QString>& keychainNames, qint64 msecsSinceEpoch = QDateTime::currentDateTime().toMSecsSinceEpoch(), unsigned int unusedPoolSize = DEFAULT_LOOKAHEAD);
    bool accountExists(const QString& name) const;
    void exportAccount(const QString& name, const QString& filePath, bool shared) const;
    QString importAccount(const QString& filePath);
    void deleteAccount(const QString& name);
    QPair<QString, bytes_t> issueNewScript(const QString& accountName, const QString& label); // returns qMakePair<address, script>
    uint32_t getMaxFirstBlockTimestamp() const; // the timestamp for latest acceptable first block

    // Transaction operations
    bool insertRawTx(const bytes_t& rawTx);
    std::shared_ptr<CoinDB::Tx> insertTx(std::shared_ptr<CoinDB::Tx> tx, bool sign = false);
    std::shared_ptr<CoinDB::Tx> createTx(const QString& accountName, std::vector<std::shared_ptr<CoinDB::TxOut>> txouts, uint64_t fee);
    std::shared_ptr<CoinDB::Tx> createTx(const QString& accountName, const std::vector<unsigned long>& coinids, std::vector<std::shared_ptr<CoinDB::TxOut>> txouts, uint64_t fee);
    bytes_t createRawTx(const QString& accountName, const std::vector<TaggedOutput>& outputs, uint64_t fee);
    // TODO: not sure I'm too happy about exposing Coin:Vault::Tx
    std::shared_ptr<CoinDB::Tx> insertTx(const Coin::Transaction& coinTx, CoinDB::Tx::status_t status = CoinDB::Tx::PROPAGATED, bool sign = false);
    Coin::Transaction createTx(const QString& accountName, const std::vector<TaggedOutput>& outputs, uint64_t fee);
    bytes_t signRawTx(const bytes_t& rawTx);

    // Block operations
    uint32_t getBestHeight() const;
    std::vector<bytes_t> getLocatorHashes() const;
    bool insertBlock(const ChainBlock& block);
    bool insertMerkleBlock(const ChainMerkleBlock& merkleBlock);
    bool deleteMerkleBlock(const bytes_t& hash);

    //CoinDB::Vault* getVault() const { return vault; }
    int getNumAccounts() const { return numAccounts; }

    // Overridden methods
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex& index) const;

signals:
    void updated(const QStringList& accountNames);
    void newTx(const bytes_t& hash);
    void newBlock(const bytes_t& hash, int height);
    void updateSyncHeight(int height);

    void error(const QString& message);

public slots:
    void update();

private:
    void setColumns();

    unsigned char base58_versions[4];
    QString currencySymbol;

    //CoinDB::Vault* vault;
    CoinDB::SynchedVault& m_synchedVault;
    int numAccounts;
};

