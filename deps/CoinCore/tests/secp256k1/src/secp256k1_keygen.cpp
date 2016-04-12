#include <iostream>
#include <stdexcept>

#include <CoinCore/secp256k1_openssl.h>
#include <stdutils/uchar_vector.h>

using namespace CoinCrypto;
using namespace std;

int main()
{
    try
    {
        secp256k1_key key;
        key.newKey();

        uchar_vector privkey = key.getPrivKey();
        uchar_vector pubkey = key.getPubKey();

        cout << "Private key: " << privkey.getHex() << endl;
        cout << "Public key:  " << pubkey.getHex() << endl;
    }
    catch (const exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return -1;
    }

    return 0;
}
