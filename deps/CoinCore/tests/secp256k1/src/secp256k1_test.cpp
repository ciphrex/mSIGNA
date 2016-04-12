#include <CoinCore/secp256k1_openssl.h>
#include <stdutils/uchar_vector.h>

#include <iostream>

#include <string>

using namespace CoinCrypto;
using namespace std;

const int SIGNATURE_FLAGS = SIGNATURE_ENFORCE_LOW_S;

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        cerr << "# usage: " << argv[0] << " <hex data to sign>" << endl;
        return -1;
    }

    try
    {
        uchar_vector data(argv[1]);
        cout << "Using data " << data.getHex() << endl;

        cout << endl << "Creating new key..." << flush; 
        secp256k1_key key;
        key.newKey();
        cout << "done." << endl;

        uchar_vector privkey = key.getPrivKey();
        cout << "Private key: " << privkey.getHex() << endl;

        uchar_vector pubkey = key.getPubKey();
        cout << "Public key: " << pubkey.getHex() << endl;

        cout << endl << "Signing data..." << flush;
        uchar_vector sig = secp256k1_sign(key, data);
        cout << "done." << endl;
        cout << "Signature: " << sig.getHex() << endl;

        cout << endl << "Verifying signature (should be valid)..." << flush;
        if (secp256k1_verify(key, data, sig, SIGNATURE_FLAGS))  { cout << "valid." << endl; }
        else                                    { cout << "invalid. TEST FAILED" << endl; }

        cout << endl << "Creating public key object..." << flush;
        secp256k1_key key2;
        key2.setPubKey(pubkey);
        cout << "done." << endl;

        try
        {
            cout << "Trying to get private key (should fail)..." << flush;
            privkey = key2.getPrivKey();
            cout << "done." << endl;
            cout << "Private key: " << privkey.getHex() << " TEST FAILED" << endl;
        }
        catch (const exception& e)
        {
            cout << "failed - " << e.what() << endl;
        }

        pubkey = key2.getPubKey();
        cout << "Public key: " << pubkey.getHex() << endl;

        cout << endl << "Verifying signature (should be valid)..." << flush;
        if (secp256k1_verify(key2, data, sig, SIGNATURE_FLAGS))  { cout << "valid." << endl; }
        else                                    { cout << "invalid. TEST FAILED" << endl; }

        cout << endl << "Creating new key..." << flush;
        key.newKey();
        cout << "done." << endl;

        privkey = key.getPrivKey();
        cout << "Private key: " << privkey.getHex() << endl;

        pubkey = key.getPubKey();
        cout << "Public key: " << pubkey.getHex() << endl;
        
        cout << endl << "Verifying old signature with new key (should be invalid)..." << flush;
        if (secp256k1_verify(key, data, sig, SIGNATURE_FLAGS))   { cout << "valid. TEST FAILED" << endl; }
        else                                    { cout << "invalid." << endl; }
    }
    catch (const exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return -2;
    }

    return 0;
}
