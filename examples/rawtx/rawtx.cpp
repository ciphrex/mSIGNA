///////////////////////////////////////////////////////////////////////////////
//
// rawtx.cpp
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

#include <uchar_vector.h>
#include <Base58Check.h>
#include <CoinKey.h>
#include <CoinNodeData.h>
#include <TransactionSigner.h>

#include <iostream>
#include <sstream>
#include <stdexcept>

#include <boost/regex.hpp>

using namespace std;
using namespace Coin;

string jsonError(const string& error)
{
    return string("{\"status\":\"error\",\"error\":\"") + error + "\"}";
}

string jsonTxHash(const Transaction& tx, bool bSize = false, bool bRawOut = false)
{
    stringstream ss;
    ss << "{\"status\":\"success\",\"tx_hash\":\"" << tx.getHashLittleEndian().getHex() << "\"";
    if (bSize) ss << ",\"tx_size\":" << tx.getSize();
    if (bRawOut) ss << ",\"tx_raw\":\"" << tx.getSerialized().getHex() << "\"";
    ss << "}";
    return ss.str();
} 

enum {
    SUCCESS_EXIT_CODE = 0,
    INVALID_NUM_PARAMS_EXIT_CODE,
    INVALID_KEY_EXIT_CODE,
    INVALID_CLAIM_EXIT_CODE,
    INVALID_ADDRESS_EXIT_CODE,
    UNRECOGNIZED_PARAM_EXIT_CODE,
    NO_INPUTS_EXIT_CODE,
    NO_OUTPUTS_EXIT_CODE,
};

class RawTxException : public runtime_error
{
private:
    int exitCode;
	
public:
    RawTxException(int _exitCode, const char* _description)
        : runtime_error(_description), exitCode(_exitCode) { }

    int getExitCode() const { return exitCode; }
};

void DisplayHelp(const string& progName)
{
    cout << endl;
    cout << " - Usage: " << progName << " [-size] [-rawout] [-rawin [tx]] \\" << endl
         << "                  -in [key] [claim] -in [key] [claim] ... \\" << endl
         << "                  -out [address] [amount] -out [address] [amount] ..." << endl << endl
         << "Keys can be either 32-byte ECDSA keys, 279-byte DER ECDSA keys, or 51-character base58 wallet import keys." << endl
         << "Claims are in the format txhash:index." << endl
         << "Addresses are standard base58 encoded bitcoin addresses" << endl
         << "Amounts are in satoshis." << endl << endl
         << "   -size   : will output the transaction size in bytes." << endl
         << "   -rawout : will output the raw transaction." << endl
         << "   -rawin  : will accept a raw transaction passed in as hex. The -in and -out parameters will be ignored." << endl << endl;
}

BasicInput GetInputFromCmd(int i, char* argv[])
{
    BasicInput input;
    string key = argv[i];
    if (key.size() == 51)
        input.walletImport = argv[i];
    else if ((key.size() == 64) || (key.size() == 558))
        input.privKey = uchar_vector(argv[i]);
    else {
        string error("Invalid key: ");
        error += argv[1];
        throw RawTxException(INVALID_KEY_EXIT_CODE, error.c_str());
    }
	
    boost::regex rxOutPoint("^([0-9a-fA-F]{64}):([0-9]+)$");
    boost::match_results<string::const_iterator> point;
    string outPointStr = argv[i+1];
    if (!boost::regex_search(outPointStr, point, rxOutPoint)) {
        string error("Invalid claim: ");
        error += argv[i+1];
        throw RawTxException(INVALID_CLAIM_EXIT_CODE, error.c_str());
    }
	
    input.outPoint = OutPoint(point[1], strtol(string(point[2]).c_str(), NULL, 10));
    input.sequence = 0xffffffff;
    return input;
}

void ShowInputs(const vector<BasicInput> inputs)
{
    cout << "Inputs:" << endl;
    for (unsigned int i = 0; i < inputs.size(); i++) {
        if (i > 0) cout << endl;
        cout << "  Key: " << ((inputs[i].walletImport != "") ? inputs[i].walletImport : inputs[i].privKey.getHex()) << endl;
        cout << "  OutPoint: " << inputs[i].outPoint.toString() << endl;
    }
}

BasicOutput GetOutputFromCmd(int i, char* argv[])
{
    if (!isBase58CheckValid(argv[i]) || (argv[i][0] != '1')) {
        string error("Invalid address: ");
        error += argv[i];
        throw RawTxException(INVALID_ADDRESS_EXIT_CODE, error.c_str());
    }
	
    boost::regex rxBtcAmount("^([0-9]*)\\.{0,1}([0-9]+){0,8}$");
    boost::match_results<string::const_iterator> decimalParts;
    string amount = argv[i+1];
    uint64_t satoshis = strtoll(amount.c_str(), NULL, 10);
	
    return BasicOutput(argv[i], satoshis);
}

int main(int argc, char* argv[])
{
    try {
        if ((argc < 2) || (string(argv[1]) == "-help")) {
            DisplayHelp(argv[0]);
            return 0;
        }
		
        int firstInputPos = 1;

        bool bSize = (string(argv[firstInputPos]) == "-size");
        if (bSize) firstInputPos++;
        if (firstInputPos >= argc)
            throw RawTxException(INVALID_NUM_PARAMS_EXIT_CODE, "Invalid number of parameters.");
		
        bool bRawOut = (string(argv[firstInputPos]) == "-rawout");
        if (bRawOut) firstInputPos++;
        if (firstInputPos >= argc)
            throw RawTxException(INVALID_NUM_PARAMS_EXIT_CODE, "Invalid number of parameters.");
		
        bool bRawIn = (string(argv[firstInputPos]) == "-rawin");
        if (bRawIn) firstInputPos++;
        if (firstInputPos >= argc)
        throw RawTxException(INVALID_NUM_PARAMS_EXIT_CODE, "Invalid number of parameters.");
		
        Transaction tx;
        if (bRawIn) {
            tx.setSerialized(uchar_vector(argv[firstInputPos]));
        }
        else {
            if ((argc-firstInputPos) % 3 != 0)
                throw RawTxException(INVALID_NUM_PARAMS_EXIT_CODE, "Invalid number of parameters.");
			
            vector<BasicInput> inputs;
            vector<BasicOutput> outputs;
            for (int i = firstInputPos; i < argc; i++) {
                if (string(argv[i]) == "-in") {
                    inputs.push_back(GetInputFromCmd(i + 1, argv));
                    i += 2;
                }
                else if (string(argv[i]) == "-out") {
                    outputs.push_back(GetOutputFromCmd(i + 1, argv));
                    i += 2;
                }
                else {
                    string error("Unrecognized parameter: ");
                    error += argv[i];
                    throw RawTxException(UNRECOGNIZED_PARAM_EXIT_CODE, error.c_str());
                }
            }
            if (inputs.size() == 0)
                throw RawTxException(NO_INPUTS_EXIT_CODE, "Transaction needs at least one input.");
            if (outputs.size() == 0)
                throw RawTxException(NO_OUTPUTS_EXIT_CODE, "Transaction needs at least one output.");
			
            tx = CreateTransaction(inputs, outputs);
            cout << jsonTxHash(tx, bSize, bRawOut) << endl;
            return SUCCESS_EXIT_CODE;
        }
    }
    catch (const RawTxException& e) {
        cout << jsonError(e.what()) << endl;
        return e.getExitCode();
    }
}
