///////////////////////////////////////////////////////////////////////////////
//
// formatting.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include <CoinCore/Base58Check.h>

#include <CoinQ/CoinQ_coinparams.h>
#include <CoinQ/CoinQ_script.h>

#include <Vault.h>

// TODO: create separate library
#include <stdutils/stringutils.h>

#include <sstream>

// TODO: Get this from a config file
const unsigned char BASE58_VERSIONS[] = { 0x00, 0x05 };

// Data formatting
inline std::string getAddressFromScript(const bytes_t& script)
{
    using namespace CoinQ::Script;

    payee_t payee = getScriptPubKeyPayee(script);
    if (payee.first == SCRIPT_PUBKEY_PAY_TO_PUBKEY_HASH)
        return toBase58Check(payee.second, BASE58_VERSIONS[0]);
    else if (payee.first == SCRIPT_PUBKEY_PAY_TO_SCRIPT_HASH)
        return toBase58Check(payee.second, BASE58_VERSIONS[1]);
    else
        return "N/A";
}


const int COIN_EXP = 100000000ull;

////////////
// Tables //
////////////

// SigningScripts
inline std::string formattedScriptHeader()
{
    using namespace std;

    stringstream ss;
    ss << " ";
    ss << left  << setw(8)  << "account" << " | "
       << left  << setw(8)  << "bin" << " | "
       << left  << setw(8)  << "label" << " | "
       << right << setw(5)  << "index" << " | "
       << left  << setw(50) << "script" << " | "
       << left  << setw(36) << "address" << " | "
       << left  << setw(8)  << "status";
    ss << " ";

    size_t header_length = ss.str().size();
    ss << endl;
    for (size_t i = 0; i < header_length; i++) { ss << "="; }
    return ss.str();
}

inline std::string formattedScript(const CoinDB::SigningScriptView& view, const CoinQ::CoinParams& coinParams)
{
    using namespace std;
    using namespace CoinDB;
    using namespace CoinQ::Script;

    stringstream ss;
    ss << " ";
    ss << left  << setw(8)  << view.account_name << " | "
       << left  << setw(8)  << view.account_bin_name << " | "
       << left  << setw(8)  << view.label << " | "
       << right << setw(5)  << view.index << " | "
       << left  << setw(50) << uchar_vector(view.txoutscript).getHex() << " | "
       << left  << setw(36) << getAddressForTxOutScript(view.txoutscript, coinParams.address_versions()) << " | "
       << left  << setw(8)  << SigningScript::getStatusString(view.status);
    ss << " ";
    return ss.str();
}

// TxIns
inline std::string formattedTxInHeader()
{
    using namespace std;

    stringstream ss;
    ss << " ";
    ss << right << setw(5)  << "input" << " | "
       << left  << setw(68) << "outpoint" << " | "
       << right << setw(15) << "value";
    ss << " ";

    size_t header_length = ss.str().size();
    ss << endl;
    for (size_t i = 0; i < header_length; i++) { ss << "="; }
    return ss.str();
}

inline std::string formattedTxIn(const std::shared_ptr<CoinDB::TxIn>& txin)
{
    using namespace std;
    using namespace CoinDB;

    stringstream outpoint_str;
    outpoint_str << uchar_vector(txin->outhash()).getHex() << ":" << txin->outindex();

    std::shared_ptr<TxOut> outpoint = txin->outpoint();
    stringstream value;
    if (outpoint)
    {
        value << fixed << setprecision(8) << 1.0*outpoint->value()/COIN_EXP;
    }
    else
    {
        value << "N/A";
    }

    stringstream ss;
    ss << " ";
    ss << right << setw(5)  << txin->txindex() << " | "
       << left  << setw(68) << outpoint_str.str() << " | "
       << right << setw(15) << value.str();
    ss << " ";
    return ss.str();
}

// TxOuts
inline std::string formattedTxOutHeader()
{
    using namespace std;

    stringstream ss;
    ss << " ";
    ss << right << setw(6)  << "output" << " | "
       << right << setw(15) << "value" << " | "
       << left  << setw(50) << "script" << " | "
       << left  << setw(36) << "address" << " | "
       << left  << setw(7)  << "status";
    ss << " ";

    size_t header_length = ss.str().size();
    ss << endl;
    for (size_t i = 0; i < header_length; i++) { ss << "="; }
    return ss.str();
}

inline std::string formattedTxOut(const std::shared_ptr<CoinDB::TxOut>& txout, const CoinQ::CoinParams& coinParams)
{
    using namespace std;
    using namespace CoinDB;
    using namespace CoinQ::Script;

    string status = txout->receiving_account() ? TxOut::getStatusString(txout->status()) : "N/A";

    stringstream ss;
    ss << " ";
    ss << right << setw(6)  << txout->txindex() << " | "
       << right << setw(15) << fixed << setprecision(8) << 1.0*txout->value()/COIN_EXP << " | "
       << left  << setw(50) << uchar_vector(txout->script()).getHex() << " | "
       << left  << setw(36) << getAddressForTxOutScript(txout->script(), coinParams.address_versions()) << " | "
       << left  << setw(7)  << status;
    ss << " ";
    return ss.str();
}

inline std::string formattedTxOutViewHeader()
{
    using namespace std;

    stringstream ss;
    ss << " ";
    ss << left  << setw(8)  << "account" << " | "
       << left  << setw(8)  << "bin" << " | "
       << left  << setw(40) << "description" << " | "
       << left  << setw(7)  << "type" << " | "
       << right << setw(15) << "value" << " | "
       << left  << setw(36) << "address" << " | "
       << right << setw(6)  << "confs" << " | "
       << left  << setw(10) << "tx status" << " | "
       << right << setw(6)  << "tx id" << " | "
       << left  << setw(64) << "tx hash";
    ss << " ";

    size_t header_length = ss.str().size();
    ss << endl;
    for (size_t i = 0; i < header_length; i++) { ss << "="; }
    return ss.str();
}

inline std::string formattedTxOutView(const CoinDB::TxOutView& view, unsigned int best_height, const CoinQ::CoinParams& coinParams)
{
    using namespace std;
    using namespace CoinDB;
    using namespace CoinQ::Script;

    bytes_t tx_hash = view.tx_status == Tx::UNSIGNED
        ? view.tx_unsigned_hash : view.tx_hash;

    unsigned int confirmations = view.height == 0
        ? 0 : best_height - view.height + 1;

    stringstream ss;
    ss << " ";
    ss << left  << setw(8)  << view.role_account() << " | "
       << left  << setw(8)  << view.role_bin() << " | "
       << left  << setw(40) << view.role_label() << " | "
       << left  << setw(7)  << TxOut::getRoleString(view.role_flags) << " | "
       << right << setw(15) << fixed << setprecision(8) << 1.0*view.value/COIN_EXP << " | "
       << left  << setw(36) << getAddressForTxOutScript(view.script, coinParams.address_versions()) << " | "
       << right << setw(6)  << confirmations << " | "
       << left  << setw(10) << Tx::getStatusString(view.tx_status) << " | "
       << right << setw(6)  << view.tx_id << " | "
       << left  << setw(64) << uchar_vector(tx_hash).getHex();
    ss << " ";
    return ss.str();
}

inline std::string formattedTxOutViewCSV(const CoinDB::TxOutView& view, unsigned int best_height, const CoinQ::CoinParams& coinParams)
{
    using namespace std;
    using namespace CoinDB;
    using namespace CoinQ::Script;

    bytes_t tx_hash = view.tx_status == Tx::UNSIGNED
        ? view.tx_unsigned_hash : view.tx_hash;

    unsigned int confirmations = view.height == 0
        ? 0 : best_height - view.height + 1;

    stringstream ss;
    ss << view.role_account() << ","
       << view.role_bin() << ","
       << view.role_label() << ","
       << TxOut::getRoleString(view.role_flags) << ","
       << setprecision(8) << 1.0*view.value/COIN_EXP << ","
       << getAddressForTxOutScript(view.script, coinParams.address_versions()) << ","
       << confirmations << ","
       << Tx::getStatusString(view.tx_status) << ","
       << uchar_vector(tx_hash).getHex();
    return ss.str();
}

inline std::string formattedUnspentTxOutViewHeader()
{
    using namespace std;

    stringstream ss;
    ss << " ";
    ss << right << setw(8)  << "id" << " | "
       << right << setw(15) << "value" << " | "
       << left  << setw(36) << "address" << " | "
       << right << setw(8)  << "confs";
    ss << " ";

    size_t header_length = ss.str().size();
    ss << endl;
    for (size_t i = 0; i < header_length; i++) { ss << "="; }
    return ss.str();
}

inline std::string formattedUnspentTxOutView(const CoinDB::TxOutView& view, unsigned int best_height, const CoinQ::CoinParams& coinParams)
{
    using namespace std;
    using namespace CoinDB;
    using namespace CoinQ::Script;

    unsigned int confirmations = view.height == 0
        ? 0 : best_height - view.height + 1;

    stringstream ss;
    ss << " ";
    ss << right << setw(8)  << view.id << " | "
       << right << setw(15) << fixed << setprecision(8) << 1.0*view.value/COIN_EXP << " | "
       << left  << setw(36) << getAddressForTxOutScript(view.script, coinParams.address_versions()) << " | "
       << right << setw(6)  << confirmations;
    ss << " ";
    return ss.str();
}

// Transactions
inline std::string formattedTxViewHeader()
{
    using namespace std;

    stringstream ss;
    ss << " ";
    ss << right << setw(6)  << "id" << " | "
       << left  << setw(64) << "hash" << " | "
       << right << setw(7)  << "version" << " | "
       << right << setw(11) << "locktime" << " | "
       << right << setw(11) << "timestamp" << " | "
       << left  << setw(10) << "status" << " | "
       << right << setw(6)  << "confs" << " | "
       << right << setw(15) << "txin total" << " | "
       << right << setw(15) << "txout total" << " | "
       << right << setw(9)  << "fee";
    ss << " ";

    size_t header_length = ss.str().size();
    ss << endl;
    for (size_t i = 0; i < header_length; i++) { ss << "="; }
    return ss.str();
}

inline std::string formattedTxView(const CoinDB::TxView& view, unsigned int best_height)
{
    using namespace std;
    using namespace CoinDB;

    bytes_t hash = view.status == Tx::UNSIGNED
        ? view.unsigned_hash : view.hash;

    unsigned int confirmations = view.height == 0
        ? 0 : best_height - view.height + 1;

    stringstream txin_total;
    stringstream txout_total;
    stringstream fee;
    if (view.have_all_outpoints)
    {
        txin_total << fixed << setprecision(8) << 1.0*view.txin_total/COIN_EXP;
        fee << fixed << setprecision(8) << 1.0*view.fee()/COIN_EXP;
    }
    else
    {
        txin_total << "N/A";
        fee << "N/A";
    }
    txout_total << fixed << setprecision(8) << 1.0*view.txout_total/COIN_EXP;

    stringstream ss;
    ss << " ";
    ss << right << setw(6)  << view.id << " | "
       << left  << setw(64) << uchar_vector(hash).getHex() << " | "
       << right << setw(7)  << view.version << " | "
       << right << setw(11) << view.locktime << " | "
       << right << setw(11) << view.timestamp << " | "
       << left  << setw(10) << Tx::getStatusString(view.status) << " | "
       << right << setw(6)  << confirmations << " | "
       << right << setw(15) << txin_total.str() << " | "
       << right << setw(15) << txout_total.str() << " | "
       << right << setw(9)  << fee.str();
    ss << " ";
    return ss.str();     
}

// Keychains
inline std::string formattedKeychainViewHeader()
{
    using namespace std;

    stringstream ss;
    ss << " ";
    ss << left  << setw(15) << "keychain" << " | "
       << left  << setw(7)  << "type" << " | "
       << left  << setw(9)  << "encrypted" << " | "
       << left  << setw(6)  << "locked" << " | "
       << right << setw(5)  << "id" << " | "
       << left  << setw(40) << "hash";
    ss << " ";

    size_t header_length = ss.str().size();
    ss << endl;
    for (size_t i = 0; i < header_length; i++) { ss << "="; }
    return ss.str();
}

inline std::string formattedKeychainView(const CoinDB::KeychainView& view)
{
    using namespace std;
    using namespace CoinDB;

    stringstream ss;
    ss << " ";
    ss << left  << setw(15) << view.name << " | "
       << left  << setw(7)  << (view.is_private ? "PRIVATE" : "PUBLIC") << " | "
       << left  << setw(9)  << (view.is_encrypted ? "YES" : "NO") << " | "
       << left  << setw(6)  << (view.is_locked ? "YES" : "NO") << " | "
       << right << setw(5)  << view.id << " | "
       << left  << setw(40) << uchar_vector(view.hash).getHex();
    ss << " ";
    return ss.str();
}

inline std::string formattedKeychainHeader()
{
    using namespace std;

    stringstream ss;
    ss << " ";
    ss << left  << setw(15) << "keychain name" << " | "
       << left  << setw(7)  << "type" << " | "
       << right << setw(5)  << "id" << " | "
       << left  << setw(40) << "hash";
    ss << " ";

    size_t header_length = ss.str().size();
    ss << endl;
    for (size_t i = 0; i < header_length; i++) { ss << "="; }
    return ss.str();
}

inline std::string formattedKeychain(const std::shared_ptr<CoinDB::Keychain>& keychain)
{
    using namespace std;
    using namespace CoinDB;

    stringstream ss;
    ss << " ";
    ss << left  << setw(15) << keychain->name() << " | "
       << left  << setw(7)  << (keychain->isPrivate() ? "PRIVATE" : "PUBLIC") << " | "
       << right << setw(5)  << keychain->id() << " | "
       << left  << setw(40) << uchar_vector(keychain->hash()).getHex();
    ss << " ";
    return ss.str();
}

// Accounts
inline std::string formattedAccountHeader()
{
    using namespace std;

    stringstream ss;
    ss << " ";
    ss << left  << setw(15) << "account name" << " | "
       << right << setw(5)  << "id" << " | "
       << left  << setw(15) << "compressed keys" << " | "
       << left  << setw(64) << "policy";
    ss << " ";

    size_t header_length = ss.str().size();
    ss << endl;
    for (size_t i = 0; i < header_length; i++) { ss << "="; }
    return ss.str();
}

inline std::string formattedAccount(const CoinDB::AccountInfo& info)
{
    using namespace std;
    using namespace CoinDB;

    stringstream policy;
    policy << info.minsigs() << " of " << stdutils:: delimited_list(info.keychain_names(), ", ");

    stringstream ss;
    ss << " ";
    ss << left  << setw(15) << info.name() << " | "
       << right << setw(5)  << info.id() << " | "
       << left  << setw(15) << (info.compressed_keys() ? "TRUE" : "FALSE") << " | "
       << left  << setw(64) << policy.str();
    ss << " ";
    return ss.str();
}

// Account bins
inline std::string formattedAccountBinViewHeader()
{
    using namespace std;

    stringstream ss;
    ss << " ";
    ss << left  << setw(15) << "account name" << " | "
       << left  << setw(15) << "bin name" << " | "
       << right << setw(10) << "account id" << " | "
       << right << setw(6)  << "bin id" << " | "
       << left  << setw(40) << "account hash" << " | "
       << left  << setw(40) << "bin hash";
    ss << " ";

    size_t header_length = ss.str().size();
    ss << endl;
    for (size_t i = 0; i < header_length; i++) { ss << "="; }
    return ss.str();
}

inline std::string formattedAccountBinView(const CoinDB::AccountBinView& view)
{
    using namespace std;
    using namespace CoinDB;

    stringstream ss;
    ss << " ";
    ss << left  << setw(15) << view.account_name << " | "
       << left  << setw(15) << view.bin_name << " | "
       << right << setw(10) << view.account_id << " | "
       << right << setw(6)  << view.bin_id << " | "
       << left  << setw(40) << uchar_vector(view.account_hash).getHex() << " | "
       << left  << setw(40) << uchar_vector(view.bin_hash).getHex();
    ss << " ";
    return ss.str();
}

