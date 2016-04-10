#include <MerkleTree.h>

#include <iostream>

using namespace Coin;
using namespace std;

int main()
{
    try
    {
        vector<uchar_vector> txHashes;
        txHashes.push_back(uchar_vector("0123456789012345678901234567890123456789012345678901234567890123").getReverse());
        txHashes.push_back(uchar_vector("1234567890123456789012345678901234567890123456789012345678901234").getReverse());
        txHashes.push_back(uchar_vector("2345678901234567890123456789012345678901234567890123456789012345").getReverse());;
        txHashes.push_back(uchar_vector("3456789012345678901234567890123456789012345678901234567890123456").getReverse());

        PartialMerkleTree tree = randomPartialMerkleTree(txHashes, 10);
        cout << tree.toIndentedString() << endl;
    }
    catch (const exception& e)
    {
        cout << "Exception: " << e.what() << endl;
        return 1;
    }

    return 0;
}
