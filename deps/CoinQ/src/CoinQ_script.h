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
#include <CoinCore/typedefs.h>

#include <algorithm>
#include <utility>

namespace CoinQ {
namespace Script {

enum
{
    // Constants
    OP_0 = 0,
    OP_PUSHDATA1 = 0x4c,
    OP_PUSHDATA2,
    OP_PUSHDATA4,
    OP_1NEGATE,
    OP_1_OFFSET,
    OP_1,
    OP_2,
    OP_3,
    OP_4,
    OP_5,
    OP_6,
    OP_7,
    OP_8,
    OP_9,
    OP_10,
    OP_11,
    OP_12,
    OP_13,
    OP_14,
    OP_15,
    OP_16,

    // Flow control
    OP_NOP,
    OP_IF = 0x63,
    OP_NOTIF,
    OP_ELSE = 0x67,
    OP_ENDIF,
    OP_VERIFY,
    OP_RETURN,

    // Stack
    OP_TOALTSTACK,
    OP_FROMALTSTACK,
    OP_2DROP,
    OP_2DUP,
    OP_3DUP,
    OP_2OVER,
    OP_2ROT,
    OP_2SWAP,
    OP_IFDUP,
    OP_DEPTH,
    OP_DROP,
    OP_DUP,
    OP_NIP,
    OP_OVER,
    OP_PICK,
    OP_ROLL,
    OP_ROT,
    OP_SWAP,
    OP_TUCK,

    // Splice
    OP_CAT,
    OP_SUBSTR,
    OP_LEFT,
    OP_RIGHT,
    OP_SIZE,

    // Bitwise logic
    OP_INVERT,
    OP_AND,
    OP_OR,
    OP_XOR,
    OP_EQUAL,
    OP_EQUALVERIFY,

    // Arithmetic
    OP_1ADD = 0x8b,
    OP_1SUB,
    OP_2MUL,
    OP_2DIV,
    OP_NEGATE,
    OP_ABS,
    OP_NOT,
    OP_0NOTEQUAL,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_LSHIFT,
    OP_RSHIFT,
    OP_BOOLAND,
    OP_BOOLOR,
    OP_NUMEQUAL,
    OP_NUMEQUALVERIFY,
    OP_NUMNOTEQUAL,
    OP_LESSTHAN,
    OP_GREATERTHAN,
    OP_LESSTHANOREQUAL,
    OP_GREATERTHANOREQUAL,
    OP_MIN,
    OP_MAX,
    OP_WITHIN,

    // Crypto
    OP_RIPEMD160,
    OP_SHA1,
    OP_SHA256,
    OP_HASH160,
    OP_HASH256,
    OP_CODESEPARATOR,
    OP_CHECKSIG,
    OP_CHECKSIGVERIFY,
    OP_CHECKMULTISIG,
    OP_CHECKMULTISIGVERIFY,

    // Expansion
    OP_NOP1,
    OP_CHECKLOCKTIMEVERIFY,
    OP_NOP3,
    OP_NOP4,
    OP_NOP5,
    OP_NOP6,
    OP_NOP7,
    OP_NOP8,
    OP_NOP9,
    OP_NOP10,

    // Pseudo
    OP_PUBKEYHASH = 0xfd,
    OP_PUBKEY,
    OP_INVALIDOPCODE,
};

/*
 * opPushData - constructs an operator (of variable length) indicating nBytes of data follow.
*/
uchar_vector opPushData(uint32_t nBytes);

/*
 * pushStackItem - constructs a stack push for an object of arbitrary length.
*/
uchar_vector pushStackItem(const uchar_vector& data);

/*
 * getDataLength
 *      precondition: pos indicates the position in script of a push operator which is followed by data.
 *      postcondition: pos is advanced to the start of the data.
 *      returns: number of bytes in the data.
*/
uint32_t getDataLength(const uchar_vector& script, uint& pos);

/*
 * getNextOp
 *      precondition: pos indicates the position in script
 *      postcondition: pos is advanced to the start of the next operation or end of script
 *      returns: the complete op starting at pos including any data if pushed
*/
uchar_vector getNextOp(const bytes_t& script, uint& pos);

// TODO: Get rid of PUBKEY in names below
enum ScriptType {
    SCRIPT_PUBKEY_UNKNOWN_TYPE,
    SCRIPT_PUBKEY_EMPTY_SCRIPT,
    SCRIPT_PUBKEY_PAY_TO_PUBKEY,
    SCRIPT_PUBKEY_PAY_TO_PUBKEY_HASH,
    SCRIPT_PUBKEY_PAY_TO_SCRIPT_HASH
};

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
 * isValidAddress - check whether address is valid
*/
bool isValidAddress(const std::string& address, const unsigned char addressVersions[]);

/*
 * getTxOutForAddress - create a txout script from a base58check address
*/
uchar_vector getTxOutScriptForAddress(const std::string& address, const unsigned char addressVersions[]);

/*
 * getAddressForTxOutScript - create a base58check address from a txoutscript
*/
std::string getAddressForTxOutScript(const bytes_t& txoutscript, const unsigned char addressVersions[]);


class SymmetricKeyGroup
{
public:
    SymmetricKeyGroup(const std::vector<bytes_t>& pubkeys) : pubkeys_(pubkeys)
    {
        std::sort(pubkeys_.begin(), pubkeys_.end());
    }

    std::size_t count() const { return pubkeys_.size(); }

    const bytes_t& operator[](std::size_t i) const { return pubkeys_[i]; }
    const std::vector<bytes_t> pubkeys() const { return pubkeys_; }

private:
    std::vector<bytes_t> pubkeys_;    
};

class ScriptTemplate
{
public:
    ScriptTemplate(const uchar_vector& pattern) : pattern_(pattern) { }

    const uchar_vector& pattern() const { return pattern_; }

    const uchar_vector& script() const;
    uchar_vector script(const bytes_t& pubkey) const; // use index 0 only
    uchar_vector script(const std::vector<bytes_t>& pubkeys) const;

    ScriptTemplate& reduce(const bytes_t& pubkey);
    ScriptTemplate& reduce(const std::vector<bytes_t>& pubkeys);
    ScriptTemplate& reset();
    
private:
    uchar_vector pattern_;
    uchar_vector reduced_;
};

class WitnessProgram
{
public:
    WitnessProgram(const WitnessProgram& wp) : redeemscript_(wp.redeemscript_) { update(); }
    WitnessProgram(const uchar_vector& redeemscript) : redeemscript_(redeemscript) { update(); }

    int version() const { return (redeemscript_.size() <= 32) ? 0 : 1; }

    const uchar_vector& witnessscript() const { return witnessscript_; }
    const uchar_vector& redeemscript() const { return redeemscript_; }
    const uchar_vector& txinscript() const { return txinscript_; }
    const uchar_vector& txoutscript() const { return txoutscript_; }

    std::string address(const unsigned char addressVersions[]) const;

private:
    uchar_vector witnessscript_;
    uchar_vector redeemscript_;
    uchar_vector txinscript_;
    uchar_vector txoutscript_;

    void update();
};

class Script
{
public:
    enum type_t { UNKNOWN, PAY_TO_PUBKEY, PAY_TO_PUBKEY_HASH, PAY_TO_MULTISIG_SCRIPT_HASH };
    Script(type_t type, unsigned int minsigs, const std::vector<bytes_t>& pubkeys, const std::vector<bytes_t>& sigs = std::vector<bytes_t>());

    // If signinghash is empty, 0-length placeholders are expected in txinscript for missing signatures and no signatures are checked.
    // If signinghash is not empty, all signatures are checked and 0-length placeholders are inserted as necessary.
    // If clearinvalidsigs is true, invalid signatures are cleared. Otherwise, an exception is thrown if signatures are invalid.
    explicit Script(const bytes_t& txinscript, const bytes_t& signinghash = bytes_t(), bool clearinvalidsigs = false);

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
    std::vector<bytes_t> presentsigs() const; // returns pubkeys for which we have signatures
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


typedef std::vector<Script> scripts_t;

class Signer
{
public:
    Signer() : isSigned_(false) { }
    explicit Signer(const Coin::Transaction& tx, bool clearinvalidsigs = false) { setTx(tx, clearinvalidsigs); }

    void setTx(const Coin::Transaction& tx, bool clearinvalidsigs = false);
    const Coin::Transaction& getTx() const { return tx_; }

    // sign returns a vector of the pubkeys for which signatures were added.
    std::vector<bytes_t> sign(const std::vector<secure_bytes_t>& privkeys);

    bool isSigned() const { return isSigned_; }

    const scripts_t& getScripts() const { return scripts_; }

    void clearSigs() { for (auto& script: scripts_) script.clearSigs(); }

private:
    Coin::Transaction tx_;
    bool isSigned_;
    scripts_t scripts_;
};

}
}

#endif // _COINQ_SCRIPT_H_
