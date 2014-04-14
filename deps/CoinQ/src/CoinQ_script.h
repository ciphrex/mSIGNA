///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_script.h 
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef _COINQ_SCRIPT_H_
#define _COINQ_SCRIPT_H_

#include "CoinQ_txs.h"

#include <CoinCore/CoinNodeData.h>

#include <utility>

namespace CoinQ {
namespace Script {

// TODO: place this type into CoinQ or global namespace
typedef std::vector<unsigned char> bytes_t;

/*
 * opPushData - constructs an operator (of variable length) indicating nBytes of data follow.
*/
uchar_vector opPushData(uint32_t nBytes);

/*
 * getDataLength
 *      precondition: pos indicates the position in script of a push operator which is followed by data.
 *      postcondition: pos is advanced to the start of the data.
 *      returns: number of bytes in the data.
*/
uint32_t getDataLength(const uchar_vector& script, uint& pos);

// TODO: Get rid of PUBKEY in names below
enum ScriptType {
    SCRIPT_PUBKEY_UNKNOWN_TYPE,
    SCRIPT_PUBKEY_EMPTY_SCRIPT,
    SCRIPT_PUBKEY_PAY_TO_PUBKEY,
    SCRIPT_PUBKEY_PAY_TO_PUBKEY_HASH,
    SCRIPT_PUBKEY_PAY_TO_SCRIPT_HASH
};

/*
const unsigned char SIGHASH_ALL             = 0x01;
const unsigned char SIGHASH_NONE            = 0x02;
const unsigned char SIGHASH_SINGLE          = 0x03;
const unsigned char SIGHASH_ANYONECANPAY    = 0x80;
*/

enum SigHashType {
    SIGHASH_ALL             = 0x01,
    SIGHASH_NONE            = 0x02,
    SIGHASH_SINGLE          = 0x03,
    SIGHASH_ANYONECANPAY    = 0x80
};

// TODO: make payee_t a class
typedef std::pair<ScriptType, uchar_vector> payee_t;

/*
 * getScriptPubKeyPayee - create a pair containing the type of transaction and the data necessary to fetch
 *      the redeem script.
*/
payee_t getScriptPubKeyPayee(const uchar_vector& scriptPubKey);

/*
 * getTxOutForAddress - create a txout script from a base58check address
*/
uchar_vector getTxOutScriptForAddress(const std::string& address, const unsigned char addressVersions[]);

/*
 * getAddressForTxOutScript - create a base58check address from a txoutscript
*/
std::string getAddressForTxOutScript(const bytes_t& txoutscript, const unsigned char addressVersions[]);


class Script
{
public:
    enum type_t { UNKNOWN, PAY_TO_PUBKEY, PAY_TO_PUBKEY_HASH, PAY_TO_MULTISIG_SCRIPT_HASH };
    Script(type_t type, unsigned int minsigs, const std::vector<bytes_t>& pubkeys, const std::vector<bytes_t>& sigs = std::vector<bytes_t>());
    Script(const bytes_t& txinscript); // with 0-length placeholders for missing signatures

    type_t type() const { return type_; }
    unsigned int minsigs() const { return minsigs_; }
    const std::vector<bytes_t>& pubkeys() const { return pubkeys_; }
    const std::vector<bytes_t>& sigs() const { return sigs_; }

    static const bool USE_PLACEHOLDERS = true;

    enum sigtype_t { EDIT, SIGN, BROADCAST }; 
    /*
     * EDIT         - includes 0-length placeholders for missing signatures
     * SIGN         - format the script for signing
     * BROADCAST    - format the script for broadcast (remove 0-length placeholders)
    */

    bytes_t txinscript(sigtype_t sigtype) const;
    bytes_t txoutscript() const;

    const bytes_t& redeemscript() const { return redeemscript_; }
    const bytes_t& hash() const { return hash_; }

    unsigned int sigsneeded() const; // returns how many signatures are still needed
    std::vector<bytes_t> missingsigs() const; // returns pubkeys for which we are still missing signatures
    bool addSig(const bytes_t& pubkey, const bytes_t& sig); // returns true iff signature was absent and has been added
    void clearSigs(); // resets all signatures to 0-length placeholders
    unsigned int mergesigs(const Script& other); // merges the signatures from another script that is otherwise identical. returns number of signatures added.

private:
    type_t type_;
    unsigned int minsigs_;
    std::vector<bytes_t> pubkeys_;
    std::vector<bytes_t> sigs_; // empty vectors for missing signatures

    bytes_t redeemscript_; // empty if type is not script hash
    bytes_t hash_;
};

}
}

#endif // _COINQ_SCRIPT_H_
