#include <iostream>
#include <cassert>

#include <hdkeys.h>
#include <Base58Check.h>

#include <sstream>

using namespace Coin;
using namespace std;

int main(int argc, char* argv[])
{
    if (argc < 3) {
        cout << "Usage: " << argv[0] << " <extended key> <child num> [extended = 0]" << endl;
        return 0;
    }

    try {
        uchar_vector extkey;
        if (!fromBase58Check(argv[1], extkey))
            throw runtime_error("Invalid extended key.");

        uint32_t childnum = strtoull(argv[2], NULL, 0);

        bool bExtended = argc > 3 && strtol(argv[3], NULL, 0);

        HDKeychain hdkeychain(extkey);
        hdkeychain = hdkeychain.getChild(childnum);

        if (bExtended)
        {
            cout << toBase58Check(hdkeychain.extkey()) << endl;
        }
        else
        {
            cout << uchar_vector(hdkeychain.key()).getHex() << endl;
        }
    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return -1;
    }
 
    return 0;
}
