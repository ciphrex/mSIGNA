///////////////////////////////////////////////////////////////////////////////
//
// formatting.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.
//

#pragma once

#include <Base58Check.h>

#include <CoinQ_script.h>

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

////////////
// Tables //
////////////

// SigningScripts
inline std::string formattedScriptHeader()
{
    using namespace std;

    stringstream ss;
    ss << " ";
    ss << left  << setw(15) << "account name" << " | "
       << left  << setw(15) << "bin name" << " | "
       << left  << setw(5)  << "index" << " | "
       << left  << setw(50) << "script" << " | "
       << left  << setw(36) << "address" << " | "
       << left  << setw(8)  << "status";
    ss << " ";

    size_t header_length = ss.str().size();
    ss << endl;
    for (size_t i = 0; i < header_length; i++) { ss << "="; }
    return ss.str();
}

inline std::string formattedScript(const CoinDB::SigningScriptView& view)
{
    using namespace std;
    using namespace CoinDB;

    stringstream ss;
    ss << " ";
    ss << left  << setw(15) << view.account_name << " | "
       << left  << setw(15) << view.account_bin_name << " | "
       << right << setw(5)  << view.index << " | "
       << left  << setw(50) << uchar_vector(view.txoutscript).getHex() << " | "
       << left  << setw(36) << getAddressFromScript(view.txoutscript) << " | "
       << left  << setw(8)  << SigningScript::getStatusString(view.status);
    ss << " ";
    return ss.str();
}

// TxOuts
enum TxOutRecordType { SEND, RECEIVE };

inline std::string formattedTxOutHeader()
{
    using namespace std;

    stringstream ss;
    ss << " ";
    ss << left  << setw(15) << "account name" << " | "
       << left  << setw(15) << "bin name" << " | "
       << left  << setw(10) << "type" << " | "
       << left  << setw(15) << "value" << " | "
       << left  << setw(50) << "script" << " | "
       << left  << setw(36) << "address" << " | "
       << left  << setw(13) << "confirmations" << " | "
       << left  << setw(11) << "tx status" << " | "
       << left  << setw(64) << "tx hash";
    ss << " ";

    size_t header_length = ss.str().size();
    ss << endl;
    for (size_t i = 0; i < header_length; i++) { ss << "="; }
    return ss.str();
}

inline std::string formattedTxOut(const CoinDB::TxOutView& view, TxOutRecordType type, unsigned int best_height)
{
    using namespace std;
    using namespace CoinDB;

    string account_name;
    string bin_name;
    string type_str;
    if (type == SEND)
    {
        account_name = view.sending_account_name;
        type_str     = "SEND";
    }
    else
    {
        account_name = view.receiving_account_name;
        bin_name     = view.account_bin_name;
        type_str     = "RECEIVE";
    }

    bytes_t tx_hash = view.tx_status == Tx::UNSIGNED
        ? view.tx_unsigned_hash : view.tx_hash;

    unsigned int confirmations = view.block_height == 0
        ? 0 : best_height - view.block_height + 1;

    stringstream ss;
    ss << " ";
    ss << left  << setw(15) << account_name << " | "
       << left  << setw(15) << bin_name << " | "
       << left  << setw(10) << type_str << " | "
       << right << setw(15) << view.value << " | "
       << left  << setw(50) << uchar_vector(view.script).getHex() << " | "
       << left  << setw(36) << getAddressFromScript(view.script) << " | "
       << right << setw(13) << confirmations << " | "
       << left  << setw(11) << Tx::getStatusString(view.tx_status) << " | "
       << left  << setw(64) << uchar_vector(tx_hash).getHex();
    ss << " ";
    return ss.str();
}

// Keychains
inline std::string formattedKeychainHeader()
{
    using namespace std;

    stringstream ss;
    ss << " ";
    ss << left  << setw(15) << "keychain name" << " | "
       << left  << setw(5)  << "id" << " | "
       << left  << setw(40) << "hash";
    ss << " ";

    size_t header_length = ss.str().size();
    ss << endl;
    for (size_t i = 0; i < header_length; i++) { ss << "="; }
    return ss.str();
}

// TODO: KeychainView
inline std::string formattedKeychain(const std::shared_ptr<CoinDB::Keychain>& keychain)
{
    using namespace std;
    using namespace CoinDB;

    stringstream ss;
    ss << " ";
    ss << left  << setw(15) << keychain->name() << " | "
       << right << setw(5)  << keychain->id() << " | "
       << left  << setw(40) << uchar_vector(keychain->hash()).getHex();
    ss << " ";
    return ss.str();
}
