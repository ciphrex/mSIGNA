#include <CoinCore/secp256k1_openssl.h>
#include <stdutils/uchar_vector.h>

#include <iostream>

using namespace CoinCrypto;
using namespace std;

const int SIGNATURE_FLAGS = SIGNATURE_ENFORCE_LOW_S;

int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        cerr << "# usage: " << argv[0] << " <data> <pubkey> <signature>" << endl;
        return -1;
    }

    try
    {
        uchar_vector data(argv[1]);
        uchar_vector pubkey(argv[2]);
        uchar_vector sig(argv[3]);

        secp256k1_key key;
        key.setPubKey(pubkey);

        if (secp256k1_verify(key, data, sig, SIGNATURE_FLAGS))  { cout << "valid" << endl; }
        else                                                    { cout << "invalid" << endl; }
    }
    catch (const exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return -2;
    }

    return 0;
}
