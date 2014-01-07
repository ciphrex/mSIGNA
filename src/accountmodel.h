///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// accountmodel.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef COINVAULT_ACCOUNTMODEL_H
#define COINVAULT_ACCOUNTMODEL_H

#include <QStandardItemModel>

#include <QPair>

#include <CoinQ_vault.h>

#include <CoinQ_typedefs.h>

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
    AccountModel();
    ~AccountModel() { if (vault) delete vault; }

    void update();

    // Vault operations
    void create(const QString& fileName);
    void load(const QString& fileName);
    void close();
    bool isOpen() const { return (vault != NULL); }
    Coin::BloomFilter getBloomFilter(double falsePositiveRate, uint32_t nTweak, uint32_t nFlags = 0) const;

    // Key Chain operations
    void newKeychain(const QString& name, unsigned long numkeys);
    void newHDKeychain(const QString& name, const bytes_t& extkey, unsigned long numkeys);

    // Account operations
    void newAccount(const QString& name, unsigned int minsigs, const QList<QString>& keychainNames);
    bool accountExists(const QString& name) const;
    void exportAccount(const QString& name, const QString& filePath) const;
    void importAccount(const QString& name, const QString& filePath);
    void deleteAccount(const QString& name);
    QPair<QString, bytes_t> issueNewScript(const QString& accountName, const QString& label); // returns qMakePair<address, script>
    uint32_t getFirstAccountTimeCreated() const; // the timestamp for when the first account in the vault was created

    // Transaction operations
    bool insertRawTx(const bytes_t& rawTx);
    std::shared_ptr<CoinQ::Vault::Tx> insertTx(const Coin::Transaction& coinTx, CoinQ::Vault::Tx::status_t status = CoinQ::Vault::Tx::RECEIVED, bool sign = false);
    bytes_t createRawTx(const QString& accountName, const std::vector<TaggedOutput>& outputs, uint64_t fee);
    // TODO: not sure I'm too happy about exposing Coin:Vault::Tx
    Coin::Transaction createTx(const QString& accountName, const std::vector<TaggedOutput>& outputs, uint64_t fee);
    bytes_t signRawTx(const bytes_t& rawTx);

    // Block operations
    std::vector<bytes_t> getLocatorHashes() const;
    bool insertBlock(const ChainBlock& block);
    bool insertMerkleBlock(const ChainMerkleBlock& merkleBlock);
    bool deleteMerkleBlock(const bytes_t& hash);

    CoinQ::Vault::Vault* getVault() const { return vault; }
    int getNumAccounts() const { return numAccounts; }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

signals:
    void updated(const QStringList& accountNames);
    void newTx(const bytes_t& hash);
    void newBlock(const bytes_t& hash, int height);
    void updateSyncHeight(int height);

    void error(const QString& message);

private:
    CoinQ::Vault::Vault* vault;
    int numAccounts;
};

#endif // COINVAULT_ACCOUNTMODEL_H
