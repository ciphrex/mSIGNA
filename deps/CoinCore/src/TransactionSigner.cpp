////////////////////////////////////////////////////////////////////////////////
//
// TransactionSigner.cpp
//
// Copyright (c) 2011-2012 Eric Lombrozo
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "TransactionSigner.h"
#include "CoinNodeData.h"
#include "Base58Check.h"
#include "CoinKey.h"

using namespace Coin;

Transaction CreateTransaction(
    const std::vector<BasicInput>& claims,
    const std::vector<BasicOutput>& payments,
    uint32_t lockTime,
    uint8_t addressVersion
)
{
    Transaction tx;

    uchar_vector prefix;
    prefix.push_back(0x76);
    prefix.push_back(0xa9);
    prefix.push_back(0x14);

    uchar_vector suffix;
    suffix.push_back(0x88);
    suffix.push_back(0xac);

    // add outputs
    for (uint i = 0; i < payments.size(); i++) {
        uchar_vector toPubKeyHash;
        uint version;
        if (!fromBase58Check(payments[i].toAddress, toPubKeyHash, version))
            throw std::runtime_error("Invalid address checksum.");
        if (version != addressVersion)
        throw std::runtime_error("Invalid address version.");

        TxOut txout(payments[i].amount, prefix + toPubKeyHash + suffix);
        tx.addOutput(txout);
    }

    // add input outpoints and sequences
    for (uint i = 0; i < claims.size(); i++) {
        TxIn txin(claims[i].outPoint, "", claims[i].sequence);
        tx.addInput(txin);
    }

    // compute signature for each input
    std::vector<uchar_vector> signatures;
    std::vector<uchar_vector> pubKeys;
    for (uint i = 0; i < claims.size(); i++) {
        CoinKey key;
        if (claims[i].walletImport != "") {
            if (!key.setWalletImport(claims[i].walletImport))
                throw std::runtime_error("Invalid wallet import key.");
        }
        else if (!key.setPrivateKey(claims[i].privKey))
            throw std::runtime_error("Error setting private key.");

        uchar_vector fromPubKey = key.getPublicKey();
        uchar_vector fromPubKeyHash = ripemd160(sha256(fromPubKey));

        tx.clearScriptSigs();
        tx.setScriptSig(i, prefix + fromPubKeyHash + suffix);

        uchar_vector signature;
        if (!key.sign(tx.getHashWithAppendedCode(1), signature))
            throw std::runtime_error("Error trying to sign.");

        signatures.push_back(signature);
        pubKeys.push_back(fromPubKey);
    }

    // add signatures to transaction
    for (uint i = 0; i < claims.size(); i++) {
        uchar_vector scriptSig;
        scriptSig.push_back(signatures[i].size() + 1);
        scriptSig += signatures[i];
        scriptSig.push_back(1); // hash type byte
        scriptSig.push_back(pubKeys[i].size());
        scriptSig += pubKeys[i];

        tx.setScriptSig(i, scriptSig);
    }

    return tx;
}

void SignTransaction(const std::vector<BasicInput>& claims, Transaction& tx)
{
    uchar_vector prefix;
    prefix.push_back(0x76);
    prefix.push_back(0xa9);
    prefix.push_back(0x14);

    uchar_vector suffix;
    suffix.push_back(0x88);
    suffix.push_back(0xac);

    // add input outpoints and sequences
    tx.clearInputs();
    for (uint i = 0; i < claims.size(); i++) {
        TxIn txin(claims[i].outPoint, "", claims[i].sequence);
        tx.addInput(txin);
    }
    // compute signature for each input
    std::vector<uchar_vector> signatures;
    std::vector<uchar_vector> pubKeys;
    for (uint i = 0; i < claims.size(); i++) {
        CoinKey key;
        if (claims[i].walletImport != "") {
            if (!key.setWalletImport(claims[i].walletImport))
                throw std::runtime_error("Invalid wallet import key.");
        }
        else if (!key.setPrivateKey(claims[i].privKey))
            throw std::runtime_error("Error setting private key.");

        uchar_vector fromPubKey = key.getPublicKey();
        uchar_vector fromPubKeyHash = ripemd160(sha256(fromPubKey));

        tx.clearScriptSigs();
        tx.setScriptSig(i, prefix + fromPubKeyHash + suffix);

        uchar_vector signature;
        if (!key.sign(tx.getHashWithAppendedCode(1), signature))
            throw std::runtime_error("Error trying to sign.");

        signatures.push_back(signature);
        pubKeys.push_back(fromPubKey);
    }

    // add signatures to transaction
    for (uint i = 0; i < claims.size(); i++) {
        uchar_vector scriptSig;
        scriptSig.push_back(signatures[i].size() + 1);
        scriptSig += signatures[i];
        scriptSig.push_back(1); // hash type byte
        scriptSig.push_back(pubKeys[i].size());
        scriptSig += pubKeys[i];

        tx.setScriptSig(i, scriptSig);
    }
}
