#include <iostream>

#include <bip39.h>
#include <uchar_vector.h>

using namespace Coin::BIP39;
using namespace std;

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        cerr << "# Usage: " << argv[0] << " <data hex>" << endl;
        return -1;
    }

    secure_bytes_t data = uchar_vector(string(argv[1]));

    try
    {
        secure_string_t wordlist = toWordlist(data);
        cout << wordlist << endl;
    }
    catch (const exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return -2;
    }

    return 0;
}
