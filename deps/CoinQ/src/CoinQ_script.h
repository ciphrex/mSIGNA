///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_script.h 
//
// Copyright (c) 2013-2016 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#ifndef _COINQ_SCRIPT_H_
#define _COINQ_SCRIPT_H_

#include "CoinQ_txs.h"

#include <CoinCore/CoinNodeData.h>
#include <CoinCore/hdkeys.h>
#include <CoinCore/typedefs.h>

#include <algorithm>
#include <deque>
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

enum
{
    OP_TOKENHASH = OP_PUBKEYHASH,
    OP_TOKEN,
    OP_SIG,
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
 *      returns: the complete op starting at pos including any data if pushed - if pushdataonly is true,
 *               only the data pushed to the stack is returned
*/
uchar_vector getNextOp(const bytes_t& script, uint& pos, bool pushdataonly = false);

// TODO: Get rid of PUBKEY in names below
enum ScriptType {
    SCRIPT_PUBKEY_UNKNOWN_TYPE,
    SCRIPT_PUBKEY_EMPTY_SCRIPT,
    SCRIPT_PUBKEY_PAY_TO_PUBKEY,
    SCRIPT_PUBKEY_PAY_TO_PUBKEY_HASH,
    SCRIPT_PUBKEY_PAY_TO_SCRIPT_HASH,
    SCRIPT_PUBKEY_PAY_TO_WITNESS_PUBKEY_HASH,
    SCRIPT_PUBKEY_PAY_TO_WITNESS_SCRIPT_HASH,
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
    SymmetricKeyGroup(const std::vector<uchar_vector>& pubkeys) : pubkeys_(pubkeys)
    {
        sort();
    }

    std::size_t count() const { return pubkeys_.size(); }

    const uchar_vector& operator[](std::size_t i) const { return pubkeys_[i]; }
    const std::vector<uchar_vector>& pubkeys() const { return pubkeys_; }

protected:
    std::vector<uchar_vector> pubkeys_;

    SymmetricKeyGroup() { }
    void sort() { std::sort(pubkeys_.begin(), pubkeys_.end()); }
};

class SymmetricHDKeyGroup : public SymmetricKeyGroup
{
public:
    SymmetricHDKeyGroup(const std::vector<uchar_vector>& extkeys);

    uint32_t index() const { return index_; }

    SymmetricHDKeyGroup& next()
    {
        index_++;
        update();
        return *this;
    }

    SymmetricHDKeyGroup& next(uint32_t index)
    {
        index_ = index;
        update();
        return *this;
    }
    
protected:
    std::vector<Coin::HDKeychain> keychains_;
    uint32_t index_;

    void update();
};


typedef std::vector<bytes_t> scriptstack_t;

scriptstack_t scriptToStack(const uchar_vector& script);

class ScriptTemplate
{
public:
    ScriptTemplate() { }
    ScriptTemplate(const uchar_vector& pattern) : pattern_(pattern) { }

    const uchar_vector& pattern() const { return pattern_; }
    void pattern(const uchar_vector& pattern) { pattern_ = pattern; reduced_.clear(); }

    const uchar_vector& script() const;
    uchar_vector script(const uchar_vector& token) const; // use index 0 only
    uchar_vector script(const std::vector<uchar_vector>& tokens) const;

    scriptstack_t stack() const { return scriptToStack(script()); }
    scriptstack_t stack(const uchar_vector& token) const { return scriptToStack(script(token)); }
    scriptstack_t stack(const std::vector<uchar_vector>& tokens) const { return scriptToStack(script(tokens)); }

    ScriptTemplate& apply(const uchar_vector& token);
    ScriptTemplate& apply(const std::vector<uchar_vector>& tokens);
    ScriptTemplate& reset();
    

private:
    uchar_vector pattern_;
    uchar_vector reduced_;
};

// Witness program types
enum WitnessProgramType
{
    WITNESS_UNDEFINED,
    WITNESS_NONE,
    WITNESS_P2WPKH,
    WITNESS_P2WSH,
};

WitnessProgramType getWitnessProgramType(const uchar_vector& wp); 

// Abstract base class for witness programs
class WitnessProgram
{
public:
    const uchar_vector&                 script() const { return script_; }
    uchar_vector                        p2shscript() const;
    std::string                         p2shaddress(const unsigned char addressVersions[]) const;
    const std::deque<uchar_vector>&     stack() const { return stack_; }

    // Subclasses must implement address()
    virtual std::string                 address(const unsigned char addressVersions[]) const = 0;
    virtual WitnessProgramType          type() const { return WITNESS_UNDEFINED; }
    virtual std::string                 typestring() const { return "WITNESS_UNDEFINED"; }

protected:
    uchar_vector script_;
    std::deque<uchar_vector> stack_;

    // Subclasses must implement update() and call it from the constructor
    virtual void update() = 0;
};

// Version 0: Pay to witness pubkey hash
class WitnessProgram_P2WPKH : public WitnessProgram
{
public:
    WitnessProgram_P2WPKH(const uchar_vector& pubkey) : pubkey_(pubkey) { update(); }

    const uchar_vector& pubkey() const { return pubkey_; }
    const uchar_vector& pubkeyhash() const { return pubkeyhash_; }
    std::string         address(const unsigned char addressVersions[]) const;
    WitnessProgramType  type() const { return WITNESS_P2WPKH; }
    std::string         typestring() const { return "WITNESS_P2WPKH"; }

protected:
    uchar_vector pubkey_;
    uchar_vector pubkeyhash_;

    void update(); 
};

// Version 0: Pay to witness script hash
class WitnessProgram_P2WSH : public WitnessProgram
{
public:
    WitnessProgram_P2WSH(const uchar_vector& redeemscript) : redeemscript_(redeemscript) { update(); }

    const uchar_vector& redeemscript() const { return redeemscript_; }
    const uchar_vector& redeemscripthash() const { return redeemscripthash_; }
    std::string         address(const unsigned char addressVersions[]) const;
    WitnessProgramType  type() const { return WITNESS_P2WSH; }
    std::string         typestring() const { return "WITNESS_P2WSH"; }

protected:
    uchar_vector redeemscript_;
    uchar_vector redeemscripthash_;

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


class SignableTxIn
{
public:
    enum type_t
    {
        UNKNOWN,
        MISSING_WITNESS,
        PAY_TO_PUBKEY,
        PAY_TO_PUBKEY_HASH,
        PAY_TO_M_OF_N_SCRIPT_HASH,
        PAY_TO_PUBKEY_HASH_WITNESS_V0,
        PAY_TO_M_OF_N_WITNESS_V0,
    };

    type_t type() const { return type_; }

    const char* typestring() const
    {
        switch (type_)
        {
        case UNKNOWN:                       return "UNKNOWN";
        case MISSING_WITNESS:               return "MISSING_WITNESS";
        case PAY_TO_PUBKEY:                 return "PAY_TO_PUBKEY";
        case PAY_TO_PUBKEY_HASH:            return "PAY_TO_PUBKEY_HASH";
        case PAY_TO_M_OF_N_SCRIPT_HASH:     return "PAY_TO_M_OF_N_SCRIPT_HASH";
        case PAY_TO_PUBKEY_HASH_WITNESS_V0: return "PAY_TO_PUBKEY_HASH_WITNESS_V0";
        case PAY_TO_M_OF_N_WITNESS_V0:      return "PAY_TO_M_OF_N_WITNESS_V0";
        default:                            return "UNDEFINED";
        }
    }

    explicit SignableTxIn(const SignableTxIn& other) :
        type_(other.type_),
        minsigs_(other.minsigs_),
        pubkeys_(other.pubkeys_),
        sigs_(other.sigs_),
        redeemscript_(other.redeemscript_) { }

    SignableTxIn(const Coin::Transaction& tx, std::size_t nIn, uint64_t outpointamount = 0, const bytes_t& txoutscript = bytes_t()) { setTxIn(tx, nIn, outpointamount); }

    void setTxIn(const Coin::Transaction& tx, std::size_t nIn, uint64_t outpointamount = 0, const bytes_t& txoutscript = bytes_t());

    unsigned int minsigs() const { return minsigs_; }
    const std::vector<bytes_t>& pubkeys() const { return pubkeys_; }
    const std::vector<bytes_t>& sigs() const { return sigs_; }

    bytes_t txinscript() const;
    bytes_t txoutscript() const;
    Coin::ScriptWitness scriptwitness() const;

    const bytes_t& redeemscript() const { return redeemscript_; }

    unsigned int sigsneeded() const; // returns how many signatures are still needed
    std::vector<bytes_t> missingsigs() const; // returns pubkeys for which we are still missing signatures
    std::vector<bytes_t> presentsigs() const; // returns pubkeys for which we have signatures
    bool addsig(const bytes_t& pubkey, const bytes_t& sig); // returns true iff signature was absent and has been added
    void clearsigs(); // resets all signatures to 0-length placeholders
    unsigned int mergesigs(const SignableTxIn& other); // merges the signatures from another input that is otherwise identical. returns number of signatures added.

private:
    type_t type_;
    unsigned int minsigs_;
    std::vector<bytes_t> pubkeys_;
    std::vector<bytes_t> sigs_; // empty vectors for missing signatures
    bytes_t redeemscript_; // empty if type is not script hash
};

typedef std::vector<SignableTxIn> signabletxins_t;


class Signer
{
public:
    Signer() { }
    explicit Signer(const Coin::Transaction& tx, const std::vector<uint64_t>& outpointvalues = std::vector<uint64_t>()) { setTx(tx, outpointvalues); }

    void setTx(const Coin::Transaction& tx, const std::vector<uint64_t>& outpointvalues = std::vector<uint64_t>());

    const Coin::Transaction& getTx() const { return tx_; }

    // returns how many signatures are still needed
    unsigned int sigsneeded(std::size_t nIn) const;
    unsigned int sigsneeded() const;

    // returns pubkeys for which we are still missing signatures
    std::vector<bytes_t> missingsigs(std::size_t nIn) const;
    std::vector<bytes_t> missingsigs() const;

    // returns pubkeys for which we have signatures
    std::vector<bytes_t> presentsigs(std::size_t nIn) const;
    std::vector<bytes_t> presentsigs() const;

    // returns true iff signature was absent and has been added
    bool addsig(std::size_t nIn, const bytes_t& pubkey, const bytes_t& sig);
    bool addsig(const bytes_t& pubkey, const bytes_t& sig);

    void clearsigs(std::size_t nIn); // resets all signatures
    void clearsigs();

    // merges the signatures from another input that is otherwise identical.
    // returns number of signatures added.
    unsigned int mergesigs(std::size_t nIn, const Signer& other);
    unsigned int mergesigs(const Signer& other);

    const signabletxins_t& getSignableTxIns() const { return signabletxins_; }

private:
    Coin::Transaction tx_;
    signabletxins_t signabletxins_;
};

}
}

#endif // _COINQ_SCRIPT_H_
