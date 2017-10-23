///////////////////////////////////////////////////////////////////////////////
//
// Vault.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#define LOCK_ALL_CALLS

#include "Vault.h"
#include "Database.h"

#include <CoinQ/CoinQ_script.h>
#include <CoinQ/CoinQ_blocks.h>

#if defined(DATABASE_MYSQL)
    #include "../odb/Schema-odb-mysql.hxx"
#elif defined(DATABASE_SQLITE)
    #include "../odb/Schema-odb-sqlite.hxx"
#else
    #error "No database engine selected."
#endif

#include <odb/transaction.hxx>
#include <odb/session.hxx>

#include <CoinCore/hash.h>
#include <CoinCore/aes.h>
#include <CoinCore/MerkleTree.h>
#include <CoinCore/secp256k1_openssl.h>
#include <CoinCore/BigInt.h>

#include <logger/logger.h>

#include <stdutils/stringutils.h>

#include <sstream>
#include <fstream>
#include <algorithm>

using namespace CoinDB;

/*
 * data migration
*/
/*
template <odb::schema_version v>
using migration_entry = odb::data_migration_entry<v, SCHEMA_BASE_VERSION>;

static void migrate_compressed_keys(odb::database& db)
{
    LOGGER(trace) << "Migrating accounts to schema 14..." << std::endl;

    for (auto& account: db.query<CoinDB::Account>())
    {
        account.compressed_keys(true);
        db.update(account);
    }
}

static const migration_entry<14> migrate_compressed_keys_entry(&migrate_compressed_keys);
*/

/*
 * class Vault implementation
*/
Vault::Vault(int argc, char** argv, bool create, uint32_t version, const std::string& network, bool migrate)
{
    LOGGER(trace) << "Vault::Vault(..., " << (create ? "true" : "false") << ", " << version << ", " << network << ", " << (migrate ? "true" : "false") << ")" << std::endl;

    open(argc, argv, create, version, network, migrate);
//    if (argc >= 2) name_ = argv[1];
//    if (create) setSchemaVersion(version);
}

Vault::Vault(const std::string& dbname, bool create, uint32_t version, const std::string& network, bool migrate)
{
    LOGGER(trace) << "Vault::Vault(" << dbname << ", " << (create ? "true" : "false") << ", " << version << ", " << network << ", " << (migrate ? "true" : "false") << ")" << std::endl;

    open("", "", dbname, create, version, network, migrate);
//    name_ = dbname;
//    if (create) setSchemaVersion(version);
}

Vault::Vault(const std::string& dbuser, const std::string& dbpasswd, const std::string& dbname, bool create, uint32_t version, const std::string& network, bool migrate)
{
    LOGGER(trace) << "Vault::Vault(" << dbuser << ", ..., " << dbname << ", " << (create ? "true" : "false") << ", " << version << ", " << network << ", " << (migrate ? "true" : "false") << ")" << std::endl;

    open(dbuser, dbpasswd, dbname, create, version, network, migrate);
//    name_ = dbname;
//    if (create) setSchemaVersion(version);
}

Vault::~Vault()
{
    LOGGER(trace) << "Vault::~Vault()" << std::endl;

    close();
}

////////////////////
// STATIC METHODS //
////////////////////
bool Vault::isValidObjectName(const std::string& name)
{
    if (name.empty()) return false;
    for (auto& c: name)
    {
        if (c >= 'a' && c <= 'z') continue;
        if (c >= 'A' && c <= 'Z') continue;
        if (c >= '0' && c <= '9') continue;
        if (c == ' ') continue;
        if (c == '_') continue;
        if (c == '-') continue;

        return false;
    }

    return true;
}

Vault::split_name_t Vault::getSplitObjectName(const std::string& name)
{
    if (name.empty()) return split_name_t(std::string(), 0);

    unsigned int n = 0;
    unsigned int mult = 1;
    unsigned int digit = 0;
    int i = name.size() - 1;
    for (; i >= 0; i--)
    {
        if (name[i] < '0' || name[i] > '9') break;
        digit = name[i] - '0';
        n += digit * mult;
        mult *= 10;
    }

    if (i < 0 && digit != 0) return split_name_t(std::string(), n); // all digits, no leading 0's
    if (digit == 0 || name[i] != ' ') return split_name_t(name, 0); // leading 0's or no space
    return split_name_t(name.substr(0, i), n);
}

///////////////////////
// GLOBAL OPERATIONS //
///////////////////////
void Vault::open(int argc, char** argv, bool create, uint32_t version, const std::string& network, bool migrate)
{
    LOGGER(trace) << "Vault::open(..., " << (create ? "true" : "false") << ", " << version << ", " << network << ", " << (migrate ? "true" : "false") << ")" << std::endl;

    if (argc >= 2) name_ = argv[1];

    boost::lock_guard<boost::mutex> lock(mutex);

    try
    {
        db_ = open_database(argc, argv, create);
    }
    catch (const std::exception& e)
    {
        throw VaultFailedToOpenDatabaseException(name_, e.what());
    }

    odb::schema_version v(db_->schema_version());
    odb::schema_version bv(odb::schema_catalog::base_version(*db_));
    odb::schema_version cv(odb::schema_catalog::current_version(*db_));

    if (v < bv)
    {
        throw VaultFailedToOpenDatabaseException(name_, "Schema version is no longer supported.");
    }

    if (v > cv)
    {
        throw VaultFailedToOpenDatabaseException(name_, "Schema version is newer than program supports. Please upgrade.");
    }

    odb::core::transaction t(db_->begin());

    if (create)
    {
        setSchemaVersion_unwrapped(version);
        setNetwork_unwrapped(network);
        t.commit();
    }
    else
    {
        //uint32_t schemaVersion = getSchemaVersion_unwrapped();
        //if (schemaVersion != version) throw VaultWrongSchemaVersionException(name_, schemaVersion);

        if (!network.empty())
        {
            std::string dbNetwork = getNetwork_unwrapped();
            if (!dbNetwork.empty())
            {
                std::string lowerNetwork = network;
                std::transform(lowerNetwork.begin(), lowerNetwork.end(), lowerNetwork.begin(), ::tolower);
                if (dbNetwork != lowerNetwork) throw VaultWrongNetworkException(name_, dbNetwork);
            }
        }

        if (v < cv)
        {
            if (!migrate) throw VaultNeedsSchemaMigrationException(name_, v, cv);

            LOGGER(info) << "Migrating database from schema " << v << " to schema " << cv << "." << std::endl;
            odb::schema_catalog::migrate(*db_);
            setSchemaVersion_unwrapped(version);

            if (v < 14 && cv >= 14)
            {
                LOGGER(info) << "Migrating account data..." << std::endl;
                odb::core::session s;
                for (auto& account: db_->query<Account>())
                {
                    account.compressed_keys(true);
                    account.initScriptPatterns(); // redeemscript must be initialized
                    db_->update(account);
                }
            }
                
            t.commit();
        }

        {
            odb::core::session s;
            for (auto& account: db_->query<Account>())
            {
                if (!account.redeempattern_set())
                    throw std::runtime_error("Error in vault file. Please import keychains into a new vault file and recreate account.");
            }
        }
    }
}

void Vault::open(const std::string& dbuser, const std::string& dbpasswd, const std::string& dbname, bool create, uint32_t version, const std::string& network, bool migrate)
{
    LOGGER(trace) << "Vault::open(" << dbuser << ", ..., " << dbname << ", " << (create ? "true" : "false") << ", " << version << ", " << network << ", " << (migrate ? "true" : "false") << ")" << std::endl;

    name_ = dbname;

    boost::lock_guard<boost::mutex> lock(mutex);

    try
    {
        db_ = openDatabase(dbuser, dbpasswd, dbname, create);
    }
    catch (const std::exception& e)
    {
        throw VaultFailedToOpenDatabaseException(name_, e.what());
    }

    odb::schema_version v(db_->schema_version());
    odb::schema_version bv(odb::schema_catalog::base_version(*db_));
    odb::schema_version cv(odb::schema_catalog::current_version(*db_));

    if (v < bv)
    {
        throw VaultFailedToOpenDatabaseException(name_, "Schema version is no longer supported.");
    }

    if (v > cv)
    {
        throw VaultFailedToOpenDatabaseException(name_, "Schema version is newer than program supports. Please upgrade.");
    }

    odb::core::transaction t(db_->begin());

    if (create)
    {
        setSchemaVersion_unwrapped(cv);
        setNetwork_unwrapped(network);
        t.commit();
    }
    else
    {
//        uint32_t schemaVersion = getSchemaVersion_unwrapped();
//        if (schemaVersion != version) throw VaultWrongSchemaVersionException(name_, schemaVersion);

        if (!network.empty())
        {
            std::string dbNetwork = getNetwork_unwrapped();
            if (!dbNetwork.empty())
            {
                std::string lowerNetwork = network;
                std::transform(lowerNetwork.begin(), lowerNetwork.end(), lowerNetwork.begin(), ::tolower);
                if (dbNetwork != lowerNetwork) throw VaultWrongNetworkException(name_, dbNetwork);
            }
        }

        if (v < cv)
        {
            if (!migrate) throw VaultNeedsSchemaMigrationException(name_, v, cv);

            LOGGER(info) << "Migrating database from schema " << v << " to schema " << cv << "." << std::endl;
            odb::schema_catalog::migrate(*db_);
            setSchemaVersion_unwrapped(version);

            if (v < 14 && cv >= 14)
            {
                LOGGER(info) << "Migrating account data..." << std::endl;
                odb::core::session s;
                for (auto& account: db_->query<Account>())
                {
                    account.compressed_keys(true);
                    account.initScriptPatterns(); // redeemscript must be initialized
                    db_->update(account);
                }
            }

            t.commit();
        }

        {
            odb::core::session s;
            for (auto& account: db_->query<Account>())
            {
                if (!account.redeempattern_set())
                    throw std::runtime_error("Error in vault file. Please import keychains into a new vault file and recreate account.");
            }
        }
    }
}

void Vault::close()
{
    LOGGER(trace) << "Vault::close()" << std::endl;

    if (!db_) return;
    boost::lock_guard<boost::mutex> lock(mutex);
    db_.reset();
}

uint32_t Vault::getSchemaVersion() const
{
    LOGGER(trace) << "Vault::getSchemaVersion()" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());
    return getSchemaVersion_unwrapped();
}

uint32_t Vault::getSchemaVersion_unwrapped() const
{
    odb::result<Version> r(db_->query<Version>());
    return r.empty() ? 0 : r.begin()->version();
}

void Vault::setSchemaVersion(uint32_t version)
{
    LOGGER(trace) << "Vault::setSchemaVersion(" << version << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::transaction t(db_->begin());
    setSchemaVersion_unwrapped(version);
    t.commit();
}

void Vault::setSchemaVersion_unwrapped(uint32_t version)
{
    odb::result<Version> r(db_->query<Version>());
    if (r.empty())
    {
        std::shared_ptr<Version> version_obj(new Version(version));
        db_->persist(version_obj);
    }
    else
    {
        std::shared_ptr<Version> version_obj(r.begin().load());
        version_obj->version(version);
        db_->update(version_obj);
    }
}

std::string Vault::getNetwork() const
{
    LOGGER(trace) << "Vault::getNetwork()" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());
    return getNetwork_unwrapped();
}

std::string Vault::getNetwork_unwrapped() const
{
    odb::result<Network> r(db_->query<Network>());
    return r.empty() ? "" : r.begin()->network();
}

void Vault::setNetwork(const std::string& network)
{
    LOGGER(trace) << "Vault::setNetwork(" << network << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::transaction t(db_->begin());
    setNetwork_unwrapped(network);
    t.commit();
}

void Vault::setNetwork_unwrapped(const std::string& network)
{
    std::string lower_network = network;
    std::transform(lower_network.begin(), lower_network.end(), lower_network.begin(), ::tolower);

    odb::result<Network> r(db_->query<Network>());
    if (r.empty())
    {
        std::shared_ptr<Network> network_obj(new Network(lower_network));
        db_->persist(network_obj);
    }
    else
    {
        std::shared_ptr<Network> network_obj(r.begin().load());
        network_obj->network(lower_network);
        db_->update(network_obj);
    }
}

uint32_t Vault::getHorizonTimestamp() const
{
    LOGGER(trace) << "Vault::getHorizonTimestamp()" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());
    return getHorizonTimestamp_unwrapped();
}

uint32_t Vault::getMaxFirstBlockTimestamp() const
{
    LOGGER(trace) << "Vault::getMaxFirstBlockTimestamp()" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());
    return getMaxFirstBlockTimestamp_unwrapped();
}

uint32_t Vault::getHorizonTimestamp_unwrapped() const
{
    odb::result<HorizonTimestampView> r(db_->query<HorizonTimestampView>());
    return r.empty() ? 0 : r.begin()->timestamp;
}

uint32_t Vault::getMaxFirstBlockTimestamp_unwrapped() const
{
    uint32_t maxFirstBlockTimestamp = getHorizonTimestamp_unwrapped();
    if (maxFirstBlockTimestamp > MAX_HORIZON_TIMESTAMP_OFFSET)
        maxFirstBlockTimestamp -= MAX_HORIZON_TIMESTAMP_OFFSET;
    return maxFirstBlockTimestamp;
}

uint32_t Vault::getHorizonHeight() const
{
    LOGGER(trace) << "Vault::getHorizonHeight()" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());
    return getHorizonHeight_unwrapped();
}

uint32_t Vault::getHorizonHeight_unwrapped() const
{
    odb::result<HorizonHeightView> r(db_->query<HorizonHeightView>());
    return r.empty() ? 0 : r.begin()->height;
}

std::vector<bytes_t> Vault::getLocatorHashes() const
{
    LOGGER(trace) << "Vault::getLocatorHashes()" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());
    return getLocatorHashes_unwrapped();
}

std::vector<bytes_t> Vault::getLocatorHashes_unwrapped() const
{
    std::vector<bytes_t>  hashes;
    std::vector<uint32_t> heights;

    uint32_t i = getBestHeight_unwrapped();
    if (i == 0) return hashes;

    uint32_t n = 1;
    uint32_t step = 1;
    heights.push_back(i);
    while (step <= i)
    {
        i -= step;
        n++;
        if (n > 10) step *= 2;
        heights.push_back(i);
    }

    typedef odb::query<BlockHeader> query_t;
    odb::result<BlockHeader> r(db_->query<BlockHeader>(query_t::height.in_range(heights.begin(), heights.end()) + "ORDER BY" + query_t::height + "DESC"));
    for (auto& header: r) { hashes.push_back(header.hash()); }
    return hashes;
}

Coin::BloomFilter Vault::getBloomFilter(double falsePositiveRate, uint32_t nTweak, uint32_t nFlags) const
{
    LOGGER(trace) << "Vault::getBloomFilter(" << falsePositiveRate << ", " << nTweak << ", " << nFlags << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    return getBloomFilter_unwrapped(falsePositiveRate, nTweak, nFlags);
}

Coin::BloomFilter Vault::getBloomFilter_unwrapped(double falsePositiveRate, uint32_t nTweak, uint32_t nFlags) const
{
    using namespace CoinQ::Script;

    std::vector<bytes_t> elements;

    // Add scripts
    {
        odb::result<SigningScript> r(db_->query<SigningScript>());
        for (auto& script: r)
        {

            // Add script elements
            if (script.account()->use_witness())
            {
                WitnessProgram_P2WSH wp(script.redeemscript());
                elements.push_back(wp.script());
                if (script.account()->use_witness_p2sh())
                {
                    elements.push_back(getScriptPubKeyPayee(script.txoutscript()).second);
                }
            }
            else
            {
                elements.push_back(getScriptPubKeyPayee(script.txoutscript()).second);
                elements.push_back(script.redeemscript());
            }
        }
    }

    {
        // Add outpoints
        typedef odb::query<TxOut> query_t;
        odb::result<TxOut> r(db_->query<TxOut>(query_t::sending_account != 0 && query_t::status == TxOut::UNSPENT));
        for (auto& txout: r)
        {
            std::shared_ptr<Tx> tx = txout.tx();
            if (tx)
            {
                Coin::OutPoint outpoint(tx->hash(), txout.txindex());
                elements.push_back(outpoint.getSerialized());
            }
        }
    }

    if (elements.empty()) return Coin::BloomFilter();

    Coin::BloomFilter filter(elements.size(), falsePositiveRate, nTweak, nFlags);
    for (auto& element: elements) { filter.insert(element); }
    return filter;
}

hashvector_t Vault::getIncompleteBlockHashes() const
{
    LOGGER(trace) << "Vault::getIncompleteBlockHashes()" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    return getIncompleteBlockHashes_unwrapped();
}

hashvector_t Vault::getIncompleteBlockHashes_unwrapped() const
{
    hashvector_t hashes;
    odb::result<MerkleBlock> r(db_->query<MerkleBlock>((odb::query<MerkleBlock>::txsinserted == false) + "ORDER BY" + odb::query<MerkleBlock>::blockheader->height));
    for (auto& merkleblock: r) { hashes.push_back(merkleblock.blockheader()->hash()); }
    return hashes;
}

void Vault::exportVault(const std::string& filepath, bool exportprivkeys) const
{
    LOGGER(trace) << "Vault::exportVault(" << filepath << ", " << (exportprivkeys ? "true" : "false") << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    std::ofstream ofs(filepath);
    boost::archive::text_oarchive oa(ofs);

    odb::core::transaction t(db_->begin());
    odb::core::session s;

    odb::result<AccountCountView> count_r(db_->query<AccountCountView>());
    uint32_t n = count_r.empty() ? 0 : count_r.begin()->count;

    oa << n;
    if (n > 0)
    {
        // Export all accounts
        odb::result<Account> account_r(db_->query<Account>());
        for (auto& account: account_r)
        {
            exportAccount_unwrapped(account, oa, exportprivkeys);
        }

        // Export merkle blocks
        exportMerkleBlocks_unwrapped(oa);

        // Export transactions
        exportTxs_unwrapped(oa, 0);
    }
}
 
void Vault::importVault(const std::string& filepath, bool importprivkeys)
{
    LOGGER(trace) << "Vault::importVault(" << filepath << ", " << (importprivkeys ? "true" : "false") << std::endl;

    {
        boost::lock_guard<boost::mutex> lock(mutex);
        std::ifstream ifs(filepath);
        boost::archive::text_iarchive ia(ifs);

        odb::core::transaction t(db_->begin());

        uint32_t n;
        ia >> n;
        if (n > 0)
        {
            // Import all accounts
            for (uint32_t i = 0; i < n; i++)
            {
                unsigned int privkeysimported = importprivkeys;
                odb::core::session s;
                importAccount_unwrapped(ia, privkeysimported);
            }

            // Import merkle blocks
            {
                odb::core::session s;
                importMerkleBlocks_unwrapped(ia);
            }

            // Import transactions
            importTxs_unwrapped(ia); 
        }
        t.commit();
    }

    signalQueue.flush();
}


////////////////////////
// CONTACT OPERATIONS //
////////////////////////
std::shared_ptr<Contact> Vault::newContact(const std::string& username)
{
    LOGGER(trace) << "Vault::newContact(" << username << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::transaction t(db_->begin());
    std::shared_ptr<Contact> contact = newContact_unwrapped(username);
    t.commit();
    return contact;
}

std::shared_ptr<Contact> Vault::newContact_unwrapped(const std::string& username)
{
    if (username.empty()) throw ContactInvalidUsernameException(username);
    if (contactExists_unwrapped(username)) throw ContactAlreadyExistsException(username);

    std::shared_ptr<Contact> contact = std::make_shared<Contact>(username);
    db_->persist(contact);
    return contact;
}

std::shared_ptr<Contact> Vault::getContact(const std::string& username) const
{
    LOGGER(trace) << "Vault::getContact(" << username << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());
    return getContact_unwrapped(username);
}

std::shared_ptr<Contact> Vault::getContact_unwrapped(const std::string& username) const
{
    if (username.empty()) throw ContactInvalidUsernameException(username);

    odb::result<Contact> r(db_->query<Contact>(odb::query<Contact>::username == username));
    if (r.empty()) throw ContactNotFoundException(username);

    std::shared_ptr<Contact> contact(r.begin().load());
    return contact;
}

ContactVector Vault::getAllContacts() const
{
    LOGGER(trace) << "Vault::getAllContacts()" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());
    return getAllContacts_unwrapped();
}

ContactVector Vault::getAllContacts_unwrapped() const
{
    odb::query<Contact> query(1 == 1);
    odb::result<Contact> r(db_->query<Contact>(query + "ORDER BY" + odb::query<Contact>::username));

    ContactVector contacts;
    for (auto& contact: r) { contacts.push_back(contact.get_shared_ptr()); }
    return contacts;
}

bool Vault::contactExists(const std::string& username) const
{
    LOGGER(trace) << "Vault::contactExists(" << username << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());
    return contactExists_unwrapped(username);
}

bool Vault::contactExists_unwrapped(const std::string& username) const
{
    if (username.empty()) throw ContactInvalidUsernameException(username);

    odb::result<Contact> r(db_->query<Contact>(odb::query<Contact>::username == username));
    return !r.empty();
}

std::shared_ptr<Contact> Vault::renameContact(const std::string& old_username, const std::string& new_username)
{
    LOGGER(trace) << "Vault::renameContact(" << old_username << ", " << new_username << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::transaction t(db_->begin());
    std::shared_ptr<Contact> contact = renameContact_unwrapped(old_username, new_username);
    t.commit();
    return contact;
}

std::shared_ptr<Contact> Vault::renameContact_unwrapped(const std::string& old_username, const std::string& new_username)
{
    if (old_username.empty()) throw ContactInvalidUsernameException(old_username);
    if (new_username.empty()) throw ContactInvalidUsernameException(new_username);

    std::shared_ptr<Contact> contact = getContact_unwrapped(old_username);
    if (old_username == new_username) return contact;

    if (contactExists_unwrapped(new_username)) throw ContactAlreadyExistsException(new_username);

    contact->username(new_username);
    db_->update(contact);
    return contact;
}


/////////////////////////
// KEYCHAIN OPERATIONS //
/////////////////////////
void Vault::exportKeychain(const std::string& keychain_name, const std::string& filepath, bool exportprivkeys) const
{
    LOGGER(trace) << "Vault::exportKeychain(" << keychain_name << ", " << filepath << ", " << (exportprivkeys ? "true" : "false") << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());
    std::shared_ptr<Keychain> keychain = getKeychain_unwrapped(keychain_name);
    if (exportprivkeys && !keychain->isPrivate()) throw KeychainIsNotPrivateException(keychain_name);
    if (!exportprivkeys) { keychain->clearPrivateKey(); }
    exportKeychain_unwrapped(keychain, filepath);
}

void Vault::exportKeychain_unwrapped(std::shared_ptr<Keychain> keychain, const std::string& filepath) const
{
    std::ofstream ofs(filepath);
    boost::archive::text_oarchive oa(ofs);
    oa << *keychain;
}

std::shared_ptr<Keychain> Vault::importKeychain(const std::string& filepath, bool& importprivkeys)
{
    LOGGER(trace) << "Vault::importKeychain(" << filepath << ", " << (importprivkeys ? "true" : "false") << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    std::shared_ptr<Keychain> keychain = importKeychain_unwrapped(filepath, importprivkeys);
    t.commit();
    return keychain;
}

std::shared_ptr<Keychain> Vault::importKeychain_unwrapped(const std::string& filepath, bool& importprivkeys)
{
    std::shared_ptr<Keychain> keychain(new Keychain());
    {
        std::ifstream ifs(filepath);
        boost::archive::text_iarchive ia(ifs);
        ia >> *keychain;
    }

    if (!keychain->isPrivate()) { importprivkeys = false; }
    if (!importprivkeys)        { keychain->clearPrivateKey(); }

    odb::result<Keychain> keychain_r(db_->query<Keychain>(odb::query<Keychain>::hash == keychain->hash()));
    if (!keychain_r.empty())
    {
        std::shared_ptr<Keychain> stored_keychain(keychain_r.begin().load());
        if (keychain->isPrivate() && !stored_keychain->isPrivate())
        {
            // We already have the public key. Import the private key and update signing keys.
            stored_keychain->importPrivateKey(*keychain);
            db_->update(stored_keychain);
            odb::result<Key> key_r(db_->query<Key>(odb::query<Key>::root_keychain == stored_keychain->id() && odb::query<Key>::is_private == 0));
            for (auto& key: key_r)
            {
                key.updatePrivate();
                db_->update(key);
            }
            return stored_keychain; 
        }
        throw KeychainAlreadyExistsException(stored_keychain->name());
    }

/*
    std::string keychain_name = keychain->name();
    unsigned int append_num = 1; // in case of name conflict
    while (keychainExists_unwrapped(keychain->name()))
    {
        std::stringstream ss;
        ss << keychain_name << append_num++;
        keychain->name(ss.str());
    }
*/

    keychain->name(getNextAvailableKeychainName_unwrapped(keychain->name()));
    db_->persist(keychain);
    return keychain;
}

bool Vault::keychainExists(const std::string& keychain_name) const
{
    LOGGER(trace) << "Vault::keychainExists(" << keychain_name << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());
    return keychainExists_unwrapped(keychain_name);
}

bool Vault::keychainExists_unwrapped(const std::string& keychain_name) const
{
    odb::result<Keychain> r(db_->query<Keychain>(odb::query<Keychain>::name == keychain_name));
    return !r.empty();
}

bool Vault::keychainExists(const bytes_t& keychain_hash) const
{
    LOGGER(trace) << "Vault::keychainExists(@hash = " << uchar_vector(keychain_hash).getHex() << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());
    return keychainExists_unwrapped(keychain_hash);
}

bool Vault::keychainExists_unwrapped(const bytes_t& keychain_hash) const
{
    odb::result<Keychain> r(db_->query<Keychain>(odb::query<Keychain>::hash == keychain_hash));
    return !r.empty();
}

std::string Vault::getNextAvailableKeychainName_unwrapped(const std::string& desired_keychain_name) const
{
    std::string keychain_name(desired_keychain_name);
    bool bValid = isValidObjectName(keychain_name);
    if (!bValid) { keychain_name = "keychain"; }

/*
    split_name_t split_name = getSplitObjectName(keychain_name);
    keychain_name = split_name.first;
    unsigned int append_num = split_name.second;
*/

    unsigned int append_num = 0;
    std::stringstream ss;
    ss << keychain_name;
    if (!bValid) { ss << " " << ++append_num; }
    while (keychainExists_unwrapped(ss.str()))
    {
        ss.str(std::string());
        ss << keychain_name << " " << ++append_num;
    }

    return ss.str();
}

bool Vault::isKeychainPrivate(const std::string& keychain_name) const
{
    LOGGER(trace) << "Vault::isKeychainPrivate(" << keychain_name << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());
    return isKeychainPrivate_unwrapped(keychain_name);
}

bool Vault::isKeychainPrivate_unwrapped(const std::string& keychain_name) const
{
    std::shared_ptr<Keychain> keychain = getKeychain_unwrapped(keychain_name);
    return keychain->isPrivate();    
}

std::shared_ptr<Keychain> Vault::newKeychain(const std::string& keychain_name, const secure_bytes_t& entropy, const secure_bytes_t& lock_key)
{
    LOGGER(trace) << "Vault::newKeychain(" << keychain_name << ", ...)" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session session;
    odb::core::transaction t(db_->begin());
    {
        odb::result<Keychain> r(db_->query<Keychain>(odb::query<Keychain>::name == keychain_name));
        if (!r.empty()) throw KeychainAlreadyExistsException(keychain_name);
    }

    std::shared_ptr<Keychain> keychain(new Keychain(keychain_name, entropy, lock_key));
    {
        odb::result<Keychain> r(db_->query<Keychain>(odb::query<Keychain>::hash == keychain->hash()));
        if (!r.empty()) throw KeychainAlreadyExistsException(keychain_name);
    }
    persistKeychain_unwrapped(keychain);
    t.commit();

    return keychain;
}

void Vault::renameKeychain(const std::string& old_name, const std::string& new_name)
{
    LOGGER(trace) << "Vault::renameKeychain(" << old_name << ", " << new_name << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::session session;
    odb::core::transaction t(db_->begin());

    odb::result<Keychain> keychain_r(db_->query<Keychain>(odb::query<Keychain>::name == old_name));
    if (keychain_r.empty()) throw KeychainNotFoundException(old_name);

    if (old_name == new_name) return;

    odb::result<Keychain> new_keychain_r(db_->query<Keychain>(odb::query<Keychain>::name == new_name));
    if (!new_keychain_r.empty()) throw KeychainAlreadyExistsException(new_name);

    std::shared_ptr<Keychain> keychain(keychain_r.begin().load());
    keychain->name(new_name);

    db_->update(keychain);
    t.commit();
}

void Vault::persistKeychain_unwrapped(std::shared_ptr<Keychain> keychain)
{
    if (keychain->parent())
        db_->update(keychain->parent());

    db_->persist(keychain);
}

void Vault::updateKeychain_unwrapped(std::shared_ptr<Keychain> keychain)
{
    if (keychain->parent())
        db_->update(keychain->parent());

    db_->update(keychain);
}

std::vector<KeychainView> Vault::getRootKeychainViews(const std::string& account_name, bool get_hidden) const
{
    LOGGER(trace) << "Vault::getRootKeychainViews(" << account_name << ", " << (get_hidden ? "true" : "false") << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());
    return getRootKeychainViews_unwrapped(account_name, get_hidden);
}

std::vector<KeychainView> Vault::getRootKeychainViews_unwrapped(const std::string& account_name, bool get_hidden) const
{
    typedef odb::query<KeychainView> query_t;
    query_t query(1 == 1);
    if (!account_name.empty())
        query = query && (query_t::Account::name == account_name);
    if (getSchemaVersion_unwrapped() >= 4 && !get_hidden)
        query = query && (!query_t::Keychain::hidden);
    odb::result<KeychainView> r(db_->query<KeychainView>(query + "ORDER BY" + query_t::Keychain::name));
    std::vector<KeychainView> views;
    // TODO: figure out why query sometimes returns duplicates.
    std::set<unsigned long> view_ids;
    for (auto& view: r)
    {
        if (view_ids.count(view.id)) continue;
        view_ids.insert(view.id);
        view.is_locked = !mapPrivateKeyUnlock.count(view.name);
        views.push_back(view);
    }
    return views;
}

secure_bytes_t Vault::exportBIP32(const std::string& keychain_name, bool export_private) const
{
    LOGGER(trace) << "Vault::exportBIP32(" << keychain_name << ", " << (export_private ? "true" : "false") << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());
    std::shared_ptr<Keychain> keychain = getKeychain_unwrapped(keychain_name);
    export_private = export_private && keychain->isPrivate();
    if (export_private) unlockKeychain_unwrapped(keychain); 
    return exportBIP32_unwrapped(keychain, export_private);
}

secure_bytes_t Vault::exportBIP32_unwrapped(std::shared_ptr<Keychain> keychain, bool export_private) const
{
    return keychain->exportBIP32(export_private);
}

std::shared_ptr<Keychain> Vault::importBIP32(const std::string& keychain_name, const secure_bytes_t& extkey, const secure_bytes_t& lock_key)
{
    LOGGER(trace) << "Vault::importKeychainExtendedKey(" << keychain_name << ", ...)" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session session;
    odb::core::transaction t(db_->begin());
    odb::result<Keychain> r(db_->query<Keychain>(odb::query<Keychain>::name == keychain_name));
    if (!r.empty()) throw KeychainAlreadyExistsException(keychain_name);

    std::shared_ptr<Keychain> keychain(new Keychain());
    keychain->name(keychain_name);
    keychain->importBIP32(extkey, lock_key);
    persistKeychain_unwrapped(keychain);
    t.commit();

    return keychain;
}

secure_bytes_t Vault::exportBIP39(const std::string& keychain_name) const
{
    LOGGER(trace) << "Vault::exportBIP39(" << keychain_name << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());
    std::shared_ptr<Keychain> keychain = getKeychain_unwrapped(keychain_name);
    unlockKeychain_unwrapped(keychain); 
    return keychain->seed();
}

void Vault::encryptKeychain(const std::string& keychain_name, const secure_bytes_t& lock_key)
{
    LOGGER(trace) << "Vault::encryptKeychain(" << keychain_name << ", ...)" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session s;
    odb::core::transaction t(db_->begin());

    std::shared_ptr<Keychain> keychain = getKeychain_unwrapped(keychain_name);
    unlockKeychain_unwrapped(keychain);
    keychain->encrypt(lock_key);
    updateKeychain_unwrapped(keychain);
    t.commit(); 
}

void Vault::decryptKeychain(const std::string& keychain_name)
{
    LOGGER(trace) << "Vault::unencryptKeychain(" << keychain_name << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session s;
    odb::core::transaction t(db_->begin());

    std::shared_ptr<Keychain> keychain = getKeychain_unwrapped(keychain_name);
    unlockKeychain_unwrapped(keychain);
    keychain->decrypt();
    updateKeychain_unwrapped(keychain);
    t.commit(); 
}

void Vault::refillAccountPool(const std::string& account_name)
{
    LOGGER(trace) << "Vault::refillAccountPool(" << account_name << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    std::shared_ptr<Account> account = getAccount_unwrapped(account_name);
    refillAccountPool_unwrapped(account);
    t.commit();
}

void Vault::refillAccountPool_unwrapped(std::shared_ptr<Account> account)
{
    for (auto& bin: account->bins()) { refillAccountBinPool_unwrapped(bin); }
}

std::shared_ptr<Keychain> Vault::getKeychain(const std::string& keychain_name) const
{
    LOGGER(trace) << "Vault::getKeychain(" << keychain_name << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());
    return getKeychain_unwrapped(keychain_name);
}

std::shared_ptr<Keychain> Vault::getKeychain_unwrapped(const std::string& keychain_name) const
{
    odb::result<Keychain> r(db_->query<Keychain>(odb::query<Keychain>::name == keychain_name));
    if (r.empty()) throw KeychainNotFoundException(keychain_name);

    std::shared_ptr<Keychain> keychain(r.begin().load());
    return keychain;
}

std::vector<std::shared_ptr<Keychain>> Vault::getAllKeychains(bool root_only, bool get_hidden) const
{
    LOGGER(trace) << "Vault::getAllKeychains()" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());
    odb::query<Keychain> query(1 == 1);
    if (root_only)     { query = query && odb::query<Keychain>::parent.is_null();  }
    if (getSchemaVersion_unwrapped() >= 4 && !get_hidden)
        query = query && !odb::query<Keychain>::hidden;
    odb::result<Keychain> r(db_->query<Keychain>(query));

    std::vector<std::shared_ptr<Keychain>> keychains;
    for (auto& keychain: r) { keychains.push_back(keychain.get_shared_ptr()); }
    return keychains;
}

//TODO: do keychain locking notifications properly
void Vault::lockAllKeychains()
{
    LOGGER(trace) << "Vault::lockAllKeychains()" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    mapPrivateKeyUnlock.clear();
    for (auto& item: mapPrivateKeyUnlock)
    {
        notifyKeychainLocked(item.first);
    }
}

void Vault::lockKeychain(const std::string& keychain_name)
{
    LOGGER(trace) << "Vault::lockKeychain(" << keychain_name << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    mapPrivateKeyUnlock.erase(keychain_name);
    notifyKeychainLocked(keychain_name);
}

void Vault::unlockKeychain(const std::string& keychain_name, const secure_bytes_t& lock_key)
{
    LOGGER(trace) << "Vault::unlockKeychain(" << keychain_name << ", ?)" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session s;
    odb::core::transaction t(db_->begin());

    {
        std::shared_ptr<Keychain> keychain = getKeychain_unwrapped(keychain_name);
        if (!keychain->isPrivate())
            throw KeychainIsNotPrivateException(keychain->name());

        // This is the only way to know the lock key is correct
        try
        {
            keychain->unlock(lock_key);
        }
        catch (const std::exception& e)
        {
            LOGGER(error) << "Failed to decrypt keychain " << keychain->name() << ": " << e.what() << std::endl;
            throw KeychainPrivateKeyUnlockFailedException(keychain->name());
        }
    }

    mapPrivateKeyUnlock[keychain_name] = lock_key;
    notifyKeychainUnlocked(keychain_name);
}

void Vault::unlockKeychain_unwrapped(std::shared_ptr<Keychain> keychain, const secure_bytes_t& lock_key) const
{
    if (!keychain->isPrivate())
        throw KeychainIsNotPrivateException(keychain->name());

    if (lock_key.empty())
    {
        const auto& it = mapPrivateKeyUnlock.find(keychain->name());
        if (it == mapPrivateKeyUnlock.end())
            throw KeychainPrivateKeyLockedException(keychain->name());

        keychain->unlock(it->second);
    }
    else
    {
        try
        {
            keychain->unlock(lock_key);
        }
        catch (const AES::AESDecryptException& e)
        {
            throw KeychainPrivateKeyUnlockFailedException(keychain->name());
        }
    }
}

bool Vault::tryUnlockKeychain_unwrapped(std::shared_ptr<Keychain> keychain, const secure_bytes_t& lock_key) const
{
    try
    {
        if (lock_key.empty())
        {
            const auto& it = mapPrivateKeyUnlock.find(keychain->name());
            if (it == mapPrivateKeyUnlock.end()) return false;

            keychain->unlock(it->second);
        }
        else
        {
            keychain->unlock(lock_key);
        }
    }
    catch (const std::exception& e)
    {
        return false;
    }

    return true;
}

bool Vault::isKeychainLocked(const std::string& keychainName) const
{
    const auto& it = mapPrivateKeyUnlock.find(keychainName);
    return (it == mapPrivateKeyUnlock.end());
}

bool Vault::isKeychainEncrypted(const std::string& keychain_name) const
{
    LOGGER(trace) << "Vault::isKeychainEncrypted(" << keychain_name << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session s;
    odb::core::transaction t(db_->begin());

    std::shared_ptr<Keychain> keychain = getKeychain_unwrapped(keychain_name);
    return keychain->isEncrypted();
}

////////////////////////
// ACCOUNT OPERATIONS //
////////////////////////    
void Vault::exportAccount(const std::string& account_name, const std::string& filepath, bool exportprivkeys) const
{
    LOGGER(trace) << "Vault::exportAccount(" << account_name << ", " << filepath << ", " << (exportprivkeys ? "true" : "false") << ", ?)" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif

    // TODO: disallow operation if file is already open
    std::ofstream ofs(filepath);
    boost::archive::text_oarchive oa(ofs);

    odb::core::session s;
    odb::core::transaction t(db_->begin());
    std::shared_ptr<Account> account = getAccount_unwrapped(account_name);

    exportAccount_unwrapped(*account, oa, exportprivkeys);
}

void Vault::exportAccount_unwrapped(Account& account, boost::archive::text_oarchive& oa, bool exportprivkeys) const
{
    if (!exportprivkeys)
        for (auto& keychain: account.keychains()) { keychain->clearPrivateKey(); }

    oa << account;
}

std::shared_ptr<Account> Vault::importAccount(const std::string& filepath, unsigned int& privkeysimported)
{
    LOGGER(trace) << "Vault::importAccount(" << filepath << ", " << privkeysimported << ")" << std::endl;

    std::ifstream ifs(filepath);
    boost::archive::text_iarchive ia(ifs);

    std::shared_ptr<Account> account;
    {
        boost::lock_guard<boost::mutex> lock(mutex);
        odb::core::session s;
        odb::core::transaction t(db_->begin());
        account = importAccount_unwrapped(ia, privkeysimported);
        t.commit();
    }

    signalQueue.flush();
    return account; 
}

std::shared_ptr<Account> Vault::importAccount_unwrapped(boost::archive::text_iarchive& ia, unsigned int& privkeysimported)
{
    std::shared_ptr<Account> account(new Account());
    ia >> *account;

    odb::result<Account> r(db_->query<Account>(odb::query<Account>::hash == account->hash()));
    if (!r.empty()) throw AccountAlreadyExistsException(r.begin().load()->name());

    // In case of missing account name or account name conflict
/*
    std::string account_name = account->name();
    if (account_name.empty()) { account_name = "default"; }
    unsigned int append_num = 1;
    while (accountExists_unwrapped(account->name()))
    {
        std::stringstream ss;
        ss << account_name << "_" << append_num++;
        account->name(ss.str());
    }
*/

    account->name(getNextAvailableAccountName_unwrapped(account->name()));

    // Persist keychains
    bool countprivkeys = (privkeysimported != 0);
    privkeysimported = 0;
    KeychainSet keychains = account->keychains(); // We will replace any duplicate loaded keychains with keychains already in database.
    for (auto& keychain: account->keychains())
    {
        odb::result<Keychain> r(db_->query<Keychain>(odb::query<Keychain>::hash == keychain->hash()));
        if (r.empty())
        {
            // We do not have this keychain. Import it.
            if (countprivkeys) { if (keychain->isPrivate()) privkeysimported++; }
            else               { keychain->clearPrivateKey(); }

/*
            std::string keychain_name = keychain->name();
            unsigned int append_num = 1; // in case of name conflict
            while (keychainExists_unwrapped(keychain->name()))
            {
                std::stringstream ss;
                ss << keychain_name << append_num++;
                keychain->name(ss.str());
            }
*/

            keychain->name(getNextAvailableKeychainName_unwrapped(keychain->name()));
            db_->persist(keychain);
        }
        else
        {
            // We already have this keychain. Just import the private key if possible and use the one we already have.

            std::shared_ptr<Keychain> stored_keychain(r.begin().load());
            if (keychain->isPrivate() && !stored_keychain->isPrivate())
            {
                stored_keychain->importPrivateKey(*keychain);
                db_->update(stored_keychain);
            }
            keychains.erase(keychain);
            keychains.insert(stored_keychain);
        }
    }

    account->keychains(keychains); // We might have replaced loaded keychains with stored keychains.
    db_->persist(account);

    // Create signing scripts and keys and persist account bins
    for (auto& bin: account->bins())
    {
        bin->makeImport();
        db_->persist(bin);

        SigningScriptVector scripts = bin->generateSigningScripts();
        for (auto& script: scripts)
        {
            for (auto& key: script->keys()) { db_->persist(key); }
            db_->persist(script);
        }

        db_->update(bin);
    } 

    // Persist account
    db_->update(account);


    uint32_t replaceBlockTimestamp = account->time_created();
    if (replaceBlockTimestamp > MAX_HORIZON_TIMESTAMP_OFFSET)
        replaceBlockTimestamp -= MAX_HORIZON_TIMESTAMP_OFFSET;
    else
        replaceBlockTimestamp = 0;

    odb::result<BlockHeader> header_r(db_->query<BlockHeader>((odb::query<BlockHeader>::timestamp >= replaceBlockTimestamp) + "ORDER BY" + odb::query<BlockHeader>::height + "ASC LIMIT 1"));
    if (!header_r.empty())
    {
        uint32_t height = header_r.begin()->height();
        LOGGER(trace) << "Vault::importAccount_unwrapped - account has horizon at height " << height << ". Blockchain must be resynched." << std::endl;
        deleteMerkleBlock_unwrapped(height);
    }    
    return account;
}

bool Vault::accountExists(const std::string& account_name) const
{
    LOGGER(trace) << "Vault::accountExists(" << account_name << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());
    return accountExists_unwrapped(account_name);
}

bool Vault::accountExists_unwrapped(const std::string& account_name) const
{
    odb::result<Account> r(db_->query<Account>(odb::query<Account>::name == account_name));
    return !r.empty();
}

std::string Vault::getNextAvailableAccountName_unwrapped(const std::string& desired_account_name) const
{
    std::string account_name(desired_account_name);
    bool bValid = isValidObjectName(account_name);
    if (!bValid) { account_name = "account"; }

/*
    split_name_t split_name = getSplitObjectName(account_name);
    account_name = split_name.first;
    unsigned int append_num = split_name.second;
*/

    unsigned int append_num = 0;

    std::stringstream ss;
    ss << account_name;
    if (!bValid) { ss << " " << ++append_num; }
    while (accountExists_unwrapped(ss.str()))
    {
        ss.str(std::string());
        ss << account_name << " " << ++append_num;
    }

    return ss.str();
}

void Vault::newAccount(const std::string& account_name, unsigned int minsigs, const std::vector<std::string>& keychain_names, uint32_t unused_pool_size, uint32_t time_created, bool compressed_keys, bool use_witness, bool use_witness_p2sh)
{
    LOGGER(trace) << "Vault::newAccount(" << account_name << ", " << minsigs << " of [" << stdutils::delimited_list(keychain_names, ", ") << "], " << unused_pool_size << ", " << time_created << (use_witness ? "true" : "false") << ", " << (use_witness_p2sh ? "true" : "false") << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    odb::result<Account> r(db_->query<Account>(odb::query<Account>::name == account_name));
    if (!r.empty()) throw AccountAlreadyExistsException(account_name);

    KeychainSet keychains;
    for (auto& keychain_name: keychain_names)
    {
        odb::result<Keychain> r(db_->query<Keychain>(odb::query<Keychain>::name == keychain_name));
        if (r.empty()) throw KeychainNotFoundException(keychain_name);
        keychains.insert(r.begin().load());
    }

    std::shared_ptr<Account> account(new Account(account_name, minsigs, keychains, unused_pool_size, time_created, compressed_keys, use_witness, use_witness_p2sh));
    r = db_->query<Account>(odb::query<Account>::hash == account->hash());
    if (!r.empty()) throw AccountAlreadyExistsException(account->name());
    db_->persist(account);

    // The first bin we create must be the change bin.
    std::shared_ptr<AccountBin> changeAccountBin= account->addBin(CHANGE_BIN_NAME);
    db_->persist(changeAccountBin);

    // The second bin we create must be the default bin.
    std::shared_ptr<AccountBin> defaultAccountBin = account->addBin(DEFAULT_BIN_NAME);
    db_->persist(defaultAccountBin);

    for (uint32_t i = 0; i < unused_pool_size; i++)
    {
        std::shared_ptr<SigningScript> changeSigningScript = changeAccountBin->newSigningScript();
        for (auto& key: changeSigningScript->keys()) { db_->persist(key); } 
        db_->persist(changeSigningScript);

        std::shared_ptr<SigningScript> defaultSigningScript = defaultAccountBin->newSigningScript();
        for (auto& key: defaultSigningScript->keys()) { db_->persist(key); }
        db_->persist(defaultSigningScript);
    }
    db_->update(changeAccountBin);
    db_->update(defaultAccountBin);
    db_->update(account);
    t.commit();
}

void Vault::newAccount(bool use_witness, bool use_witness_p2sh, const std::string& account_name, unsigned int minsigs, const std::vector<std::string>& keychain_names, uint32_t unused_pool_size, uint32_t time_created, bool compressed_keys)
{
    newAccount(account_name, minsigs, keychain_names, unused_pool_size, time_created, compressed_keys, use_witness, use_witness_p2sh);
}

void Vault::renameAccount(const std::string& old_name, const std::string& new_name)
{
    LOGGER(trace) << "Vault::renameAccount(" << old_name << ", " << new_name << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::session session;
    odb::core::transaction t(db_->begin());

    odb::result<Account> account_r(db_->query<Account>(odb::query<Account>::name == old_name));
    if (account_r.empty()) throw std::runtime_error("Account not found.");

    if (old_name == new_name) return;

    odb::result<Account> new_account_r(db_->query<Account>(odb::query<Account>::name == new_name));
    if (!new_account_r.empty()) throw std::runtime_error("An account with that name already exists.");

    std::shared_ptr<Account> account(account_r.begin().load());
    account->name(new_name);

    db_->update(account);
    t.commit();
}

std::shared_ptr<Account> Vault::getAccount(const std::string& account_name) const
{
    LOGGER(trace) << "Vault::getAccount(" << account_name << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());
    return getAccount_unwrapped(account_name);
}

std::shared_ptr<Account> Vault::getAccount_unwrapped(const std::string& account_name) const
{
    odb::result<Account> r(db_->query<Account>(odb::query<Account>::name == account_name));
    if (r.empty()) throw AccountNotFoundException(account_name);

    std::shared_ptr<Account> account(r.begin().load());
    return account;
}

std::vector<TxOutView> Vault::getUnspentTxOutViews(const std::string& account_name, uint32_t min_confirmations) const
{
    LOGGER(trace) << "Vault::getUnspentTxOutViews(" << account_name << ", " << min_confirmations << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    std::shared_ptr<Account> account = getAccount_unwrapped(account_name);
    return getUnspentTxOutViews_unwrapped(account, min_confirmations);
}

std::vector<TxOutView> Vault::getUnspentTxOutViews_unwrapped(std::shared_ptr<Account> account, uint32_t min_confirmations) const
{
    typedef odb::query<TxOutView> query_t;
    query_t query(query_t::Tx::status > Tx::UNSIGNED && query_t::TxOut::status == TxOut::UNSPENT && query_t::receiving_account::id == account->id());

    std::vector<TxOutView> utxoviews;
    if (min_confirmations > 0)
    {
        uint32_t best_height = getBestHeight_unwrapped();
        if (min_confirmations > best_height) return utxoviews;
        query = (query && query_t::BlockHeader::height <= best_height + 1 - min_confirmations);
    }
    odb::result<TxOutView> utxoview_r(db_->query<TxOutView>(query + "ORDER BY" + query_t::TxOut::value + "DESC"));
    for (auto& utxoview: utxoview_r) { utxoviews.push_back(utxoview); }

    return utxoviews;
}

AccountInfo Vault::getAccountInfo(const std::string& account_name) const
{
    LOGGER(trace) << "Vault::getAccountInfo(" << account_name << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    std::shared_ptr<Account> account = getAccount_unwrapped(account_name);
    return account->accountInfo();
}

std::vector<AccountInfo> Vault::getAllAccountInfo() const
{
    LOGGER(trace) << "Vault::getAllAccountInfo()" << std::endl;
 
#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    odb::result<Account> r(db_->query<Account>());
    std::vector<AccountInfo> accountInfoVector;
    for (auto& account: r) { accountInfoVector.push_back(account.accountInfo()); }
    return accountInfoVector;
}

uint64_t Vault::getAccountBalance(const std::string& account_name, unsigned int min_confirmations, int tx_flags) const
{
    LOGGER(trace) << "Vault::getAccountBalance(" << account_name << ", " << min_confirmations << ")" << std::endl;

    std::vector<Tx::status_t> tx_statuses = Tx::getStatusFlags(tx_flags);

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());
    typedef odb::query<BalanceView> query_t;
    query_t query(query_t::Account::name == account_name && query_t::TxOut::status == TxOut::UNSPENT && query_t::Tx::status.in_range(tx_statuses.begin(), tx_statuses.end()));
    if (min_confirmations > 0)
    {
        uint32_t best_height = getBestHeight_unwrapped();
        if (min_confirmations > best_height) return 0;
        query = (query && query_t::BlockHeader::height <= best_height + 1 - min_confirmations);
    }
    odb::result<BalanceView> r(db_->query<BalanceView>(query));
    return r.empty() ? 0 : r.begin()->balance;
}

std::shared_ptr<AccountBin> Vault::addAccountBin(const std::string& account_name, const std::string& bin_name)
{
    LOGGER(trace) << "Vault::addAccountBin(" << account_name << ", " << bin_name << ")" << std::endl;

    if (bin_name.empty() || bin_name[0] == '@') throw std::runtime_error("Invalid account bin name.");

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::session s;
    odb::core::transaction t(db_->begin());

    bool binExists = true;
    try
    {
        getAccountBin_unwrapped(account_name, bin_name);
    }
    catch (const AccountBinNotFoundException& e)
    {
        binExists = false;
    }

    if (binExists) throw AccountBinAlreadyExistsException(account_name, bin_name);

    std::shared_ptr<Account> account = getAccount_unwrapped(account_name);

    std::shared_ptr<AccountBin> bin = account->addBin(bin_name);
    db_->persist(bin);

    for (uint32_t i = 0; i < account->unused_pool_size(); i++)
    {
        std::shared_ptr<SigningScript> script = bin->newSigningScript();
        for (auto& key: script->keys()) { db_->persist(key); }
        db_->persist(script);
    }
    db_->update(bin);
    db_->update(account);
    t.commit();

    return bin;
}

std::shared_ptr<SigningScript> Vault::issueSigningScript(const std::string& account_name, const std::string& bin_name, const std::string& label, uint32_t index, const std::string& username)
{
    LOGGER(trace) << "Vault::issueSigningScript(" << account_name << ", " << bin_name << ", " << label << ", " << index << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    if (!accountExists_unwrapped(account_name)) throw AccountNotFoundException(account_name);
    std::shared_ptr<AccountBin> bin = getAccountBin_unwrapped(account_name, bin_name);
    if (bin->isChange()) throw AccountCannotIssueChangeScriptException(account_name);
    std::shared_ptr<SigningScript> script = issueAccountBinSigningScript_unwrapped(bin, label, index);

    if (!username.empty())
    {
        std::shared_ptr<Contact> contact;
        odb::result<Contact> r(db_->query<Contact>(odb::query<Contact>::username == username));
        if (r.empty())
        {
            contact = std::make_shared<Contact>(username);
            db_->persist(contact);
        }
        else
        {
            contact = r.begin().load();
        }

        script->contact(contact);
        db_->update(script);
    }
    t.commit();
    return script;
}

std::shared_ptr<SigningScript> Vault::issueAccountBinSigningScript_unwrapped(std::shared_ptr<AccountBin> bin, const std::string& label, uint32_t index)
{
    refillAccountBinPool_unwrapped(bin, index);

    // Get either the specified script or the next available unused signing script if index = 0
    typedef odb::query<SigningScriptView> view_query_t;
    view_query_t view_query(view_query_t::AccountBin::id == bin->id());
    if (index > 0)  { view_query = view_query && view_query_t::SigningScript::index == index; }
    else            { view_query = (view_query && view_query_t::SigningScript::status == SigningScript::UNUSED) + "ORDER BY" + view_query_t::SigningScript::index + "LIMIT 1"; }

    odb::result<SigningScriptView> view_result(db_->query<SigningScriptView>(view_query));

/*
    odb::result<SigningScriptView> view_result(db_->query<SigningScriptView>(
        (view_query::AccountBin::id == bin->id() && view_query::SigningScript::status == SigningScript::UNUSED) +
        "ORDER BY" + view_query::SigningScript::index + "LIMIT 1"));
*/
    if (view_result.empty()) throw AccountBinOutOfScriptsException(bin->account_name(), bin->name());

    std::shared_ptr<SigningScriptView> view(view_result.begin().load());

    typedef odb::query<SigningScript> script_query;
    odb::result<SigningScript> script_result(db_->query<SigningScript>(script_query::id == view->id));
    std::shared_ptr<SigningScript> script(script_result.begin().load());
    script->label(label);
    script->status(SigningScript::ISSUED);
    db_->update(script);
    bin->markSigningScriptIssued(script->index());
    db_->update(bin);
    return script;
}

void Vault::refillAccountBinPool_unwrapped(std::shared_ptr<AccountBin> bin, uint32_t index)
{
    // get largest signing script index that is not unused
    typedef odb::query<ScriptCountView> count_query_t;
    odb::result<ScriptCountView> count_result(db_->query<ScriptCountView>(count_query_t::AccountBin::id == bin->id() && count_query_t::SigningScript::status != SigningScript::UNUSED));
    uint32_t max_index = count_result.empty() ? 0 : count_result.begin()->max_index;
    uint32_t max_unused_index = (index > max_index) ? index : max_index;

    if (max_unused_index > 0)
    {
        // update any unused signing scripts with smaller index to issued status
        typedef odb::query<SigningScriptView> script_query_t;
        odb::result<SigningScriptView> script_result(db_->query<SigningScriptView>(script_query_t::AccountBin::id == bin->id() &&
            script_query_t::SigningScript::status == SigningScript::UNUSED && script_query_t::SigningScript::index < max_unused_index));
        for (auto& script_view: script_result)
        {
            std::shared_ptr<SigningScript> script(db_->load<SigningScript>(script_view.id));
            script->status(SigningScript::ISSUED);
            db_->update(script);
        }
    }

    // create any additional scripts to have all up to specified index issued
    if (index > max_index)
    {
        count_result = db_->query<ScriptCountView>();
        uint32_t count = count_result.empty() ? 0 : count_result.begin().load()->count;
        for (uint32_t i = count + 1; i < index; i++)
        {
            std::shared_ptr<SigningScript> script = bin->newSigningScript();
            script->status(SigningScript::ISSUED);
            for (auto& key: script->keys()) { db_->persist(key); }
            db_->persist(script); 
        }
    }

    // refill remaining pool
    count_result = db_->query<ScriptCountView>(count_query_t::AccountBin::id == bin->id() && count_query_t::SigningScript::status == SigningScript::UNUSED);
    uint32_t count = count_result.empty() ? 0 : count_result.begin().load()->count;

    uint32_t unused_pool_size = bin->account() ? bin->account()->unused_pool_size() : DEFAULT_UNUSED_POOL_SIZE;
    for (uint32_t i = count; i < unused_pool_size; i++)
    {
        std::shared_ptr<SigningScript> script = bin->newSigningScript();
        for (auto& key: script->keys()) { db_->persist(key); }
        db_->persist(script); 
    } 
    db_->update(bin);
}

std::vector<SigningScriptView> Vault::getSigningScriptViews(const std::string& account_name, const std::string& bin_name, int flags) const
{
    LOGGER(trace) << "Vault::getSigningScriptViews(" << account_name << ", " << bin_name << ", " << SigningScript::getStatusString(flags) << ")" << std::endl;

    std::vector<SigningScript::status_t> statusRange = SigningScript::getStatusFlags(flags);

    typedef odb::query<SigningScriptView> query_t;
    query_t query(query_t::SigningScript::status.in_range(statusRange.begin(), statusRange.end()));
    if (!account_name.empty()) query = (query && query_t::Account::name == account_name);
    if (!bin_name.empty())     query = (query && query_t::AccountBin::name == bin_name);
    query += "ORDER BY" + query_t::Account::name + "ASC," + query_t::AccountBin::name + "ASC," + query_t::SigningScript::status + "DESC," + query_t::SigningScript::index + "ASC";

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::session s;
    odb::core::transaction t(db_->begin());

    std::vector<SigningScriptView> views;
    odb::result<SigningScriptView> r(db_->query<SigningScriptView>(query));
    for (auto& view: r) { views.push_back(view); }
    return views;
}

std::vector<TxOutView> Vault::getTxOutViews(const std::string& account_name, const std::string& bin_name, int role_flags, int txout_status_flags, int tx_status_flags, bool hide_change) const
{
    LOGGER(trace) << "Vault::getTxOutViews(" << account_name << ", " << bin_name << ", " << TxOut::getRoleString(role_flags) << ", " << TxOut::getStatusString(txout_status_flags) << ", " << ", " << Tx::getStatusString(tx_status_flags) << ")" << std::endl;

    typedef odb::query<TxOutView> query_t;
    query_t query(query_t::receiving_account::id != 0 || query_t::sending_account::id != 0);
    if (!account_name.empty())
    {
        query_t role_query(1 == 1);
        if (role_flags & TxOut::ROLE_SENDER)    role_query = (role_query && (query_t::sending_account::name == account_name));
        if (role_flags & TxOut::ROLE_RECEIVER)  role_query = (role_query || (query_t::receiving_account::name == account_name));
        query = query && role_query;
    }
    if (!bin_name.empty())                      query = (query && query_t::AccountBin::name == bin_name);
    if (hide_change)                            query = (query && (query_t::TxOut::account_bin.is_null() || query_t::AccountBin::name != CHANGE_BIN_NAME));

    std::vector<TxOut::status_t> txout_statuses = TxOut::getStatusFlags(txout_status_flags);
    query = (query && query_t::TxOut::status.in_range(txout_statuses.begin(), txout_statuses.end()));

    if (tx_status_flags != Tx::ALL)
    {
        std::vector<Tx::status_t> tx_statuses = Tx::getStatusFlags(tx_status_flags);
        query = (query && query_t::Tx::status.in_range(tx_statuses.begin(), tx_statuses.end()));
    }

    query += "ORDER BY" + query_t::BlockHeader::height + "DESC," + query_t::Tx::timestamp + "DESC," + query_t::Tx::id + "DESC";

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());
    std::vector<TxOutView> views;
    odb::result<TxOutView> r(db_->query<TxOutView>(query));
    for (auto& view: r)
    {
        view.updateRole(role_flags);
        std::vector<TxOutView> split_views = view.getSplitRoles(TxOut::ROLE_RECEIVER, account_name);
        for (auto& split_view: split_views) { views.push_back(split_view); }
    }
    return views;
}


////////////////////////////
// ACCOUNT BIN OPERATIONS //
////////////////////////////    
std::shared_ptr<AccountBin> Vault::getAccountBin(const std::string& account_name, const std::string& bin_name) const
{
    LOGGER(trace) << "Vault::getAccountBin(" << account_name << ", " << bin_name << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    std::shared_ptr<AccountBin> bin = getAccountBin_unwrapped(account_name, bin_name);
    return bin; 
}

std::shared_ptr<AccountBin> Vault::getAccountBin_unwrapped(const std::string& account_name, const std::string& bin_name) const
{
    typedef odb::query<AccountBin> query_t;
    query_t query(query_t::name == bin_name);
    if (account_name.empty())   { query = query && query_t::account.is_null();              }
    else                        { query = query && query_t::account->name == account_name;  }
    odb::result<AccountBin> r(db_->query<AccountBin>(query));
    if (r.empty()) throw AccountBinNotFoundException(account_name, bin_name);
    return r.begin().load();
}

std::vector<AccountBinView> Vault::getAllAccountBinViews() const
{
    LOGGER(trace) << "Vault::getAllAccountBinViews()" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());
    odb::result<AccountBinView> r(db_->query<AccountBinView>());
    std::vector<AccountBinView> views;
    for (auto& view: r) { views.push_back(view); }
    return views; 
}

void Vault::exportAccountBin(const std::string& account_name, const std::string& bin_name, const std::string& export_name, const std::string& filepath) const
{
    LOGGER(trace) << "Vault::exportAccountBin(" << account_name << ", " << bin_name << ", " << filepath << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    std::shared_ptr<AccountBin> bin = getAccountBin_unwrapped(account_name, bin_name);
    exportAccountBin_unwrapped(bin, export_name, filepath);
}

void Vault::exportAccountBin_unwrapped(const std::shared_ptr<AccountBin> account_bin, const std::string& export_name, const std::string& filepath) const
{
    account_bin->makeExport(export_name);
    std::ofstream ofs(filepath);
    boost::archive::text_oarchive oa(ofs);
    oa << *account_bin;
}

std::shared_ptr<AccountBin> Vault::importAccountBin(const std::string& filepath)
{
    LOGGER(trace) << "Vault::importAccountBin(" << filepath << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    std::shared_ptr<AccountBin> bin = importAccountBin_unwrapped(filepath);
    t.commit();
    return bin;
}

std::shared_ptr<AccountBin> Vault::importAccountBin_unwrapped(const std::string& filepath)
{
    std::shared_ptr<AccountBin> bin(new AccountBin());
    {
        std::ifstream ifs(filepath);
        boost::archive::text_iarchive ia(ifs);
        ia >> *bin;
    }
    bin->updateHash();

    odb::result<AccountBin> r(db_->query<AccountBin>(odb::query<AccountBin>::hash == bin->hash()));
    if (!r.empty())
    {
        std::shared_ptr<AccountBin> bin(r.begin().load());
        throw AccountBinAlreadyExistsException(bin->account_name(), bin->name());
    }

    // In case of bin name conflict
    // TODO: detect account bin duplicates.
    std::string bin_name = bin->name();
    unsigned int append_num = 1;
    while (true)
    {
        odb::result<AccountBin> r(db_->query<AccountBin>(odb::query<AccountBin>::name == bin->name()));
        if (r.empty()) break;

        std::stringstream ss;
        ss << bin_name << append_num++;
        bin->name(ss.str());
    }

    // Persist keychains
    std::string keychain_base_name = bin->name();
    unsigned int keychain_append_num = 1;
    auto& loaded_keychains = bin->keychains();
    KeychainSet stored_keychains; // We will replace any duplicate loaded keychains with keychains already in database.
    for (auto& keychain: loaded_keychains)
    {
        odb::result<Keychain> r(db_->query<Keychain>(odb::query<Keychain>::hash == keychain->hash()));
        if (r.empty())
        {
            do
            {
                std::stringstream ss;
                ss << keychain_base_name << "(" << keychain_append_num++ << ")";
                keychain->name(ss.str());
            }
            while (keychainExists_unwrapped(keychain->name()));

            keychain->hidden(true); // Do not display this keychain in UI.
            db_->persist(keychain);
            stored_keychains.insert(keychain);
        }
        else
        {
            // We already have this keychain. Just replace the loaded one with the stored one.
            std::shared_ptr<Keychain> stored_keychain(r.begin().load());
            stored_keychains.insert(stored_keychain);
        }
    }

    bin->keychains(stored_keychains); // Replace loaded keychains with stored keychains.

    // Create signing scripts and keys and persist account bin
    db_->persist(bin);

    unsigned int next_script_index = bin->next_script_index();
    for (unsigned int i = 0; i < next_script_index; i++)
    {
        std::shared_ptr<SigningScript> script = bin->newSigningScript();
        script->status(SigningScript::ISSUED);
        for (auto& key: script->keys()) { db_->persist(key); }
        db_->persist(script);
    }
    for (unsigned int i = 0; i < DEFAULT_UNUSED_POOL_SIZE; i++)
    {
        std::shared_ptr<SigningScript> script = bin->newSigningScript();
        for (auto& key: script->keys()) { db_->persist(key); }
        db_->persist(script);
    }
    db_->update(bin);
    
    return bin;
}


////////////////////////////
// TRANSACTION OPERATIONS //
////////////////////////////
std::shared_ptr<Tx> Vault::getTx(const bytes_t& hash) const
{
    LOGGER(trace) << "Vault::getTx(" << uchar_vector(hash).getHex() << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    return getTx_unwrapped(hash);
}

std::shared_ptr<Tx> Vault::getTx_unwrapped(const bytes_t& hash) const
{
    odb::result<Tx> r(db_->query<Tx>(odb::query<Tx>::hash == hash || odb::query<Tx>::unsigned_hash == hash));
    if (r.empty()) throw TxNotFoundException(hash);

    std::shared_ptr<Tx> tx(r.begin().load());
    return tx;
}

std::shared_ptr<Tx> Vault::getTx(unsigned long tx_id) const
{
    LOGGER(trace) << "Vault::getTx(" << tx_id << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    return getTx_unwrapped(tx_id);
}

std::shared_ptr<Tx> Vault::getTx_unwrapped(unsigned long tx_id) const
{
    odb::result<Tx> r(db_->query<Tx>(odb::query<Tx>::id == tx_id));
    if (r.empty()) throw TxNotFoundException();

    std::shared_ptr<Tx> tx(r.begin().load());
    return tx;
}

txs_t Vault::getTxs(int tx_status_flags, unsigned long start, int count, uint32_t minheight) const
{
    LOGGER(trace) << "Vault::getTxs(" << Tx::getStatusString(tx_status_flags) << ", " << start << ", " << count << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    return getTxs_unwrapped(tx_status_flags, start, count, minheight);
}

txs_t Vault::getTxs_unwrapped(int tx_status_flags, unsigned long start, int count, uint32_t minheight) const
{
    txs_t txs;

    typedef odb::query<Tx> query_t;
    query_t query (1 == 1);
    if (tx_status_flags != Tx::ALL)
    {
        std::vector<Tx::status_t> tx_statuses = Tx::getStatusFlags(tx_status_flags);
        query = query && query_t::status.in_range(tx_statuses.begin(), tx_statuses.end());
    }

    if (minheight > 0)
    {
        query = query && (query_t::blockheader->height >= minheight);
    }

    query += "ORDER BY" + query_t::blockheader->height + "ASC," + query_t::timestamp + "ASC," + query_t::id + "ASC";
    if (start != 0 || count != -1)
    {
        if (count == -1) { count = 0xffffffff; } // TODO: do this correctly
        std::stringstream ss;
        ss << "LIMIT " << start << "," << count;
        query = query + ss.str().c_str();
    }
    odb::result<Tx> r(db_->query<Tx>(query));
    for (odb::result<Tx>::iterator it = r.begin(); it != r.end(); ++it) { txs.push_back(it.load()); }
    return txs; 
}

std::vector<std::string> Vault::getSerializedUnsignedTxs(const std::string& account_name) const
{
    LOGGER(trace) << "Vault::getSerializedUnsignedTxs(" << account_name << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    return getSerializedUnsignedTxs_unwrapped(account_name);
}

std::vector<std::string> Vault::getSerializedUnsignedTxs_unwrapped(const std::string& account_name) const
{
    std::shared_ptr<Account> account = getAccount_unwrapped(account_name);

    odb::result<TxOutView> view_r(db_->query<TxOutView>(odb::query<TxOutView>::sending_account::id == account->id() && odb::query<TxOutView>::Tx::status == Tx::UNSIGNED));

    std::set<unsigned long> txIds;
    for (auto& view: view_r) { txIds.insert(view.tx_id); }

    odb::result<Tx> tx_r(db_->query<Tx>(odb::query<Tx>::id.in_range(txIds.begin(), txIds.end())));

    std::vector<std::string> serializedTxs;
    for (auto& tx: tx_r) { serializedTxs.push_back(tx.toSerialized()); }
    return serializedTxs;
}

uint32_t Vault::getTxConfirmations(const bytes_t& hash) const
{
    LOGGER(trace) << "Vault::getTxConfirmations(" << uchar_vector(hash).getHex() << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    std::shared_ptr<Tx> tx = getTx_unwrapped(hash);
    return getTxConfirmations_unwrapped(tx);
}
    
uint32_t Vault::getTxConfirmations(unsigned long tx_id) const
{
    LOGGER(trace) << "Vault::getTxConfirmations(" << tx_id << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    std::shared_ptr<Tx> tx = getTx_unwrapped(tx_id);
    return getTxConfirmations_unwrapped(tx);
}
    
uint32_t Vault::getTxConfirmations(std::shared_ptr<Tx> tx) const
{
    LOGGER(trace) << "Vault::getTxConfirmations(tx: " << uchar_vector(tx->hash()).getHex() << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    return getTxConfirmations_unwrapped(tx);
}

uint32_t Vault::getTxConfirmations_unwrapped(std::shared_ptr<Tx> tx) const
{
    if (!tx->blockheader()) return 0;
    return (getBestHeight_unwrapped() + 1 - tx->blockheader()->height());
}
 
std::vector<TxView> Vault::getTxViews(int tx_status_flags, unsigned long start, int count, uint32_t minheight) const
{
    LOGGER(trace) << "Vault::getTxViews(" << Tx::getStatusString(tx_status_flags) << ", " << start << ", " << count << ")" << std::endl;

    typedef odb::query<TxView> query_t;
    query_t query (1 == 1);
    if (tx_status_flags != Tx::ALL)
    {
        std::vector<Tx::status_t> tx_statuses = Tx::getStatusFlags(tx_status_flags);
        query = query && query_t::Tx::status.in_range(tx_statuses.begin(), tx_statuses.end());
    }

    if (minheight > 0)
    {
        query = query && (query_t::BlockHeader::height >= minheight);
    }

    query += "ORDER BY" + query_t::BlockHeader::height + "DESC," + query_t::Tx::timestamp + "DESC," + query_t::Tx::id + "DESC";
    if (start != 0 || count != -1)
    {
        if (count == -1) { count = 0xffffffff; } // TODO: do this correctly
        std::stringstream ss;
        ss << "LIMIT " << start << "," << count;
        query = query + ss.str().c_str();
    }

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());
    std::vector<TxView> views;
    odb::result<TxView> r(db_->query<TxView>(query));
    for (auto& view: r) { views.push_back(view); }
    return views; 
}

std::shared_ptr<Tx> Vault::insertTx(std::shared_ptr<Tx> tx, bool replace_labels)
{
    LOGGER(trace) << "Vault::insertTx(...) - hash: " << uchar_vector(tx->hash()).getHex() << ", unsigned hash: " << uchar_vector(tx->unsigned_hash()).getHex() << ", replace_labels: " << (replace_labels ? "true" : "false") << std::endl;

    {
        boost::lock_guard<boost::mutex> lock(mutex);
        odb::core::session s;
        odb::core::transaction t(db_->begin());
        tx = insertTx_unwrapped(tx, replace_labels);
        if (tx) t.commit();
    }

    signalQueue.flush();
    return tx;
}

std::shared_ptr<Tx> Vault::insertTx_unwrapped(std::shared_ptr<Tx> tx, bool replace_labels)
{
    try
    {
        tx->updateStatus();
        Coin::Transaction cointx(tx->toCoinCore());
        std::string hashstr = uchar_vector(tx->hash()).getHex();
        std::string unsignedhashstr = uchar_vector(tx->unsigned_hash()).getHex();
        LOGGER(trace) << "Vault::insertTx_unwrapped(...) - hash: " << hashstr << ", unsigned hash: " << unsignedhashstr << std::endl;


        odb::result<Tx> tx_r(db_->query<Tx>(odb::query<Tx>::unsigned_hash == tx->unsigned_hash()));

        // First handle situations where we have a duplicate
        if (!tx_r.empty())
        {
            LOGGER(debug) << "Vault::insertTx_unwrapped - We have a transaction with the same unsigned hash: " << unsignedhashstr << std::endl;
            std::shared_ptr<Tx> stored_tx(tx_r.begin().load());

            Coin::Transaction stored_cointx(stored_tx->toCoinCore());

            // Sanity check: TxIn and TxOut counts should match
            if (tx->txins().size() != stored_tx->txins().size() ||
                tx->txouts().size() != stored_tx->txouts().size())
            {
                throw TxMismatchException(stored_tx->hash());
            }

            // We need to set the outpoints for sighash operations in updateStatus.
            bool checksigs = true;
            for (auto& txin: tx->txins())
            {
                txin->outpoint(stored_tx->txins()[txin->txindex()]->outpoint());
                if (!txin->outpoint()) { checksigs = false; }
            }

            tx->updateStatus(Tx::NO_STATUS, checksigs);
            LOGGER(trace) << "tx status: " << tx->getStatusString() << std::endl;

            bool updated = false;

            // Update labels.
            std::size_t i = 0;
            txouts_t txouts = tx->txouts();
            for (auto& txout: stored_tx->txouts())
            {
                bool labels_updated = false;

                if (!txouts[i]->sending_label().empty() && (replace_labels || txout->sending_label().empty()))
                {
                    txout->sending_label(txouts[i]->sending_label());
                    labels_updated = true;
                }

                if (!txouts[i]->receiving_label().empty() && (replace_labels || txout->receiving_label().empty()))
                {
                    txout->receiving_label(txouts[i]->receiving_label());
                    labels_updated = true;
                }

                if (labels_updated)
                {
                    db_->update(txout);
                    updated = true;
                }

                i++;
            }

            if (updated)
            {
                LOGGER(debug) << "Vault::insertTx_unwrapped - LABELS UPDATED. hash: " << hashstr << std::endl;
            }

            // Update signatures.
            if (stored_tx->status() == Tx::UNSIGNED)
            {
                if (tx->status() != Tx::UNSIGNED)
                {
                    // The transaction we received is a signed version of the one we had unsigned, so replace
                    LOGGER(debug) << "Vault::insertTx_unwrapped - REPLACING OLD UNSIGNED TRANSACTION WITH NEW SIGNED TRANSACTION. hash: " << uchar_vector(tx->hash()).getHex() << std::endl; 
                    std::size_t i = 0;
                    txins_t txins = tx->txins();
                    for (auto& txin: stored_tx->txins())
                    {
                        txin->script(txins[i]->script());
                        txin->scriptwitnessstack(txins[i]->scriptwitnessstack());
                        db_->update(txin);
                        i++;
                    }
                    stored_tx->updateStatus(tx->status(), true);
                    db_->update(stored_tx);
                    updated = true;
                }
                else
                {
                    // The transaction we received is unsigned but might have more signatures. Merge signatures
                    bool sigs_updated = false;
                    std::size_t i = 0;
                    txins_t txins = tx->txins();
                    for (auto& txin: stored_tx->txins())
                    {
                        using namespace CoinQ::Script;
                        uint64_t outpointvalue = txin->outpoint() ? txin->outpoint()->value() : 0;
                        SignableTxIn stored_stxin(stored_cointx, i, outpointvalue);
                        SignableTxIn new_stxin(cointx, i, outpointvalue); 
                        unsigned int sigsadded = stored_stxin.mergesigs(new_stxin);
                        if (sigsadded > 0)
                        {
                            int sigsneeded = stored_stxin.sigsneeded();
                            LOGGER(debug) << "Vault::insertTx_unwrapped - ADDED " << sigsadded << " NEW SIGNATURE(S) TO INPUT " << i << ", " << sigsneeded << " STILL NEEDED." << std::endl;
                            txin->script(stored_stxin.txinscript());
                            std::vector<bytes_t> stack;
                            for (auto& item: stored_stxin.scriptwitness().stack) { stack.push_back(item); }
                            txin->scriptwitnessstack(stack);
                            db_->update(txin);
                            sigs_updated = true;
                        }
                        i++;
                    }

                    if (sigs_updated)
                    {
                        stored_tx->updateStatus(Tx::NO_STATUS, true);
                        db_->update(stored_tx);
                        updated = true;
                    }
                }
            }
            else
            {
                // The transaction we currently have is already fully signed, so only update status if necessary
                if (tx->status() != Tx::UNSIGNED)
                {
                    if (tx->status() > stored_tx->status())
                    {
                        LOGGER(debug) << "Vault::insertTx_unwrapped - UPDATING TRANSACTION STATUS FROM " << stored_tx->status() << " TO " << tx->status() << ". hash: " << uchar_vector(stored_tx->hash()).getHex() << std::endl;
                        stored_tx->updateStatus(tx->status());
                        db_->update(stored_tx);
                        updated = true;
                    }
                    else
                    {
                        LOGGER(debug) << "Vault::insertTx_unwrapped - Transaction signatures not updated. hash: " << uchar_vector(stored_tx->hash()).getHex() << std::endl;
                    }
                }
                else
                {
                    LOGGER(debug) << "Vault::insertTx_unwrapped - Stored transaction is already signed, received transaction is missing signatures. Ignore signatures. hash: " << uchar_vector(stored_tx->hash()).getHex() << std::endl;
                }
            }

            if (!updated) return nullptr;

            updateConfirmations_unwrapped(stored_tx);
            signalQueue.push(notifyTxUpdated.bind(stored_tx));
            return stored_tx;
        }

        // If we get here it means we've either never seen this transaction before or it doesn't affect our accounts.

        std::set<std::shared_ptr<Tx>> conflicting_txs;
        std::set<std::shared_ptr<TxIn>> updated_txins;
        std::set<std::shared_ptr<TxOut>> updated_txouts;
        std::set<std::shared_ptr<Tx>> updated_txs;

        // Check inputs
        bool sent_from_vault = false; // whether any of the inputs belong to vault
        std::shared_ptr<Account> sending_account;

        for (auto& txin: tx->txins())
        {
            // Check if inputs connect
            tx_r = db_->query<Tx>(odb::query<Tx>::hash == txin->outhash());
            if (tx_r.empty())
            {
                // The txinscript is in one of our accounts but we don't have the outpoint, 
                txin->outpoint(nullptr);
                bytes_t txoutscript;
                try
                {
                    CoinQ::Script::SignableTxIn signabletxin(cointx, txin->txindex(), 0);
                    txoutscript = signabletxin.txoutscript();
LOGGER(trace) << "txoutscript!!! " << uchar_vector(txoutscript).getHex() << std::endl;
                }
                catch (const std::exception& e)
                {
                    // TODO: handle errors
                }
                if (!txoutscript.empty())
                {
                    odb::result<SigningScript> script_r(db_->query<SigningScript>(odb::query<SigningScript>::txoutscript == txoutscript));
                    if (!script_r.empty())
                    {
                        sent_from_vault = true;
                        if (!sending_account)
                        {
                            // Assuming all inputs belong to the same account
                            // TODO: Allow coin mixing
                            std::shared_ptr<SigningScript> script(script_r.begin().load());
                            sending_account = script->account();
                        }
                    }
                }
            }
            else
            {
                std::shared_ptr<Tx> spent_tx(tx_r.begin().load());
                txouts_t outpoints = spent_tx->txouts();
                uint32_t outindex = txin->outindex();
                if (outpoints.size() <= outindex) throw std::runtime_error("Vault::insertTx_unwrapped - outpoint out of range.");
                std::shared_ptr<TxOut>& outpoint = outpoints[outindex];
                txin->outpoint(outpoint);

                // Check for double spend, track conflicted transaction so we can update status if necessary later.
                std::shared_ptr<TxIn> conflict_txin = outpoint->spent();
                if (conflict_txin)
                {
                    LOGGER(debug) << "Vault::insertTx_unwrapped - Discovered conflicting transaction. Double spend. hash: " << uchar_vector(conflict_txin->tx()->hash()).getHex() << std::endl;
                    conflicting_txs.insert(conflict_txin->tx());
                } 

                // Was this transaction signed using one of our accounts?
                odb::result<SigningScript> script_r(db_->query<SigningScript>(odb::query<SigningScript>::txoutscript == outpoint->script()));
                if (!script_r.empty())
                {
                    sent_from_vault = true;
                    outpoint->spent(txin);
                    updated_txouts.insert(outpoint);
                    if (!sending_account)
                    {
                        // Assuming all inputs belong to the same account
                        // TODO: Allow coin mixing
                        std::shared_ptr<SigningScript> script(script_r.begin().load());
                        sending_account = script->account();
                    }
                }
            }
            if (sent_from_vault)
            {
                tx->updateStatus(Tx::NO_STATUS, true);
                LOGGER(trace) << "sent from vault tx status: " << tx->getStatusString() << std::endl;
            }
        }

        // Check outputs
        bool sent_to_vault = false; // whether any of the outputs are spendable by accounts in vault
        for (auto& txout: tx->txouts())
        {
            // Assume all inputs sent from same account.
            // TODO: Allow coin mixing.
            if (sending_account) { txout->sending_account(sending_account); }

            odb::result<SigningScript> script_r(db_->query<SigningScript>(odb::query<SigningScript>::txoutscript == txout->script()));
            if (!script_r.empty())
            {
                // This output is spendable from an account in the vault
                sent_to_vault = true;
                std::shared_ptr<SigningScript> script(script_r.begin().load());
                txout->signingscript(script);

                // Update the signing script and txout status
                switch (script->status())
                {
                case SigningScript::UNUSED:
                    if (sent_from_vault && script->account_bin()->isChange())
                    {
                        script->status(SigningScript::CHANGE);
                    }
                    else
                    {
                        script->status(SigningScript::USED);
                    }
                    db_->update(script);
                    refillAccountBinPool_unwrapped(script->account_bin());
                    break;

                case SigningScript::ISSUED:
                    script->status(SigningScript::USED);
                    db_->update(script);
                    break;

                default:
                    break;
                }

                // Check if the output has already been spent (transactions inserted out of order)
                odb::result<TxIn> txin_r(db_->query<TxIn>(odb::query<TxIn>::outhash == tx->hash() && odb::query<TxIn>::outindex == txout->txindex()));
                if (!txin_r.empty())
                {
                    LOGGER(debug) << "Vault::insertTx_unwrapped - out of order insertion." << std::endl;
                    std::shared_ptr<TxIn> txin(txin_r.begin().load());
                    if (!txin->tx()) throw std::runtime_error("Tx is null for txin.");
                    txout->spent(txin);
                    txin->outpoint(txout); // We now have the outpoint. TODO: deal with conflicts.
                    updated_txins.insert(txin);

                    std::shared_ptr<Tx> tx(txin->tx());
                    if (tx)
                    {
                        tx->updateTotals();
                        updated_txs.insert(tx);
                    }
                }
            }
        }

        if (!conflicting_txs.empty())
        {
            tx->conflicting(true);
            for (auto& conflicting_tx: conflicting_txs)
            {
                if (conflicting_tx->status() != Tx::CONFIRMED)
                {
                    conflicting_tx->conflicting(true);
                    db_->update(conflicting_tx);
                    signalQueue.push(notifyTxUpdated.bind(conflicting_tx));
                    //notifyTxUpdated(conflicting_tx);
                }
            }
        }

        if (sent_from_vault || sent_to_vault)
        {
            // TODO: better tx status update method
            if (!sent_from_vault) { tx->status(Tx::PROPAGATED); tx->hash(tx->toCoinCore().hash()); }
            LOGGER(debug) << "Vault::insertTx_unwrapped - INSERTING NEW TRANSACTION. hash: " << uchar_vector(tx->hash()).getHex() << ", unsigned hash: " << uchar_vector(tx->unsigned_hash()).getHex() << std::endl;
            tx->updateTotals();

            // Persist the transaction
            db_->persist(*tx);
            for (auto& txin:        tx->txins())    { db_->persist(txin);       }
            for (auto& txout:       tx->txouts())   { db_->persist(txout);      }

            // Update other affected objects
            for (auto& txin:        updated_txins)  { db_->update(txin);        }
            for (auto& txout:       updated_txouts) { db_->update(txout);       }
            for (auto& tx:          updated_txs)    { db_->update(tx);          }

            if (tx->status() >= Tx::SENT) updateConfirmations_unwrapped(tx);
            signalQueue.push(notifyTxInserted.bind(tx));
            //notifyTxInserted(tx);
            return tx;
        }

        LOGGER(debug) << "Vault::insertTx_unwrapped - transaction not inserted." << std::endl;
        return nullptr;
    }
    catch (...)
    {
        signalQueue.clear();
        throw;
    }
}

std::shared_ptr<Tx> Vault::insertNewTx(const Coin::Transaction& cointx, std::shared_ptr<BlockHeader> blockheader, bool verifysigs, bool isCoinbase)
{
    std::stringstream ss;
    ss << "Vault::insertNewTx(" << cointx.hash().getHex() << ", ";
    if (blockheader)    { ss << uchar_vector(blockheader->hash()).getHex(); }
    else                { ss << "null"; }
    ss << ", " << (verifysigs ? "true" : "false") << ")";
    LOGGER(trace) << ss.str() << std::endl;

    std::shared_ptr<Tx> tx;
    {
        boost::lock_guard<boost::mutex> lock(mutex);
        odb::core::session s;
        odb::core::transaction t(db_->begin());
        tx = insertNewTx_unwrapped(cointx, blockheader, verifysigs, isCoinbase);
        t.commit();
    }

    signalQueue.flush();
    return tx;
}

std::shared_ptr<Tx> Vault::insertNewTx_unwrapped(const Coin::Transaction& cointx, std::shared_ptr<BlockHeader> blockheader, bool verifysigs, bool isCoinbase)
{
//LOGGER(trace) << "Vault::insertNewTx_unwrapped: entered." << std::endl;
    try
    {
        using namespace CoinQ::Script;

        std::shared_ptr<Tx> tx(new Tx());
        tx->set(cointx, blockheader ? blockheader->timestamp() : time(NULL), Tx::PROPAGATED);

        // If we already have it but it is unsent update to propagated and update confirmations.
//LOGGER(trace) << "Vault::insertNewTx_unwrapped: If we already have it but it is unsent update to propagated and update confirmations." << std::endl;
        odb::result<Tx> r(db_->query<Tx>(odb::query<Tx>::hash == tx->hash() || odb::query<Tx>::unsigned_hash == tx->unsigned_hash()));
        if (!r.empty())
        {
            std::shared_ptr<Tx> stored_tx(r.begin().load());
            if (verifysigs)
            {
                std::vector<uint64_t> outpointvalues;
                for (auto& txin: stored_tx->txins())
                {
                    outpointvalues.push_back(txin->outpoint() ? txin->outpoint()->value() : 0);
                    Signer signer(cointx, outpointvalues);
                    if (signer.sigsneeded()) throw TxNotSignedException(cointx.hash());
                }
            }

            if (stored_tx->status() < Tx::CONFIRMED)
            {
                LOGGER(debug) << "Vault::insertNewTx_unwrapped - ADDING/REPLACING SIGNATURES, UPDATING CONFIRMATIONS AND STATUS. hash: " << uchar_vector(tx->hash()).getHex() << std::endl;

                // Sanity check - the following condition should never be true, but using out-of-bounds indices will crash the program
                if (stored_tx->txins().size() != cointx.inputs.size())
                    throw std::runtime_error("Transaction input mismatch.");

                // Replace stored tx signatures since this transaction is signed
                txins_t::size_type i = 0;
                txins_t txins = tx->txins();
                for (auto& txin: stored_tx->txins())
                {
                    txin->script(txins[i++]->script());
                    db_->update(txin);
                }

                stored_tx->updateStatus(tx->status());
                stored_tx->blockheader(blockheader);
                db_->update(stored_tx);
                signalQueue.push(notifyTxUpdated.bind(stored_tx));
                return stored_tx; 
            }
            return nullptr;
        }

//LOGGER(trace) << "Vault::insertNewTx_unwrapped: tx->blockheader(blockheader)" << std::endl;
//LOGGER(trace) << "Vault::insertNewTx_unwrapped: tx: " << (tx ? "Not null" : "Null") << std::endl; 
        tx->blockheader(blockheader);
//LOGGER(trace) << "Vault::insertNewTx_unwrapped: tx->blockheader(blockheader) returned" << std::endl;

        std::set<std::shared_ptr<SigningScript>>    updated_scripts;
        std::set<std::shared_ptr<TxIn>>             updated_txins;
        std::set<std::shared_ptr<TxOut>>            updated_txouts;
        std::set<std::shared_ptr<Tx>>               updated_txs;

        std::shared_ptr<Account> sending_account;

//LOGGER(trace) << "Vault::insertNewTx_unwrapped: isCoinbase: " << (isCoinbase ? "TRUE" : "FALSE") << std::endl;
        if (!isCoinbase)
        {
            for (auto& txin: tx->txins())
            {
//LOGGER(trace) << "Vault::insertNewTx_unwrapped: Checking txin" << std::endl;
                bytes_t unsigned_script;
                try
                {
//LOGGER(trace) << "Vault::insertNewTx_unwrapped: txin->unsigned_script()" << std::endl;
                    unsigned_script = txin->unsigned_script();
//LOGGER(trace) << "Vault::insertNewTx_unwrapped: txin->unsigned_script() returned" << std::endl;
                }
                catch (const std::exception& e)
                {
                    LOGGER(error) << "Vault::insertNewTx_unwrapped() - unrecognized input script type: " << e.what() << std::endl;
                    signalQueue.push(notifyTxInsertionError.bind(tx, "Unrecognized input script type."));
                    continue;
                }

                odb::result<SigningScript> r(db_->query<SigningScript>(odb::query<SigningScript>::txinscript == unsigned_script));
                if (!r.empty())
                {
                    // TODO: support sending from multiple accounts in one transaction
                    std::shared_ptr<SigningScript> signingscript(r.begin().load());
                    signingscript->markUsed();
                    updated_scripts.insert(signingscript);

                    sending_account = signingscript->account();

                    // Search for outpoint it spends
//LOGGER(trace) << "Vault::insertNewTx_unwrapped: Search for outpoint it spends" << std::endl;
                    odb::result<TxOut> txout_r(db_->query<TxOut>(odb::query<TxOut>::tx->hash == txin->outhash() && odb::query<TxOut>::txindex == txin->outindex()));
                    if (!txout_r.empty())
                    {
                        std::shared_ptr<TxOut> txout(txout_r.begin().load());
                        // if (txout->script() != signingscript->txoutscript()) throw TxInvalidOutpointException();
                        txin->outpoint(txout);

                        txout->spent(txin);
                        updated_txouts.insert(txout);
                    }
                }
            }
        }

        bool receive = false;
        for (auto& txout: tx->txouts())
        {
//LOGGER(trace) << "Vault::insertNewTx_unwrapped: Checking txout" << std::endl;
            txout->sending_account(sending_account);

            odb::result<SigningScript> r(db_->query<SigningScript>(odb::query<SigningScript>::txoutscript == txout->script()));
            if (!r.empty())
            {
                receive = true;

                std::shared_ptr<SigningScript> signingscript(r.begin().load());
                signingscript->markUsed();
                updated_scripts.insert(signingscript);

                txout->signingscript(signingscript);

                // Search for an input that claims it (to support out-of-order insertion)
//LOGGER(trace) << "Vault::insertNewTx_unwrapped: Search for an input that claims it" << std::endl;
                odb::result<TxIn> txin_r(db_->query<TxIn>(odb::query<TxIn>::outhash == tx->hash() && odb::query<TxIn>::outindex == txout->txindex()));
                if (!txin_r.empty())
                {
                    std::shared_ptr<TxIn> txin(txin_r.begin().load());
                    txout->spent(txin);

                    txin->outpoint(txout);
                    updated_txins.insert(txin);
                    updated_txs.insert(txin->tx());
                }
            }
        }

        if (sending_account || receive)
        {
            // TODO: better tx status update method
//LOGGER(trace) << "Vault::insertNewTx_unwrapped: update tx" << std::endl;
            if (!sending_account) { tx->status(Tx::PROPAGATED); tx->hash(tx->toCoinCore().hash()); }
            for (auto& script:  updated_scripts)
            {
                db_->update(script);
                refillAccountBinPool_unwrapped(script->account_bin());
            }

            tx->updateTotals(); db_->persist(tx);
            for (auto& txin:    tx->txins())            { db_->persist(txin);                   }
            for (auto& txout:   tx->txouts())           { db_->persist(txout);                  }

            for (auto& txin:    updated_txins)          { db_->update(txin);                    }
            for (auto& txout:   updated_txouts)         { db_->update(txout);                   }
            for (auto& tx:      updated_txs)            { tx->updateTotals(); db_->update(tx);  }

            signalQueue.push(notifyTxInserted.bind(tx));
            return tx;
        }

        return nullptr;
    }
    catch (...)
    {
        signalQueue.clear();
        throw;
    }
}

std::shared_ptr<Tx> Vault::insertMerkleTx(const ChainMerkleBlock& chainmerkleblock, const Coin::Transaction& cointx, unsigned int txindex, unsigned int txcount, bool verifysigs, bool isCoinbase)
{
    LOGGER(trace) << "Vault::insertMerkleTx(" << chainmerkleblock.hash().getHex() << ", " << cointx.hash().getHex() << ", " << txindex << ", " << txcount << ", " << (verifysigs ? "true" : "false") << ")" << std::endl;

    std::shared_ptr<Tx> tx;
    {
        boost::lock_guard<boost::mutex> lock(mutex);
        odb::core::session s;
        odb::core::transaction t(db_->begin());
        tx = insertMerkleTx_unwrapped(chainmerkleblock, cointx, txindex, txcount, verifysigs, isCoinbase);
        t.commit();
    }

    signalQueue.flush();
    return tx;
}

std::shared_ptr<Tx> Vault::insertMerkleTx_unwrapped(const ChainMerkleBlock& chainmerkleblock, const Coin::Transaction& cointx, unsigned int txindex, unsigned int txcount, bool verifysigs, bool isCoinbase)
{
    try
    {
        bytes_t blockhash = chainmerkleblock.hash();
        bytes_t txhash = cointx.hash();

        // Instantiate merkleblock
//LOGGER(trace) << "Vault::insertMerkleTx_unwrapped: Instantiate merkleblock" << std::endl;
        std::shared_ptr<MerkleBlock> merkleblock;
        {
            odb::result<MerkleBlock> r(db_->query<MerkleBlock>(odb::query<MerkleBlock>::blockheader.is_not_null() && odb::query<MerkleBlock>::blockheader->hash == blockhash));
            if (!r.empty())
            {
                merkleblock = r.begin().load();
            }
            else
            {
                // Connect to chain
//LOGGER(trace) << "Vault::insertMerkleTx_unwrapped: Connect to chain" << std::endl;
                if (txindex != 0) throw MerkleTxBadInsertionOrderException(blockhash, chainmerkleblock.height, txhash, txindex, txcount);

                odb::result<MerkleBlock> r(db_->query<MerkleBlock>(odb::query<MerkleBlock>::blockheader->hash == chainmerkleblock.prevBlockHash()));
                if (r.empty())
                {
                    odb::result<BlockCountView> r(db_->query<BlockCountView>());
                    if (!r.empty() && r.begin()->count > 0) throw MerkleTxFailedToConnectException(blockhash, chainmerkleblock.height, txhash, txindex, txcount);
                }
                else
                {
                    if ((unsigned int)chainmerkleblock.height != r.begin().load()->blockheader()->height() + 1)
                        throw MerkleTxInvalidHeightException(blockhash, chainmerkleblock.height, txhash, txindex, txcount);
                }

                {
                    // Unconfirm any transactions with equal or larger height
//LOGGER(trace) << "Vault::insertMerkleTx_unwrapped: Unconfirm any transactions with equal or larger height" << std::endl;
                    odb::result<Tx> r(db_->query<Tx>(odb::query<Tx>::blockheader->height >= (unsigned int)chainmerkleblock.height));
                    for (odb::result<Tx>::iterator it = r.begin(); it != r.end(); ++it)
                    {
                        std::shared_ptr<Tx> tx(it.load());
                        tx->blockheader(nullptr);
                        db_->update(tx);
                        signalQueue.push(notifyTxUpdated.bind(tx));
                    }
                }

                {
                    // Delete any merkleblocks with equal or larger height
//LOGGER(trace) << "Vault::insertMerkleTx_unwrapped: Delete any merkleblocks with equal or larger height" << std::endl;
                    odb::result<MerkleBlock> r(db_->query<MerkleBlock>(odb::query<MerkleBlock>::blockheader->height >= (unsigned int)chainmerkleblock.height));
                    for (auto& merkleblock: r) { db_->erase(merkleblock); }
                }

                {
                    // Delete any blockheaders with equal or larger height
//LOGGER(trace) << "Vault::insertMerkleTx_unwrapped: Delete any blockheaders with equal or larger height" << std::endl;
                    odb::result<BlockHeader> r(db_->query<BlockHeader>(odb::query<BlockHeader>::height >= (unsigned int)chainmerkleblock.height));
                    for (auto& blockheader: r) { db_->erase(blockheader); }
                }

                // TODO: test and use the following instead of the above three code blocks
                //deleteMerkleBlock_unwrapped((uint32_t)chainmerkleblock.height);

                // Instantiate the new merkle block and store
//LOGGER(trace) << "Vault::insertMerkleTx_unwrapped: Instantiate the new merkle block and store" << std::endl;
                merkleblock = std::make_shared<MerkleBlock>(chainmerkleblock);
                db_->persist(merkleblock->blockheader());
                db_->persist(merkleblock);
            }
        }

        // If we already have the transaction, just update it.
//LOGGER(trace) << "Vault::insertMerkleTx_unwrapped: If we already have the transaction, just update it." << std::endl;
        std::shared_ptr<Tx> tx;
        {
            odb::result<Tx> r(db_->query<Tx>(odb::query<Tx>::hash == txhash));
            if (!r.empty())
            {
                tx = r.begin().load();
                tx->blockheader(merkleblock->blockheader());
                tx->status(Tx::CONFIRMED);
                tx->conflicting(false);
                db_->update(tx);
                signalQueue.push(notifyTxUpdated.bind(tx));
            }
            else
            {
                Coin::Transaction cointxcopy(cointx);
                cointxcopy.clearScriptSigs();
                bytes_t txunsignedhash = cointxcopy.hash();
                odb::result<Tx> r(db_->query<Tx>(odb::query<Tx>::unsigned_hash == txunsignedhash));
                if (!r.empty())
                {
                    // We have an unsigned version of the transaction
                    LOGGER(debug) << "Vault::insertMerkleTx_unwrapped - ADDING/REPLACING SIGNATURES, UPDATING CONFIRMATIONS AND STATUS. hash: " << uchar_vector(txhash).getHex() << " unsigned hash: " << uchar_vector(txunsignedhash).getHex() << std::endl;

                    tx = r.begin().load();

                    // Sanity check - the following condition should never be true, but using out-of-bounds indices will crash the program
                    if (tx->txins().size() != cointx.inputs.size())
                        throw MerkleTxMismatchException(blockhash, chainmerkleblock.height, txhash, txindex, txcount);

                    // Replace stored tx inputs with new ones since this transaction is signed
                    txins_t::size_type i = 0;
                    for (auto& txin: tx->txins())
                    {
                        txin->fromCoinCore(cointx.inputs[i++]);
                        db_->update(txin);
                    }

                    // Another sanity check - compare hashes
                    if (tx->toCoinCore().hash() != txhash)
                        throw MerkleTxMismatchException(blockhash, chainmerkleblock.height, txhash, txindex, txcount);

                    tx->blockheader(merkleblock->blockheader());
                    tx->timestamp(merkleblock->blockheader()->timestamp());
                    tx->hash(txhash);
                    tx->status(Tx::CONFIRMED);
                    tx->conflicting(false);
                    db_->update(tx);
                    signalQueue.push(notifyTxUpdated.bind(tx));
                }
            } 
        }

        // We've never seen this transaction before - treat it as a new transaction
//LOGGER(trace) << "Vault::insertMerkleTx_unwrapped: We've never seen this transaction before - treat it as a new transaction." << std::endl;
        if (!tx)
        {
            try
            {
//LOGGER(trace) << "Vault::insertMerkleTx_unrapped: calling insertNewTx_unwrapped" << std::endl;
                tx = insertNewTx_unwrapped(cointx, merkleblock->blockheader(), verifysigs, isCoinbase);
                if (tx)
                {
                    tx->status(Tx::CONFIRMED);
                    db_->update(tx);
                    signalQueue.push(notifyTxUpdated.bind(tx));
                }
//LOGGER(trace) << "Vault::insertMerkleTx_unrapped: returned from insertNewTx_unwrapped" << std::endl;
            }
            catch (const std::runtime_error& e)
            {
                LOGGER(error) << "insertNewTx_unwrapped() threw exception: " << e.what() << std::endl;
                signalQueue.push(notifyMerkleBlockInsertionError.bind(merkleblock, e.what()));
            }
        }

        if (txindex + 1 == txcount)
        {
            merkleblock->txsinserted(true);
            db_->update(merkleblock);
            signalQueue.push(notifyMerkleBlockInserted.bind(merkleblock));
        }

        return tx;
    }
    catch (...)
    {
        signalQueue.clear();
        throw;
    }
}

std::shared_ptr<Tx> Vault::confirmMerkleTx(const ChainMerkleBlock& chainmerkleblock, const bytes_t& txhash, unsigned int txindex, unsigned int txcount)
{
    LOGGER(trace) << "Vault::confirmMerkleTx(" << chainmerkleblock.hash().getHex() << ", " << uchar_vector(txhash).getHex() << ", " << txindex << ", " << txcount << ")" << std::endl;

    std::shared_ptr<Tx> tx;
    {
        boost::lock_guard<boost::mutex> lock(mutex);
        odb::core::session s;
        odb::core::transaction t(db_->begin());
        tx = confirmMerkleTx_unwrapped(chainmerkleblock, txhash, txindex, txcount);
        t.commit();
    }

    signalQueue.flush();
    return tx;
}

std::shared_ptr<Tx> Vault::confirmMerkleTx_unwrapped(const ChainMerkleBlock& chainmerkleblock, const bytes_t& txhash, unsigned int txindex, unsigned int txcount)
{
    try
    {
        bytes_t blockhash = chainmerkleblock.hash();

        // Instantiate merkleblock
        std::shared_ptr<MerkleBlock> merkleblock;
        {
            odb::result<MerkleBlock> r(db_->query<MerkleBlock>(odb::query<MerkleBlock>::blockheader.is_not_null() && odb::query<MerkleBlock>::blockheader->hash == blockhash));
            if (!r.empty())
            {
                merkleblock = r.begin().load();
            }
            else
            {
                // Connect to chain
                if (txindex != 0) throw MerkleTxBadInsertionOrderException(blockhash, chainmerkleblock.height, txhash, txindex, txcount);

                odb::result<MerkleBlock> r(db_->query<MerkleBlock>(odb::query<MerkleBlock>::blockheader->hash == chainmerkleblock.prevBlockHash()));
                if (r.empty())
                {
                    odb::result<BlockCountView> r(db_->query<BlockCountView>());
                    if (!r.empty() && r.begin()->count > 0) throw MerkleTxFailedToConnectException(blockhash, chainmerkleblock.height, txhash, txindex, txcount);
                }
                else
                {
                    if ((unsigned int)chainmerkleblock.height != r.begin().load()->blockheader()->height() + 1)
                        throw MerkleTxInvalidHeightException(blockhash, chainmerkleblock.height, txhash, txindex, txcount);
                }

                {
                    // Unconfirm any transactions with equal or larger height
                    odb::result<Tx> r(db_->query<Tx>(odb::query<Tx>::blockheader->height >= (unsigned int)chainmerkleblock.height));
                    for (odb::result<Tx>::iterator it = r.begin(); it != r.end(); ++it)
                    {
                        std::shared_ptr<Tx> tx(it.load());
                        tx->status(Tx::PROPAGATED);
                        db_->update(tx);
                        signalQueue.push(notifyTxUpdated.bind(tx));
                    }
                }


                {
                    // Delete any merkleblocks with equal or larger height
                    odb::result<MerkleBlock> r(db_->query<MerkleBlock>(odb::query<MerkleBlock>::blockheader->height >= (unsigned int)chainmerkleblock.height));
                    for (auto& merkleblock: r) { db_->erase(merkleblock); }
                }

                {
                    // Delete any blockheaders with equal or larger height
                    odb::result<BlockHeader> r(db_->query<BlockHeader>(odb::query<BlockHeader>::height >= (unsigned int)chainmerkleblock.height));
                    for (auto& blockheader: r) { db_->erase(blockheader); }
                }

                // TODO: test and use the following instead of the above three code blocks
                //deleteMerkleBlock_unwrapped((uint32_t)chainmerkleblock.height);

                // Instantiate the new merkle block and store
                merkleblock = std::make_shared<MerkleBlock>(chainmerkleblock);
                db_->persist(merkleblock->blockheader());
                db_->persist(merkleblock);
            }
        }

        std::shared_ptr<Tx> tx;
        odb::result<Tx> tx_r(db_->query<Tx>(odb::query<Tx>::hash == txhash));
        if (!tx_r.empty())
        {
            tx = tx_r.begin().load();
            tx->blockheader(merkleblock->blockheader());
            tx->status(Tx::CONFIRMED);
            tx->conflicting(false);
            db_->update(tx);
            signalQueue.push(notifyTxUpdated.bind(tx));
        }

        if (txindex + 1 == txcount)
        {
            merkleblock->txsinserted(true);
            db_->update(merkleblock);
            signalQueue.push(notifyMerkleBlockInserted.bind(merkleblock));
        }

        return tx;
    }
    catch (...)
    {
        signalQueue.clear();
        throw;
    }
}

std::shared_ptr<Tx> Vault::createTx(const std::string& account_name, uint32_t tx_version, uint32_t tx_locktime, txouts_t txouts, uint64_t fee, unsigned int maxchangeouts, bool insert)
{
    LOGGER(trace) << "Vault::createTx(" << account_name << ", " << tx_version << ", " << tx_locktime << ", " << txouts.size() << " txout(s), " << fee << ", " << maxchangeouts << ", " << (insert ? "insert" : "no insert") << ")" << std::endl;

    std::shared_ptr<Tx> tx;
    {
        boost::lock_guard<boost::mutex> lock(mutex);
        odb::core::session s;
        odb::core::transaction t(db_->begin());
        tx = createTx_unwrapped(account_name, tx_version, tx_locktime, txouts, fee, maxchangeouts);
        if (insert)
        {
            tx = insertTx_unwrapped(tx);
            if (tx) t.commit();
        }
    }

    signalQueue.flush();
    return tx;
}

std::shared_ptr<Tx> Vault::createTx_unwrapped(const std::string& account_name, uint32_t tx_version, uint32_t tx_locktime, txouts_t txouts, uint64_t fee, unsigned int /*maxchangeouts*/)
{
    // TODO: Better rng seeding
    std::srand(std::time(0));

    // TODO: Better fee calculation heuristics
    uint64_t desired_total = fee;
    for (auto& txout: txouts)
    {
        if (txout->value() == 0) throw TxInvalidOutputsException();
        desired_total += txout->value();
    }

    std::shared_ptr<Account> account = getAccount_unwrapped(account_name);

    // TODO: Better coin selection
    typedef odb::query<TxOutView> query_t;
    odb::result<TxOutView> utxoview_r(db_->query<TxOutView>(query_t::Tx::status > Tx::UNSIGNED && query_t::TxOut::status == TxOut::UNSPENT && query_t::receiving_account::id == account->id()));
    std::vector<TxOutView> utxoviews;
    for (auto& utxoview: utxoview_r) { utxoviews.push_back(utxoview); }
   
    std::random_shuffle(utxoviews.begin(), utxoviews.end(), [](int i) { return std::rand() % i; });

    txins_t txins;
    int i = 0;
    uint64_t total = 0;
    for (auto& utxoview: utxoviews)
    {
        total += utxoview.value;
        //std::shared_ptr<TxIn> txin(new TxIn(utxoview.tx_hash, utxoview.tx_index, utxoview.signingscript_txinscript, 0xffffffff));
        std::shared_ptr<TxIn> txin(new TxIn(utxoview.tx_hash, utxoview.tx_index, utxoview.signingscript_txinscript, 0));
        if (account->use_witness())
        {
            using namespace CoinQ::Script;
            scriptstack_t stack;
            for (std::size_t k = 0; k <= account->keychains().size(); k++) { stack.push_back(bytes_t()); }
            stack.push_back(utxoview.signingscript_redeemscript); 
            txin->scriptwitnessstack(stack);
        }
        txins.push_back(txin);
        i++;
        if (total >= desired_total) break;
    }
    if (total < desired_total) throw AccountInsufficientFundsException(account_name, desired_total, total); 

    utxoviews.resize(i);
    uint64_t change = total - desired_total;

    if (change > 0)
    {
        std::shared_ptr<AccountBin> bin = getAccountBin_unwrapped(account_name, CHANGE_BIN_NAME);
        std::shared_ptr<SigningScript> changescript = issueAccountBinSigningScript_unwrapped(bin);

        // TODO: Allow adding multiple change outputs
        std::shared_ptr<TxOut> txout(new TxOut(change, changescript));
        txouts.push_back(txout);
    }
    std::random_shuffle(txouts.begin(), txouts.end(), [](int i) { return std::rand() % i; });

    std::shared_ptr<Tx> tx(new Tx());
    tx->set(tx_version, txins, txouts, tx_locktime, time(NULL), Tx::UNSIGNED);
    return tx;
}

std::shared_ptr<Tx> Vault::createTx(const std::string& username, const std::string& account_name, uint32_t tx_version, uint32_t tx_locktime, txouts_t txouts, uint64_t fee, unsigned int maxchangeouts, bool insert)
{
    LOGGER(trace) << "Vault::createTx(" << username << ", " << account_name << ", " << tx_version << ", " << tx_locktime << ", " << txouts.size() << " txout(s), " << fee << ", " << maxchangeouts << ", " << (insert ? "insert" : "no insert") << ")" << std::endl;

    std::shared_ptr<Tx> tx;
    {
        boost::lock_guard<boost::mutex> lock(mutex);
        odb::core::session s;
        odb::core::transaction t(db_->begin());
        tx = createTx_unwrapped(username, account_name, tx_version, tx_locktime, txouts, fee, maxchangeouts);
        if (insert)
        {
            tx = insertTx_unwrapped(tx);
            if (tx) t.commit();
        }
    }

    signalQueue.flush();
    return tx;
}

std::shared_ptr<Tx> Vault::createTx_unwrapped(const std::string& username, const std::string& account_name, uint32_t tx_version, uint32_t tx_locktime, txouts_t txouts, uint64_t fee, unsigned int maxchangeouts)
{
    std::shared_ptr<User> user = getUser_unwrapped(username);

    if (user->isTxOutScriptWhitelistEnabled())
    {
        for (auto& txout: txouts)
        {
            if (!user->txoutscript_whitelist().count(txout->script())) throw TxOutputScriptNotInUserWhitelistException(username, txout->script());        
        }
    }

    std::shared_ptr<Tx> tx;
    try
    {
        tx = createTx_unwrapped(account_name, tx_version, tx_locktime, txouts, fee, maxchangeouts);
    }
    catch (AccountInsufficientFundsException& e)
    {
        e.username(username);
        throw e;
    }

    tx->user(user);
    return tx;
}

std::shared_ptr<Tx> Vault::createTx(const std::string& account_name, uint32_t tx_version, uint32_t tx_locktime, ids_t coin_ids, txouts_t txouts, uint64_t fee, uint32_t min_confirmations, bool insert)
{
    LOGGER(trace) << "Vault::createTx(" << account_name << ", " << tx_version << ", " << tx_locktime << ", " << coin_ids.size() << " txin(s), " << txouts.size() << " txout(s), " << fee << ", " << min_confirmations << ", " << (insert ? "insert" : "no insert") << ")" << std::endl;

    std::shared_ptr<Tx> tx;
    {
        boost::lock_guard<boost::mutex> lock(mutex);
        odb::core::session s;
        odb::core::transaction t(db_->begin());
        tx = createTx_unwrapped(account_name, tx_version, tx_locktime, coin_ids, txouts, fee, min_confirmations);
        if (insert)
        {
            tx = insertTx_unwrapped(tx);
            if (tx) t.commit();
        }
    }

    signalQueue.flush();
    return tx;
}

std::shared_ptr<Tx> Vault::createTx_unwrapped(const std::string& account_name, uint32_t tx_version, uint32_t tx_locktime, ids_t coin_ids, txouts_t txouts, uint64_t fee, uint32_t min_confirmations)
{
    std::shared_ptr<Account> account = getAccount_unwrapped(account_name);

    // TODO: Better fee calculation heuristics
    uint64_t input_total = 0;
    uint64_t desired_total = fee;
    for (auto& txout: txouts)
    {
        desired_total += txout->value();
        if (txout->value() == 0) throw TxInvalidOutputsException();
    }

    typedef odb::query<TxOutView> query_t;
    query_t base_query(query_t::Tx::status > Tx::UNSIGNED && query_t::TxOut::status == TxOut::UNSPENT && query_t::receiving_account::id == account->id());

    if (min_confirmations > 0)
    {
        uint32_t best_height = getBestHeight_unwrapped();
        if (min_confirmations > best_height) throw AccountInsufficientFundsException(account_name, desired_total, 0);
        base_query = (base_query && query_t::BlockHeader::height <= best_height + 1 - min_confirmations);
    }

    txins_t txins;
    if (!coin_ids.empty())
    {
        odb::result<TxOutView> utxoview_r(db_->query<TxOutView>(base_query && query_t::TxOut::id.in_range(coin_ids.begin(), coin_ids.end())));
        for (auto& utxoview: utxoview_r)
        {
            //std::shared_ptr<TxIn> txin(new TxIn(utxoview.tx_hash, utxoview.tx_index, utxoview.signingscript_txinscript, 0xffffffff));
            std::shared_ptr<TxIn> txin(new TxIn(utxoview.tx_hash, utxoview.tx_index, utxoview.signingscript_txinscript, 0));
            if (account->use_witness())
            {
                using namespace CoinQ::Script;
                scriptstack_t stack;
                for (std::size_t k = 0; k <= account->keychains().size(); k++) { stack.push_back(bytes_t()); }
                stack.push_back(utxoview.signingscript_redeemscript); 
                txin->scriptwitnessstack(stack);
            }
            txins.push_back(txin);
            input_total += utxoview.value;
        }
        if (txins.size() < coin_ids.size()) throw TxInvalidInputsException();
    }

    // If the supplied inputs are insufficient, automatically add more
    // TODO; Better coin selection heuristics
    if (input_total < desired_total)
    {
        query_t query(base_query);
        if (!coin_ids.empty()) { query = (query && !query_t::TxOut::id.in_range(coin_ids.begin(), coin_ids.end())); }
        odb::result<TxOutView> utxoview_r(db_->query<TxOutView>(query));
        std::vector<TxOutView> utxoviews;
        for (auto& utxoview: utxoview_r) { utxoviews.push_back(utxoview); }
        std::random_shuffle(utxoviews.begin(), utxoviews.end(), [](int i) { return std::rand() % i; });

        for (auto& utxoview: utxoviews)
        {
            //std::shared_ptr<TxIn> txin(new TxIn(utxoview.tx_hash, utxoview.tx_index, utxoview.signingscript_txinscript, 0xffffffff));
            std::shared_ptr<TxIn> txin(new TxIn(utxoview.tx_hash, utxoview.tx_index, utxoview.signingscript_txinscript, 0));
            if (account->use_witness())
            {
                using namespace CoinQ::Script;
                scriptstack_t stack;
                for (std::size_t k = 0; k <= account->keychains().size(); k++) { stack.push_back(bytes_t()); }
                stack.push_back(utxoview.signingscript_redeemscript); 
                txin->scriptwitnessstack(stack);
            }
            txins.push_back(txin);
            input_total += utxoview.value;
            if (input_total >= desired_total) break;
        }
        if (input_total < desired_total) throw AccountInsufficientFundsException(account_name, desired_total, input_total);
    }
 
    // Use supplied outputs first
    std::shared_ptr<AccountBin> change_bin;
    for (auto& txout: txouts)
    {
        if (txout->script().empty())
        {
            if (!change_bin) { change_bin = getAccountBin_unwrapped(account_name, CHANGE_BIN_NAME); }
            std::shared_ptr<SigningScript> changescript = issueAccountBinSigningScript_unwrapped(change_bin);
            txout->signingscript(changescript);
        }
    }

    // If supplied change amounts are insufficient, add another change output
    uint64_t change = input_total - desired_total;
    if (change > 0)
    {
        if (!change_bin) { change_bin = getAccountBin_unwrapped(account_name, CHANGE_BIN_NAME); }
        std::shared_ptr<SigningScript> changescript = issueAccountBinSigningScript_unwrapped(change_bin);

        std::shared_ptr<TxOut> txout(new TxOut(change, changescript));
        txouts.push_back(txout);
    }
    // TODO: Better rng seeding
    std::srand(std::time(0));
    std::random_shuffle(txins.begin(), txins.end(), [](int i) { return std::rand() % i; });
    std::random_shuffle(txouts.begin(), txouts.end(), [](int i) { return std::rand() % i; });

    std::shared_ptr<Tx> tx(new Tx());
    tx->set(tx_version, txins, txouts, tx_locktime, time(NULL), Tx::UNSIGNED);
    return tx;
}

std::shared_ptr<Tx> Vault::createTx(const std::string& username, const std::string& account_name, uint32_t tx_version, uint32_t tx_locktime, ids_t coin_ids, txouts_t txouts, uint64_t fee, uint32_t min_confirmations, bool insert)
{
    LOGGER(trace) << "Vault::createTx(" << username << ", " << account_name << ", " << tx_version << ", " << tx_locktime << ", " << coin_ids.size() << " txin(s), " << txouts.size() << " txout(s), " << fee << ", " << min_confirmations << ", " << (insert ? "insert" : "no insert") << ")" << std::endl;

    std::shared_ptr<Tx> tx;
    {
        boost::lock_guard<boost::mutex> lock(mutex);
        odb::core::session s;
        odb::core::transaction t(db_->begin());
        tx = createTx_unwrapped(username, account_name, tx_version, tx_locktime, coin_ids, txouts, fee, min_confirmations);
        if (insert)
        {
            tx = insertTx_unwrapped(tx);
            if (tx) t.commit();
        }
    }

    signalQueue.flush();
    return tx;
}

std::shared_ptr<Tx> Vault::createTx_unwrapped(const std::string& username, const std::string& account_name, uint32_t tx_version, uint32_t tx_locktime, ids_t coin_ids, txouts_t txouts, uint64_t fee, uint32_t min_confirmations)
{
    std::shared_ptr<User> user = getUser_unwrapped(username);

    if (user->isTxOutScriptWhitelistEnabled())
    {
        for (auto& txout: txouts)
        {
            if (!user->txoutscript_whitelist().count(txout->script())) throw TxOutputScriptNotInUserWhitelistException(username, txout->script());        
        }
    }

    std::shared_ptr<Tx> tx = createTx_unwrapped(account_name, tx_version, tx_locktime, coin_ids, txouts, fee, min_confirmations);
    tx->user(user);
    return tx;
}

txs_t Vault::consolidateTxOuts(const std::string& account_name, uint32_t max_tx_size, uint32_t tx_version, uint32_t tx_locktime, ids_t coin_ids, const bytes_t& txoutscript, uint64_t min_fee, uint32_t min_confirmations, bool insert)
{
    LOGGER(trace) << "Vault::consolidateTxOuts(" << account_name << ", " << max_tx_size << ", " << tx_version << ", " << tx_locktime << ", " << coin_ids.size() << " txin(s), " << uchar_vector(txoutscript).getHex() << ", " << min_fee << ", " << min_confirmations << ", " << (insert ? "insert" : "no insert") << ")" << std::endl;

    txs_t txs;
    {
        boost::lock_guard<boost::mutex> lock(mutex);
        odb::core::session s;
        odb::core::transaction t(db_->begin());
        txs = consolidateTxOuts_unwrapped(account_name, max_tx_size, tx_version, tx_locktime, coin_ids, txoutscript, min_fee, min_confirmations);
        if (insert)
        {
            bool bInserted = false;
            for (auto& tx: txs)
            {
                tx = insertTx_unwrapped(tx);
                if (tx) { bInserted = true; }
            }
            if (bInserted) t.commit();
        }
    }

    signalQueue.flush();
    return txs;
}

txs_t Vault::consolidateTxOuts(const std::string& username, const std::string& account_name, uint32_t max_tx_size, uint32_t tx_version, uint32_t tx_locktime, ids_t coin_ids, const bytes_t& txoutscript, uint64_t min_fee, uint32_t min_confirmations, bool insert)
{
    LOGGER(trace) << "Vault::consolidateTxOuts(" << username << ", " << account_name << ", " << max_tx_size << ", " << tx_version << ", " << tx_locktime << ", " << coin_ids.size() << " txin(s), " << uchar_vector(txoutscript).getHex() << ", " << min_fee << ", " << min_confirmations << ", " << (insert ? "insert" : "no insert") << ")" << std::endl;

    std::shared_ptr<User> user = getUser_unwrapped(username);

    txs_t txs;
    {
        boost::lock_guard<boost::mutex> lock(mutex);
        odb::core::session s;
        odb::core::transaction t(db_->begin());
        txs = consolidateTxOuts_unwrapped(account_name, max_tx_size, tx_version, tx_locktime, coin_ids, txoutscript, min_fee, min_confirmations);
        bool bInserted = false;
        for (auto& tx: txs)
        {
            tx->user(user); 
            if (insert)
            {
                tx = insertTx_unwrapped(tx);
                if (tx) { bInserted = true; }
            }
        }
        if (bInserted) t.commit();
    }

    signalQueue.flush();
    return txs;
}

txs_t Vault::consolidateTxOuts_unwrapped(const std::string& account_name, uint32_t max_tx_size, uint32_t tx_version, uint32_t tx_locktime, ids_t coin_ids, const bytes_t& txoutscript, uint64_t min_fee, uint32_t min_confirmations)
{
    std::shared_ptr<Account> account = getAccount_unwrapped(account_name);

    typedef odb::query<TxOutView> query_t;
    query_t base_query(query_t::Tx::status > Tx::UNSIGNED && query_t::TxOut::status == TxOut::UNSPENT && query_t::receiving_account::id == account->id());

    if (min_confirmations > 0)
    {
        uint32_t best_height = getBestHeight_unwrapped();
        if (min_confirmations > best_height) throw AccountInsufficientFundsException(account_name, 0, 0);
        base_query = (base_query && query_t::BlockHeader::height <= best_height + 1 - min_confirmations);
    }

    odb::result<TxOutView> utxoview_r;
    if (coin_ids.empty())
    {
        utxoview_r = db_->query<TxOutView>(base_query);
    }
    else
    {
        utxoview_r = db_->query<TxOutView>(base_query && query_t::TxOut::id.in_range(coin_ids.begin(), coin_ids.end()));
    }

    std::vector<TxOutView> utxoviews;
    for (auto& utxoview: utxoview_r) { utxoviews.push_back(utxoview); }
    if (utxoviews.size() < coin_ids.size()) throw TxInvalidInputsException();

    // TODO: Better rng seeding
    std::srand(std::time(0));
    std::random_shuffle(utxoviews.begin(), utxoviews.end(), [](int i) { return std::rand() % i; });

    txins_t txins;
    uint64_t input_total = 0;

    std::shared_ptr<Tx> test_tx = std::make_shared<Tx>();
    txs_t txs;
    std::shared_ptr<TxOut> txout = std::make_shared<TxOut>(0, txoutscript);
    txouts_t txouts;
    txouts.push_back(txout);
    for (auto& utxoview: utxoviews)
    {
        //std::shared_ptr<TxIn> txin(new TxIn(utxoview.tx_hash, utxoview.tx_index, utxoview.signingscript_txinscript, 0xffffffff));
        std::shared_ptr<TxIn> txin(new TxIn(utxoview.tx_hash, utxoview.tx_index, utxoview.signingscript_txinscript, 0));
        if (account->use_witness())
        {
            using namespace CoinQ::Script;
            scriptstack_t stack;
            for (std::size_t k = 0; k <= account->keychains().size(); k++) { stack.push_back(bytes_t()); }
            stack.push_back(utxoview.signingscript_redeemscript); 
            txin->scriptwitnessstack(stack);
        }
        txins.push_back(txin);
        test_tx->set(tx_version, txins, txouts, tx_locktime, time(NULL), Tx::UNSIGNED);
        if (test_tx->raw().size() > max_tx_size)
        {
            txins.pop_back();
            if (txins.empty()) throw std::runtime_error("Vault::consolidateTxOuts_unwrapped() - txins empty.");
            if (input_total <= min_fee) throw std::runtime_error("Vault::consolidateTxOuts_unwrapped() - input total is not greater than fee.");
            txouts_t txouts;
            std::shared_ptr<TxOut> txout = std::make_shared<TxOut>(input_total - min_fee, txoutscript);
            txouts.push_back(txout);
            std::shared_ptr<Tx> tx = std::make_shared<Tx>();
            tx->set(tx_version, txins, txouts, tx_locktime, time(NULL), Tx::UNSIGNED);
            if (tx->raw().size() > max_tx_size) throw std::runtime_error("Vault::consolidateTxOuts_unwrapped() - maximum transaction size is too small.");

            txs.push_back(tx);
            input_total = 0;
            txins.clear();
            txins.push_back(txin);
        }
            
        input_total += utxoview.value;
    }
    
    if (!txins.empty() && input_total >= min_fee)
    {
        txouts_t txouts;
        std::shared_ptr<TxOut> txout = std::make_shared<TxOut>(input_total - min_fee, txoutscript);
        txouts.push_back(txout);
        std::shared_ptr<Tx> tx = std::make_shared<Tx>();
        tx->set(tx_version, txins, txouts, tx_locktime, time(NULL), Tx::UNSIGNED);
        if (tx->raw().size() < max_tx_size) { txs.push_back(tx); }
    }
    
    return txs;
}

void Vault::updateTx_unwrapped(std::shared_ptr<Tx> tx)
{
    for (auto& txin: tx->txins()) { db_->update(txin); }
    for (auto& txout: tx->txouts()) { db_->update(txout); }
    db_->update(tx); 
}

void Vault::deleteTx(const bytes_t& tx_hash)
{
    LOGGER(trace) << "Vault::deleteTx(" << uchar_vector(tx_hash).getHex() << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    odb::result<Tx> r(db_->query<Tx>(odb::query<Tx>::hash == tx_hash || odb::query<Tx>::unsigned_hash == tx_hash));
    if (r.empty()) throw TxNotFoundException(tx_hash);

    std::shared_ptr<Tx> tx(r.begin().load());
    deleteTx_unwrapped(tx);
    t.commit();

    signalQueue.flush();
}

void Vault::deleteTx(unsigned long tx_id)
{
    LOGGER(trace) << "Vault::deleteTx(" << tx_id << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    odb::result<Tx> r(db_->query<Tx>(odb::query<Tx>::id == tx_id));
    if (r.empty()) throw TxNotFoundException();

    std::shared_ptr<Tx> tx(r.begin().load());
    deleteTx_unwrapped(tx);
    t.commit();

    signalQueue.flush();
}

void Vault::deleteTx_unwrapped(std::shared_ptr<Tx> tx)
{
    try
    {
        // NOTE: signingscript statuses are not updated. once received always received.

        // delete txins
        for (auto& txin: tx->txins())
        {
            // unspend spent outpoints first
            odb::result<TxOut> txout_r(db_->query<TxOut>(odb::query<TxOut>::spent == txin->id()));
            if (!txout_r.empty())
            {
                std::shared_ptr<TxOut> txout(txout_r.begin().load());
                txout->spent(nullptr);
                db_->update(txout);
            }
            db_->erase(txin);
        }

        // delete txouts
        for (auto& txout: tx->txouts())
        {
            // recursively delete any transactions that depend on this one first
            if (txout->spent()) { deleteTx_unwrapped(txout->spent()->tx()); }
            db_->erase(txout);
        }

        // delete tx
        db_->erase(tx);
        signalQueue.push(notifyTxDeleted.bind(tx));
    }
    catch (...)
    {
        signalQueue.clear();
        throw;
    }
}

SigningRequest Vault::getSigningRequest(const bytes_t& hash, bool include_raw_tx) const
{
    LOGGER(trace) << "Vault::getSigningRequest(" << uchar_vector(hash).getHex() << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    odb::result<Tx> r(db_->query<Tx>(odb::query<Tx>::hash == hash || odb::query<Tx>::unsigned_hash == hash));
    if (r.empty()) throw TxNotFoundException(hash);

    std::shared_ptr<Tx> tx(r.begin().load());
    return getSigningRequest_unwrapped(tx, include_raw_tx);
}

SigningRequest Vault::getSigningRequest(unsigned long tx_id, bool include_raw_tx) const
{
    LOGGER(trace) << "Vault::getSigningRequest(" << tx_id << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    odb::result<Tx> r(db_->query<Tx>(odb::query<Tx>::id == tx_id));
    if (r.empty()) throw TxNotFoundException();

    std::shared_ptr<Tx> tx(r.begin().load());
    return getSigningRequest_unwrapped(tx, include_raw_tx);
}

SigningRequest Vault::getSigningRequest_unwrapped(std::shared_ptr<Tx> tx, bool include_raw_tx) const
{
    unsigned int sigs_needed = tx->missingSigCount();
    std::set<bytes_t> pubkeys = tx->missingSigPubkeys();
    std::set<SigningRequest::keychain_info_t> keychain_info;
    odb::result<Key> key_r(db_->query<Key>(odb::query<Key>::pubkey.in_range(pubkeys.begin(), pubkeys.end())));
    for (auto& keychain: key_r)
    {
        std::shared_ptr<Keychain> root_keychain(keychain.root_keychain());
        keychain_info.insert(std::make_pair(root_keychain->name(), root_keychain->hash()));
    }

    bytes_t rawtx;
    if (include_raw_tx) rawtx = tx->raw();
    return SigningRequest(tx->hash(), sigs_needed, keychain_info, rawtx);
}

SignatureInfo Vault::getSignatureInfo(const bytes_t& hash) const
{
    LOGGER(trace) << "Vault::getSignatureInfo(" << uchar_vector(hash).getHex() << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    odb::result<Tx> r(db_->query<Tx>(odb::query<Tx>::hash == hash || odb::query<Tx>::unsigned_hash == hash));
    if (r.empty()) throw TxNotFoundException(hash);

    std::shared_ptr<Tx> tx(r.begin().load());
    return getSignatureInfo_unwrapped(tx);
}

SignatureInfo Vault::getSignatureInfo(unsigned long tx_id) const
{
    LOGGER(trace) << "Vault::getSignatureInfo(" << tx_id << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    odb::result<Tx> r(db_->query<Tx>(odb::query<Tx>::id == tx_id));
    if (r.empty()) throw TxNotFoundException();

    std::shared_ptr<Tx> tx(r.begin().load());
    return getSignatureInfo_unwrapped(tx);
}

SignatureInfo Vault::getSignatureInfo_unwrapped(std::shared_ptr<Tx> tx) const
{
    // Assume for now all inputs belong to the same account.
    CoinQ::Script::Signer signer = tx->signer();

    unsigned int sigsNeeded = 0;
    std::set<bytes_t> missingpubkeys;
    std::set<bytes_t> presentpubkeys;
 
    for (auto& signabletxin: signer.getSignableTxIns())
    {
        unsigned int n = signabletxin.sigsneeded();
        if (n > sigsNeeded) sigsNeeded = n;

        {
            std::vector<bytes_t> pubkeys = signabletxin.missingsigs();
            for (auto& pubkey: pubkeys) { missingpubkeys.insert(pubkey); }
        }

        {
            std::vector<bytes_t> pubkeys = signabletxin.presentsigs();
            for (auto& pubkey: pubkeys) { presentpubkeys.insert(pubkey); }
        }
    }

    SigningKeychainSet signingKeychainSet;

    {
        odb::result<Key> key_r(db_->query<Key>(odb::query<Key>::pubkey.in_range(missingpubkeys.begin(), missingpubkeys.end())));
        for (auto& keychain: key_r)
        {
            std::shared_ptr<Keychain> root_keychain(keychain.root_keychain());
            signingKeychainSet.insert(SigningKeychain(root_keychain->name(), root_keychain->hash(), false, root_keychain->isPrivate()));
        }
    }

    {
        odb::result<Key> key_r(db_->query<Key>(odb::query<Key>::pubkey.in_range(presentpubkeys.begin(), presentpubkeys.end())));
        for (auto& keychain: key_r)
        {
            std::shared_ptr<Keychain> root_keychain(keychain.root_keychain());
            signingKeychainSet.insert(SigningKeychain(root_keychain->name(), root_keychain->hash(), true, root_keychain->isPrivate()));
        }
    }

    return SignatureInfo(sigsNeeded, signingKeychainSet);
}

std::shared_ptr<Tx> Vault::signTx(const bytes_t& hash, std::vector<std::string>& keychain_names, bool update)
{
    LOGGER(trace) << "Vault::signTx(" << uchar_vector(hash).getHex() << ", [" << stdutils::delimited_list(keychain_names, ", ") << "], " << (update ? "update" : "no update") << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session s;
    odb::core::transaction t(db_->begin());

    odb::result<Tx> tx_r;
    tx_r = db_->query<Tx>(odb::query<Tx>::hash == hash);
    if (!tx_r.empty())
    {
        std::shared_ptr<Tx> tx(tx_r.begin().load());
        return tx;
    }

    tx_r = db_->query<Tx>(odb::query<Tx>::unsigned_hash == hash);
    if (tx_r.empty()) throw TxNotFoundException(hash);
    std::shared_ptr<Tx> tx(tx_r.begin().load());

    unsigned int sigcount = signTx_unwrapped(tx, keychain_names);
    if (sigcount && update)
    {
        updateTx_unwrapped(tx);
        t.commit();
    }
    return tx;
}

std::shared_ptr<Tx> Vault::signTx(unsigned long tx_id, std::vector<std::string>& keychain_names, bool update)
{
    LOGGER(trace) << "Vault::signTx(" << tx_id << ", [" << stdutils::delimited_list(keychain_names, ", ") << "], " << (update ? "update" : "no update") << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session s;
    odb::core::transaction t(db_->begin());

    odb::result<Tx> tx_r(db_->query<Tx>(odb::query<Tx>::id == tx_id));
    if (tx_r.empty()) throw TxNotFoundException();
    std::shared_ptr<Tx> tx(tx_r.begin().load());

    unsigned int sigcount = signTx_unwrapped(tx, keychain_names);
    if (sigcount && update)
    {
        updateTx_unwrapped(tx);
        t.commit();
    }
    return tx;
}

unsigned int Vault::signTx_unwrapped(std::shared_ptr<Tx> tx, std::vector<std::string>& keychain_names)
{
    using namespace CoinQ::Script;
    using namespace CoinCrypto;

    Coin::Transaction coin_tx = tx->toCoinCore();

    // No point in trying nonprivate keys
    odb::query<Key> privkey_query(odb::query<Key>::is_private != 0);

    // If the keychain name list is not empty, only try keys belonging to the named keychains
    if (!keychain_names.empty())
        privkey_query = privkey_query && odb::query<Key>::root_keychain->name.in_range(keychain_names.begin(), keychain_names.end());

    KeychainSet keychains_signed;

    unsigned int sigsadded = 0;
    for (auto& txin: tx->txins())
    {
        uint64_t outpointvalue = txin->outpoint() ? txin->outpoint()->value() : 0;
        SignableTxIn signableTxIn(coin_tx, txin->txindex(), outpointvalue);

        unsigned int sigsneeded = signableTxIn.sigsneeded();
        if (sigsneeded == 0) continue;

        std::vector<bytes_t> pubkeys = signableTxIn.missingsigs();
        if (pubkeys.empty()) continue;

        odb::result<Key> key_r(db_->query<Key>(privkey_query && odb::query<Key>::pubkey.in_range(pubkeys.begin(), pubkeys.end())));
        if (key_r.empty()) continue;

        // Compute hash to sign
        bytes_t signingHash = coin_tx.getSigHash(SIGHASH_ALL, txin->txindex(), signableTxIn.redeemscript(), outpointvalue);
        LOGGER(debug) << "Vault::signTx_unwrapped - computed signing hash " << uchar_vector(signingHash).getHex() << " for input " << txin->txindex() << std::endl;

        for (auto& key: key_r)
        {
            if (!tryUnlockKeychain_unwrapped(key.root_keychain()))
            {
                LOGGER(debug) << "Vault::signTx_unwrapped - private key locked for keychain " << key.root_keychain()->name() << std::endl;
                continue;
            }

            LOGGER(debug) << "Vault::signTx_unwrapped - SIGNING INPUT " << txin->txindex() << " WITH KEYCHAIN " << key.root_keychain()->name() << std::endl;        
            secure_bytes_t privkey = key.try_privkey();

            // TODO: Better exception handling with secp256kl_key class
            secp256k1_key signingKey;
            signingKey.setPrivKey(privkey);

            // Try checking both compressed and uncompressed pubkeys
            if (signingKey.getPubKey() != key.pubkey() && signingKey.getPubKey(false) != key.pubkey()) throw KeychainInvalidPrivateKeyException(key.root_keychain()->name(), key.pubkey());

            bytes_t signature = secp256k1_sign(signingKey, signingHash);
            signature.push_back(SIGHASH_ALL);
            signableTxIn.addsig(key.pubkey(), signature);
            LOGGER(debug) << "Vault::signTx_unwrapped - PUBLIC KEY: " << uchar_vector(key.pubkey()).getHex() << " SIGNATURE: " << uchar_vector(signature).getHex() << std::endl;
            keychains_signed.insert(key.root_keychain());
            sigsadded++;
            sigsneeded--;
            if (sigsneeded == 0) break;
        }

        txin->script(signableTxIn.txinscript());
        std::vector<bytes_t> stack;
        for (auto& item: signableTxIn.scriptwitness().stack) { stack.push_back(item); }
        txin->scriptwitnessstack(stack);
    }

    keychain_names.clear();
    if (!sigsadded) return 0;

    for (auto& keychain: keychains_signed) { keychain_names.push_back(keychain->name()); }
    tx->updateStatus(Tx::NO_STATUS, true);
    return sigsadded;
}

std::shared_ptr<TxOut> Vault::getTxOut(const bytes_t& outhash, uint32_t outindex) const
{
    LOGGER(trace) << "Vault::getTxOut(" << uchar_vector(outhash).getHex() << ", " << outindex << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    return getTxOut_unwrapped(outhash, outindex);
}

std::shared_ptr<TxOut> Vault::getTxOut_unwrapped(const bytes_t& outhash, uint32_t outindex) const
{
    odb::result<TxOut> r(db_->query<TxOut>(odb::query<TxOut>::tx->hash == outhash && odb::query<TxOut>::txindex == outindex));
    if (r.empty()) throw TxOutputNotFoundException(outhash, (int)outindex);
    std::shared_ptr<TxOut> txout(r.begin().load());
    return txout;
}

std::shared_ptr<TxOut> Vault::setSendingLabel(const bytes_t& outhash, uint32_t outindex, const std::string& label)
{
    LOGGER(trace) << "Vault::setSendingLabel(" << uchar_vector(outhash).getHex() << ", " << outindex << ", " << label << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    std::shared_ptr<TxOut> txout = setSendingLabel_unwrapped(outhash, outindex, label);
    t.commit();
    return txout;
}

std::shared_ptr<TxOut> Vault::setSendingLabel_unwrapped(const bytes_t& outhash, uint32_t outindex, const std::string& label)
{
    std::shared_ptr<TxOut> txout = getTxOut_unwrapped(outhash, outindex);
    txout->sending_label(label);
    db_->update(txout);
    return txout;
}

std::shared_ptr<TxOut> Vault::setReceivingLabel(const bytes_t& outhash, uint32_t outindex, const std::string& label)
{
    LOGGER(trace) << "Vault::setReceivingLabel(" << uchar_vector(outhash).getHex() << ", " << outindex << ", " << label << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    std::shared_ptr<TxOut> txout = setReceivingLabel_unwrapped(outhash, outindex, label);
    t.commit();
    return txout;
}

std::shared_ptr<TxOut> Vault::setReceivingLabel_unwrapped(const bytes_t& outhash, uint32_t outindex, const std::string& label)
{
    std::shared_ptr<TxOut> txout = getTxOut_unwrapped(outhash, outindex);
    txout->receiving_label(label);
    db_->update(txout);
    return txout;
}


std::shared_ptr<Tx> Vault::exportTx(const bytes_t& hash, const std::string& filepath) const
{
    LOGGER(trace) << "Vault::exportTx(" << uchar_vector(hash).getHex() << ", " << filepath << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif

    std::shared_ptr<Tx> tx;
    {
        odb::core::session s;
        odb::core::transaction t(db_->begin());
        tx = getTx_unwrapped(hash);
    }
    exportTx(tx, filepath);
    return tx;
}

std::shared_ptr<Tx> Vault::exportTx(unsigned long tx_id, const std::string& filepath) const
{
    LOGGER(trace) << "Vault::exportTx(" << tx_id << ", " << filepath << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif

    std::shared_ptr<Tx> tx;
    {
        odb::core::session s;
        odb::core::transaction t(db_->begin());
        tx = getTx_unwrapped(tx_id);
    }
    exportTx(tx, filepath);
    return tx;
}

void Vault::exportTx(std::shared_ptr<Tx> tx, const std::string& filepath) const
{
    LOGGER(trace) << "Vault::exportTx(tx: " << uchar_vector(tx->hash()).getHex()  << ", " << filepath << ")" << std::endl;

    //TODO: disable opetation if file is already open
    std::ofstream ofs(filepath);
    boost::archive::text_oarchive oa(ofs);
    oa << *tx;
}

std::string Vault::exportTx(const bytes_t& hash) const
{
    LOGGER(trace) << "Vault::exportTx(" << uchar_vector(hash).getHex() << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif

    std::shared_ptr<Tx> tx;
    {
        odb::core::session s;
        odb::core::transaction t(db_->begin());
        tx = getTx_unwrapped(hash);
    }
    return exportTx(tx);
}

std::string Vault::exportTx(unsigned long tx_id) const
{
    LOGGER(trace) << "Vault::exportTx(" << tx_id << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif

    std::shared_ptr<Tx> tx;
    {
        odb::core::session s;
        odb::core::transaction t(db_->begin());
        tx = getTx_unwrapped(tx_id);
    }
    return exportTx(tx);
}

std::string Vault::exportTx(std::shared_ptr<Tx> tx) const
{
    LOGGER(trace) << "Vault::exportTx(tx: " << uchar_vector(tx->hash()).getHex()  << ")" << std::endl;

    std::stringstream ss;
    boost::archive::text_oarchive oa(ss);
    oa << *tx;
    return ss.str();
}

std::shared_ptr<Tx> Vault::importTx(const std::string& filepath)
{
    LOGGER(trace) << "Vault::importTx(" << filepath << ")" << std::endl;

    std::ifstream ifs(filepath);
    boost::archive::text_iarchive ia(ifs);

    std::shared_ptr<Tx> tx(new Tx());
    {
        boost::lock_guard<boost::mutex> lock(mutex);
        odb::core::session s;
        odb::core::transaction t(db_->begin());
        ia >> *tx;
        tx = insertTx_unwrapped(tx);     
        if (tx) { t.commit(); }
    }

    signalQueue.flush();
    return tx;
}

std::shared_ptr<Tx> Vault::importTxFromString(const std::string& txstr)
{
    LOGGER(trace) << "Vault::importTxFromString(...)" << std::endl;

    std::stringstream ss;
    ss << txstr;
    boost::archive::text_iarchive ia(ss);

    std::shared_ptr<Tx> tx(new Tx());
    {
        boost::lock_guard<boost::mutex> lock(mutex);
        odb::core::session s;
        odb::core::transaction t(db_->begin());
        ia >> *tx;
        tx = insertTx_unwrapped(tx);     
        if (tx) { t.commit(); }
    }

    signalQueue.flush();
    return tx;
}

unsigned int Vault::exportTxs(const std::string& filepath, uint32_t minheight) const
{
    LOGGER(trace) << "Vault::exportTxs(" << filepath << ", " << minheight << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif

    //TODO: disable opetation if file is already open
    std::ofstream ofs(filepath);
    boost::archive::text_oarchive oa(ofs);

    odb::core::session s;
    odb::core::transaction t(db_->begin());
    return exportTxs_unwrapped(oa, minheight);
}

unsigned int Vault::exportTxs_unwrapped(boost::archive::text_oarchive& oa, uint32_t minheight) const
{
    typedef odb::query<Tx> tx_query_t;
    odb::result<Tx> r;

    std::vector<std::shared_ptr<Tx>> txs;

    // First the confirmed transactions
    r = db_->query<Tx>((tx_query_t::blockheader.is_not_null() && tx_query_t::blockheader->height >= minheight) + "ORDER BY" + tx_query_t::blockheader + "ASC, " + tx_query_t::timestamp + "ASC");
    for (auto it(r.begin()); it != r.end (); ++it) { txs.push_back(it.load()); }

    // Then the unconfirmed
    r = db_->query<Tx>(tx_query_t::blockheader.is_null() + "ORDER BY" + tx_query_t::blockheader + "ASC, " + tx_query_t::timestamp + "ASC");
    for (auto it(r.begin()); it != r.end (); ++it) { txs.push_back(it.load()); }

    uint32_t n = txs.size();
    oa << n;
    for (auto& tx: txs) { oa << *tx; }
    return n;
}

unsigned int Vault::importTxs(const std::string& filepath)
{
    LOGGER(trace) << "Vault::importTxs(" << filepath << ")" << std::endl;

    std::ifstream ifs(filepath);
    boost::archive::text_iarchive ia(ifs);

    uint32_t n;
    {
        boost::lock_guard<boost::mutex> lock(mutex);
        odb::core::transaction t(db_->begin());
        n = importTxs_unwrapped(ia);
        t.commit();
    }

    signalQueue.flush();
    return n;
}

unsigned int Vault::importTxs_unwrapped(boost::archive::text_iarchive& ia)
{
    uint32_t n;
    ia >> n;
    for (uint32_t i = 0; i < n; i++)
    {
        std::shared_ptr<Tx> tx(new Tx());
        ia >> *tx;
        odb::core::session s;
        insertTx_unwrapped(tx);
    }
    return n;
}

//////////////////////////////
// SIGNINGSCRIPT OPERATIONS //
//////////////////////////////
std::shared_ptr<SigningScript> Vault::getSigningScript(const bytes_t& script) const
{
    LOGGER(trace) << "Vault::getSigningScript(" << uchar_vector(script).getHex() << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::session s;
    odb::core::transaction t(db_->begin());
    std::shared_ptr<SigningScript> signingscript = getSigningScript_unwrapped(script);
    return signingscript; 
}

std::shared_ptr<SigningScript> Vault::getSigningScript_unwrapped(const bytes_t& script) const
{
    typedef odb::query<SigningScript> query_t;
    odb::result<SigningScript> r(db_->query<SigningScript>(query_t::txoutscript == script));
    if (r.empty()) throw SigningScriptNotFoundException();
    return r.begin().load(); 
}

///////////////////////////
// BLOCKCHAIN OPERATIONS //
///////////////////////////
uint32_t Vault::getBestHeight() const
{
    LOGGER(trace) << "Vault::getBestHeight()" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());
    return getBestHeight_unwrapped();
}

uint32_t Vault::getBestHeight_unwrapped() const
{
    odb::result<BestHeightView> r(db_->query<BestHeightView>());
    uint32_t best_height = r.empty() ? 0 : r.begin()->height;
    return best_height;
}

std::shared_ptr<BlockHeader> Vault::getBlockHeader(const bytes_t& hash) const
{
    LOGGER(trace) << "Vault::getBlockHeader(" << uchar_vector(hash).getHex() << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());
    return getBlockHeader_unwrapped(hash);
}

std::shared_ptr<BlockHeader> Vault::getBlockHeader(uint32_t height) const
{
    LOGGER(trace) << "Vault::getBlockHeader(" << height << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());
    return getBlockHeader_unwrapped(height);
}

std::shared_ptr<BlockHeader> Vault::getBlockHeader_unwrapped(const bytes_t& hash) const
{
    odb::result<BlockHeader> r(db_->query<BlockHeader>(odb::query<BlockHeader>::hash == hash));
    if (r.empty()) throw BlockHeaderNotFoundException(hash);
    return r.begin().load();
}

std::shared_ptr<BlockHeader> Vault::getBlockHeader_unwrapped(uint32_t height) const
{
    odb::result<BlockHeader> r(db_->query<BlockHeader>(odb::query<BlockHeader>::height == height));
    if (r.empty()) throw BlockHeaderNotFoundException(height);
    return r.empty() ? nullptr : r.begin().load();
}

std::shared_ptr<BlockHeader> Vault::getBestBlockHeader() const
{
    LOGGER(trace) << "Vault::getBestBlockHeader()" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());
    return getBestBlockHeader_unwrapped();
}

std::shared_ptr<BlockHeader> Vault::getBestBlockHeader_unwrapped() const
{
    odb::result<BlockHeader> r(db_->query<BlockHeader>("ORDER BY" + odb::query<BlockHeader>::height + "DESC LIMIT 1"));
    if (r.empty()) return nullptr;
    return r.begin().load();
}

std::shared_ptr<MerkleBlock> Vault::insertMerkleBlock(std::shared_ptr<MerkleBlock> merkleblock)
{
    LOGGER(trace) << "Vault::insertMerkleBlock(" << uchar_vector(merkleblock->blockheader()->hash()).getHex() << ")" << std::endl;

    {
        boost::lock_guard<boost::mutex> lock(mutex);
        odb::core::session s;
        odb::core::transaction t(db_->begin());
        merkleblock = insertMerkleBlock_unwrapped(merkleblock);
        t.commit();
    }

    signalQueue.flush();
    return merkleblock;
}

std::shared_ptr<MerkleBlock> Vault::insertMerkleBlock_unwrapped(std::shared_ptr<MerkleBlock> merkleblock)
{
    try
    {
        auto& new_blockheader = merkleblock->blockheader();
        std::string new_blockheader_hash = uchar_vector(new_blockheader->hash()).getHex();

        odb::result<BlockCountView> blockcount_r(db_->query<BlockCountView>(odb::query<BlockCountView>()));
        unsigned long blockcount = blockcount_r.empty() ? 0 : blockcount_r.begin()->count;
        if (blockcount == 0)
        {
            uint32_t maxFirstBlockTimestamp = getMaxFirstBlockTimestamp_unwrapped();
            if (maxFirstBlockTimestamp == 0)
            {
                LOGGER(debug) << "Vault::insertMerkleBlock_unwrapped - account must exist before inserting blocks." << std::endl;
                return nullptr;
            }

            if (new_blockheader->timestamp() > maxFirstBlockTimestamp)
            {
                LOGGER(debug) << "Vault::insertMerkleBlock_unwrapped - block timestamp is not early enough for accounts in database. hash: " << new_blockheader_hash << ", height: " << new_blockheader->height() << std::endl;
                return nullptr;
            }

            if (new_blockheader->height() == 0)
            {
                LOGGER(debug) << "Vault::insertMerkleBlock_unwrapped - horizon merkle block must have height > 0. hash: " << new_blockheader_hash << ", height: " << new_blockheader->height() << std::endl;
                return nullptr;
            }

            LOGGER(debug) << "Vault::insertMerkleBlock_unwrapped - inserting horizon merkle block. hash: " << new_blockheader_hash << ", height: " << new_blockheader->height() << std::endl;
            db_->persist(new_blockheader);
            db_->persist(merkleblock);
            signalQueue.push(notifyMerkleBlockInserted.bind(merkleblock));
            //notifyMerkleBlockInserted(merkleblock);
            return merkleblock;
        }

        typedef odb::query<BlockHeader> query_t;
        odb::result<BlockHeader> blockheader_r;

        // Check if we already have it
        blockheader_r = db_->query<BlockHeader>(query_t::hash == new_blockheader->hash());
        if (!blockheader_r.empty())
        {
            std::shared_ptr<BlockHeader> blockheader(blockheader_r.begin().load());
            LOGGER(debug) << "Vault::insertMerkleBlock_unwrapped - already have block. hash: " << uchar_vector(blockheader->hash()).getHex() << ", height: " << blockheader->height() << std::endl;
            return nullptr;
        }

        // Check if it connects
        blockheader_r = db_->query<BlockHeader>(query_t::hash == new_blockheader->prevhash());
        if (blockheader_r.empty())
        {
            LOGGER(debug) << "Vault::insertMerkleBlock_unwrapped - could not connect block. hash: " << new_blockheader_hash << ", height: " << new_blockheader->height() << std::endl;
            return nullptr;
        }

        // Make sure we have correct height
        new_blockheader->height(blockheader_r.begin().load()->height() + 1);

        // Make sure this block is unique at this height. All higher blocks are also deleted.
        unsigned int reorg_depth = deleteMerkleBlock_unwrapped(new_blockheader->height());
        if (reorg_depth > 0)
        {
            LOGGER(debug) << "Vault::insertMerkleBlock_unwrapped - reorganization. " << reorg_depth << " blocks removed from chain." << std::endl;
        }

        // Persist merkle block
        LOGGER(debug) << "Vault::insertMerkleBlock_unwrapped - inserting merkle block. hash: " << new_blockheader_hash << ", height: " << new_blockheader->height() << std::endl;
        db_->persist(new_blockheader);
        db_->persist(merkleblock);
        signalQueue.push(notifyMerkleBlockInserted.bind(merkleblock));

        // Confirm transactions
        bool confirmations_updated = false;
        const auto& hashes = merkleblock->hashes();
        odb::result<Tx> tx_r(db_->query<Tx>(odb::query<Tx>::hash.in_range(hashes.begin(), hashes.end())));
        for (auto& tx: tx_r)
        {
            if (tx.blockheader())
            {
                LOGGER(error) << "Vault::insertMerkleBlock_unwrapped - transaction appears in more than one block. hash: " << uchar_vector(tx.hash()).getHex() << std::endl;
                throw MerkleBlockInvalidException(new_blockheader->hash(), new_blockheader->height());
            } 
            LOGGER(debug) << "Vault::insertMerkleBlock_unwrapped - confirming transaction. hash: " << uchar_vector(tx.hash()).getHex() << std::endl;
            tx.blockheader(new_blockheader);
            db_->update(tx);
            confirmations_updated = true;
            signalQueue.push(notifyTxUpdated.bind(std::make_shared<Tx>(tx)));
        }

        if (confirmations_updated)
        {
            db_->update(merkleblock);
        }

        return merkleblock;     
    }
    catch (...)
    {
        signalQueue.clear();
        throw;
    }
}

unsigned int Vault::deleteMerkleBlock(const bytes_t& hash)
{
    return 0;
}

unsigned int Vault::deleteMerkleBlock(uint32_t height)
{
    LOGGER(trace) << "Vault::deleteMerkleBlock(" << height << ")" << std::endl;

    unsigned int count;
    {
        boost::lock_guard<boost::mutex> lock(mutex);
        odb::core::session s;
        odb::core::transaction t(db_->begin());
        count = deleteMerkleBlock_unwrapped(height);
        t.commit();
    }

    signalQueue.flush();
    return count;
}

unsigned int Vault::deleteMerkleBlock_unwrapped(uint32_t height)
{
    try
    {

    /*
        unsigned int count = 0;

        typedef odb::query<ConfirmedTxView> query_t;
        odb::result<ConfirmedTxView> r(db_->query<ConfirmedTxView>((query_t::BlockHeader::height >= height) + "ORDER BY" + query_t::BlockHeader::height));
        for (auto& view: r)
        {
            std::shared_ptr<Tx> tx(db_->find<Tx>(view.tx_id));
            if (tx)
            {
                tx->blockheader(nullptr);
                db_->update(tx);
                signalQueue.push(notifyTxUpdated.bind(tx));
            }

            std::shared_ptr<MerkleBlock> merkleblock(db_->find<MerkleBlock>(view.merkleblock_id));
            if (merkleblock) { db_->erase(merkleblock); }

            std::shared_ptr<BlockHeader> blockheader(db_->find<BlockHeader>(view.blockheader_id));
            if (blockheader)
            {
                db_->erase(blockheader);
                count++;
            }
        }

        return count;
    */

        typedef odb::query<BlockHeader> query_t;
        odb::result<BlockHeader> r(db_->query<BlockHeader>((query_t::height >= height) + "ORDER BY" + query_t::height + "DESC"));
        unsigned int count = 0;
        for (auto& blockheader: r)
        {
            LOGGER(debug) << "Vault::deleteMerkleBlock_unwrapped - deleting block. hash: " << uchar_vector(blockheader.hash()).getHex() << ", height: " << blockheader.height() << std::endl;

            // Remove tx confirmations
            odb::result<Tx> tx_r(db_->query<Tx>(odb::query<Tx>::blockheader == blockheader.id()));
            for (auto& tx: tx_r)
            {
    //            LOGGER(debug) << "Vault::deleteMerkleBlock_unwrapped - unconfirming transaction. hash: " << uchar_vector(tx.hash()).getHex() << std::endl;
                tx.blockheader(nullptr);
                db_->update(tx);
                signalQueue.push(notifyTxUpdated.bind(std::make_shared<Tx>(tx)));
                //notifyTxUpdated(std::make_shared<Tx>(tx));
            }

            // Delete merkle block
            db_->erase_query<MerkleBlock>(odb::query<MerkleBlock>::blockheader == blockheader.id());

            // Delete block header
            db_->erase(blockheader);

            count++;
        }

        return count;
    }
    catch (...)
    {
        signalQueue.clear();
        throw;
    }
}

unsigned int Vault::updateConfirmations_unwrapped(std::shared_ptr<Tx> tx)
{
    LOGGER(debug) << "Vault::updateConfirmations(" << (tx ? uchar_vector(tx->hash()).getHex() : std::string("...")) << ")" << std::endl;

    try
    {
        unsigned int count = 0;
        typedef odb::query<ConfirmedTxView> query_t;
        query_t query(query_t::Tx::blockheader.is_null());
        if (tx) query = (query && query_t::Tx::hash == tx->hash());

        odb::result<ConfirmedTxView> r(db_->query<ConfirmedTxView>(query));
        for (auto& view: r)
        {
            if (view.blockheader_id == 0) continue;

            std::shared_ptr<Tx> tx(db_->load<Tx>(view.tx_id));
            std::shared_ptr<BlockHeader> blockheader(db_->load<BlockHeader>(view.blockheader_id));
            std::shared_ptr<MerkleBlock> merkleblock(db_->load<MerkleBlock>(view.merkleblock_id));

            tx->blockheader(blockheader);
            db_->update(tx);
            signalQueue.push(notifyTxUpdated.bind(tx));
            count++;
            LOGGER(debug) << "Vault::updateConfirmations_unwrapped - transaction " << uchar_vector(tx->hash()).getHex() << " confirmed in block " << uchar_vector(tx->blockheader()->hash()).getHex() << " height: " << tx->blockheader()->height() << std::endl;
        }

        return count;
    }
    catch (...)
    {
        signalQueue.clear();
        throw;
    }
}

void Vault::exportMerkleBlocks(const std::string& filepath) const
{
    LOGGER(trace) << "Vault::exportMerkleBlocks(" << filepath << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif

    // TODO: Disable operation if file is already open
    std::ofstream ofs(filepath);
    boost::archive::text_oarchive oa(ofs);

    odb::core::session s;
    odb::core::transaction t(db_->begin());
    exportMerkleBlocks_unwrapped(oa);
}

void Vault::exportMerkleBlocks_unwrapped(boost::archive::text_oarchive& oa) const
{
    odb::result<MerkleBlockCountView> count_r(db_->query<MerkleBlockCountView>());
    uint32_t n = count_r.empty() ? 0 : count_r.begin()->count;
    oa << n;

    typedef odb::query<MerkleBlock> mb_query_t;
    odb::result<MerkleBlock> mb_r(db_->query<MerkleBlock>("ORDER BY " + mb_query_t::blockheader->height));
    for (auto& merkleblock: mb_r)   { oa << merkleblock; }
}

void Vault::importMerkleBlocks(const std::string& filepath)
{
    LOGGER(trace) << "Vault::importMerkleBlocks(" << filepath << ")" << std::endl;

    std::ifstream ifs(filepath);
    boost::archive::text_iarchive ia(ifs);

    {
        boost::lock_guard<boost::mutex> lock(mutex);
        odb::core::session s;
        odb::core::transaction t(db_->begin());
        importMerkleBlocks_unwrapped(ia);
        t.commit();
    }

    signalQueue.flush();
}

void Vault::importMerkleBlocks_unwrapped(boost::archive::text_iarchive& ia)
{
    uint32_t n;
    ia >> n;
    for (uint32_t i = 0; i < n; i++)
    {
        std::shared_ptr<MerkleBlock> merkleblock(new MerkleBlock());
        ia >> *merkleblock;
        insertMerkleBlock_unwrapped(merkleblock);
    }
}

/////////////////////
// USER OPERATIONS //
/////////////////////
std::shared_ptr<User> Vault::addUser(const std::string& username, bool txoutscript_whitelist_enabled)
{
    LOGGER(trace) << "Vault::addUser(" << username << ", " << (txoutscript_whitelist_enabled ? "true" : "false") << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::transaction t(db_->begin());
    std::shared_ptr<User> user = addUser_unwrapped(username, txoutscript_whitelist_enabled);
    t.commit();

    return user;
}

std::shared_ptr<User> Vault::addUser_unwrapped(const std::string& username, bool txoutscript_whitelist_enabled)
{
    odb::result<User> r(db_->query<User>(odb::query<User>::username == username));
    if (!r.empty()) throw UserAlreadyExistsException(username);

    std::shared_ptr<User> user = std::make_shared<User>(username, txoutscript_whitelist_enabled);
    db_->persist(user);

    return user;
}

std::shared_ptr<User> Vault::getUser(const std::string& username) const
{
    LOGGER(trace) << "Vault::getUser(" << username << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());
    return getUser_unwrapped(username);
}

std::shared_ptr<User> Vault::getUser_unwrapped(const std::string& username) const
{
    odb::result<User> r(db_->query<User>(odb::query<User>::username == username));
    if (r.empty()) throw UserNotFoundException(username);

    std::shared_ptr<User> user(r.begin().load());
    return user;
}

const std::set<bytes_t>& Vault::getTxOutScriptWhitelist(const std::string& username) const
{
    LOGGER(trace) << "Vault::getTxOutScriptWhitelist(" << username << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());

    std::shared_ptr<User> user = getUser_unwrapped(username);
    return user->txoutscript_whitelist();
}

std::shared_ptr<User> Vault::setTxOutScriptWhitelist(const std::string& username, const std::set<bytes_t>& txoutscripts)
{
    LOGGER(trace) << "Vault::setTxOutScriptWhitelist(" << username << ", ...)" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::transaction t(db_->begin());
    std::shared_ptr<User> user = getUser_unwrapped(username);
    user->txoutscript_whitelist(txoutscripts);
    db_->update(user);
    t.commit();

    return user;
}

std::shared_ptr<User> Vault::addTxOutScriptToWhitelist(const std::string& username, const bytes_t& txoutscript)
{
    LOGGER(trace) << "Vault::addTxOutScriptToWhitelist(" << username << ", " << uchar_vector(txoutscript).getHex() << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::transaction t(db_->begin());
    std::shared_ptr<User> user = getUser_unwrapped(username);
    user->addTxOutScriptToWhitelist(txoutscript);
    db_->update(user);
    t.commit();

    return user;
}

std::shared_ptr<User> Vault::removeTxOutScriptFromWhitelist(const std::string& username, const bytes_t& txoutscript)
{
    LOGGER(trace) << "Vault::removeTxOutScriptToWhitelist(" << username << ", " << uchar_vector(txoutscript).getHex() << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::transaction t(db_->begin());
    std::shared_ptr<User> user = getUser_unwrapped(username);
    if (user->removeTxOutScriptFromWhitelist(txoutscript))
    {
        db_->update(user);
        t.commit();
    }

    return user;
}

std::shared_ptr<User> Vault::clearTxOutScriptWhitelist(const std::string& username)
{
    LOGGER(trace) << "Vault::clearTxOutScriptWhitelist()" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::transaction t(db_->begin());
    std::shared_ptr<User> user = getUser_unwrapped(username);
    user->clearTxOutScriptWhitelist();
    db_->update(user);
    t.commit();

    return user;
}

std::shared_ptr<User> Vault::enableTxOutScriptWhitelist(const std::string& username, bool enable)
{
    LOGGER(trace) << "Vault::enableTxOutScriptWhitelist(" << username << ", " << (enable ? "true" : "false") << ")" << std::endl;

    boost::lock_guard<boost::mutex> lock(mutex);
    odb::core::transaction t(db_->begin());
    std::shared_ptr<User> user = getUser_unwrapped(username);
    if (user->isTxOutScriptWhitelistEnabled() != enable)
    {
        user->enableTxOutScriptWhitelist(enable);
        db_->update(user);
        t.commit();
    }

    return user;
}

bool Vault::isTxOutScriptWhitelistEnabled(const std::string& username) const
{
    LOGGER(trace) << "Vault::isTxOutScriptWhitelistEnabled(" << username << ")" << std::endl;

#if defined(LOCK_ALL_CALLS)
    boost::lock_guard<boost::mutex> lock(mutex);
#endif
    odb::core::transaction t(db_->begin());

    std::shared_ptr<User> user = getUser_unwrapped(username);
    return user->isTxOutScriptWhitelistEnabled();
}
