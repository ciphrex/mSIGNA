#include <CoinCore/secp256k1_openssl.h>
#include <CoinCore/hash.h>
#include <stdutils/uchar_vector.h>

#include <iostream>

#include <string>

using namespace CoinCrypto;
using namespace std;

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        cerr << "# usage: " << argv[0] << " <privkey hex> <data hex>" << endl;
        return -1;
    }

    try
    {
	uchar_vector privkey(argv[1]);
	if (privkey.size() > 32) throw runtime_error("Key is too long.");
	while (privkey.size() < 32)
	{
            privkey = uchar_vector("00") + privkey;
        }

        cout << endl << "Creating key..." << flush; 
        secp256k1_key key;
        key.setPrivKey(privkey);
        cout << "done.";
        cout << endl << "Using privkey " << uchar_vector(key.getPrivKey()).getHex() << endl;

        uchar_vector data(argv[2]);
        cout << "Using data " << data.getHex() << endl;
        cout << "Data hash: " << sha256(data).getHex() << endl;

        uchar_vector k = secp256k1_rfc6979_k(key, data);
        cout << "k = " << k.getHex() << endl;

        cout << "Signing..." << endl;
        uchar_vector sig = secp256k1_sign_rfc6979(key, data);
        cout << "Signature: " << sig.getHex() << endl;
        cout << "Valid: " << (secp256k1_verify(key, data, sig) ? "TRUE" : "FALSE") << endl << endl;
    }
    catch (const exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return -2;
    }

    return 0;
}
