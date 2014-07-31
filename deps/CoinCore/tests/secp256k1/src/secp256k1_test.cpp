#include <CoinCore/secp256k1.h>
#include <stdutils/uchar_vector.h>

#include <iostream>

#include <string>

using namespace CoinCrypto;
using namespace std;

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

        cout << "Creating new key..." << flush; 
        secp256k1_key key;
        key.newKey();
        cout << "done." << endl;
    }
    catch (const exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return -2;
    }

    return 0;
}
