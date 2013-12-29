std::string standardtxout(bool bHelp, params_t& params)
{
    if (bHelp || params.size() < 2 || params.size() > 3) {
        return "standardtxout <address> <value> [options] - creates a standard transaction output. Use option -h for serialized hex.";
    }

    bool bHex = false;
    if (params.size() == 3) {
        if (params[2] == "-h") {
            bHex = true;
        }
        else {
            throw std::runtime_error(std::string("Invalid option: ") + params[2]);
        }
    }

    StandardTxOut txOut;
    txOut.set(params[0], strtoull(params[1].c_str(), NULL, 10));

    if (bHex) {
        return txOut.getSerialized().getHex();
    }
    else {
        return txOut.toJson();
    }
}

std::string createtransaction(bool bHelp, params_t& params)
{
    if (bHelp || params.size() != 5) {
        return "createtransaction <outhash> <outindex> <redeemscript> <toaddress> <value> - creates a transaction claiming a multisignature input.";
    }

    uchar_vector outHash = params[0];
    uint outIndex = strtoul(params[1].c_str(), NULL, 10);
    uchar_vector redeemScript = params[2];
    std::string toAddress = params[3];
    uint64_t value = strtoull(params[4].c_str(), NULL, 10);

    StandardTxOut txOut;
    txOut.set(toAddress, value);

    MultiSigRedeemScript multiSig;
    multiSig.parseRedeemScript(redeemScript);

    P2SHTxIn txIn(outHash, outIndex, multiSig.getRedeemScript());
    txIn.setScriptSig(SCRIPT_SIG_SIGN);

    Transaction tx;
    tx.addOutput(txOut);
    tx.addInput(txIn);
    uchar_vector hashToSign = tx.getHashWithAppendedCode(1); // SIGHASH_ALL

    for (uint i = 0; i < multiSig.getPubKeyCount(); i++) {
        txIn.addSig(uchar_vector(), uchar_vector(), SIGHASH_ALREADYADDED);
    }

    txIn.setScriptSig(SCRIPT_SIG_EDIT);
    tx.clearInputs();
    tx.addInput(txIn);
    return tx.getSerialized().getHex();
}

std::string signtransaction(bool bHelp, params_t& params)
{
    if (bHelp || params.size() < 6) {
        return "signtransaction <outhash> <outindex> <redeemscript> <toaddress> <value> <privkey1> [<privkey2> <privkey3> ...] - creates and signs a transaction claiming a multisignature input.";
    }

    uchar_vector outHash = params[0];
    uint outIndex = strtoul(params[1].c_str(), NULL, 10);
    uchar_vector redeemScript = params[2];
    std::string toAddress = params[3];
    uint64_t value = strtoull(params[4].c_str(), NULL, 10);

    std::vector<std::string> privKeys;
    for (uint i = 5; i < params.size(); i++) {
        privKeys.push_back(params[i]);
    }

    StandardTxOut txOut;
    txOut.set(toAddress, value);

    MultiSigRedeemScript multiSig;
    multiSig.parseRedeemScript(redeemScript);

    P2SHTxIn txIn(outHash, outIndex, multiSig.getRedeemScript());
    txIn.setScriptSig(SCRIPT_SIG_SIGN);

    Transaction tx;
    tx.addOutput(txOut);
    tx.addInput(txIn);
    uchar_vector hashToSign = tx.getHashWithAppendedCode(1); // SIGHASH_ALL

    // TODO: make sure to wipe all key data if there's any failure
    CoinKey key;
    for (uint i = 0; i < privKeys.size(); i++) {
        if (!key.setWalletImport(privKeys[i])) {
            std::stringstream ss;
            ss << "Private key " << i+1 << " is invalid.";
            throw std::runtime_error(ss.str());
        }

        uchar_vector sig;
        if (!key.sign(hashToSign, sig)) {
            std::stringstream ss;
            ss << "Error signing with key " << i+1 << ".";
            throw std::runtime_error(ss.str());
        }
        txIn.addSig(uchar_vector(), sig);
    }

    if (privKeys.size() < multiSig.getMinSigs()) {
        txIn.setScriptSig(SCRIPT_SIG_EDIT);
    }
    else {
        txIn.setScriptSig(SCRIPT_SIG_BROADCAST);
    }
    tx.clearInputs();
    tx.addInput(txIn);
    return tx.getSerialized().getHex();
}

std::string signmofn(bool bHelp, params_t& params)
{
    if (bHelp || params.size() < 6) {
        return "signmofn <outhash> <outindex> <redeemscript> <toaddress> <value> <privkey1> [<privkey2> <privkey3> ...] - creates and signs a transaction claiming a multisignature input.";
    }

    uchar_vector outHash = params[0];
    uint outIndex = strtoul(params[1].c_str(), NULL, 10);
    uchar_vector redeemScript = params[2];
    std::string toAddress = params[3];
    uint64_t value = strtoull(params[4].c_str(), NULL, 10);

    std::vector<std::string> privKeys;
    for (uint i = 5; i < params.size(); i++) {
        privKeys.push_back(params[i]);
    }

    StandardTxOut txOut;
    txOut.set(toAddress, value);
/*
    MultiSigRedeemScript multiSig;
    multiSig.parseRedeemScript(redeemScript);
*/
    MofNTxIn txIn(outHash, outIndex, redeemScript);
    txIn.setScriptSig(SCRIPT_SIG_SIGN);
/*
    P2SHTxIn txIn(outHash, outIndex, multiSig.getRedeemScript());
    txIn.scriptSig = multiSig.getRedeemScript();
*/

    Transaction tx;
    tx.addOutput(txOut);
    tx.addInput(txIn);
    uchar_vector hashToSign = tx.getHashWithAppendedCode(1); // SIGHASH_ALL

    // TODO: make sure to wipe all key data if there's any failure
    CoinKey key;
    for (uint i = 5; i < params.size(); i++) {
        if (!key.setWalletImport(params[i])) {
            std::stringstream ss;
            ss << "Private key " << i+1 << " is invalid.";
            throw std::runtime_error(ss.str());
        }

        uchar_vector sig;
        if (!key.sign(hashToSign, sig)) {
            std::stringstream ss;
            ss << "Error signing with key " << i+1 << ".";
            throw std::runtime_error(ss.str());
        }
        txIn.addSig(key.getPublicKey(), sig);
    }

    txIn.setScriptSig(SCRIPT_SIG_BROADCAST);
    tx.clearInputs();
    tx.addInput(txIn);
    return tx.getSerialized().getHex();
}

