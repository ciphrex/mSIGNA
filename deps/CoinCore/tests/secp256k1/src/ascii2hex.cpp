#include <stdutils/uchar_vector.h>
#include <CoinCore/secp256k1_openssl.h>

#include <iostream>

using namespace std;

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        cerr << "# Usage: " << argv[0] << " <text>" << endl;
        return -1;
    }

    bytes_t data((unsigned char*)&argv[1][0], (unsigned char*)&argv[1][0] + strlen(argv[1]));
    cout << uchar_vector(data).getHex();
    return 0;
}
