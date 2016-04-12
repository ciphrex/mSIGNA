#include <iostream>
#include <bip39.h>

using namespace Coin::BIP39;
using namespace std;

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        cerr << "# Usage: " << argv[0] << " <wordlist>" << endl;
        return -1;
    }

    secure_string_t wordlist(argv[1]);
    try
    {
        secure_bytes_t data = fromWordlist(wordlist);
        cout << uchar_vector(data).getHex() << endl;
    }
    catch (const exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return -2;
    }

    return 0;
}
