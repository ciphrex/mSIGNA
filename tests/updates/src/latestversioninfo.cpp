///////////////////////////////////////////////////////////////////
//
// latestversioninfo.cpp
//
// Unit test for LatestVersionInfo class
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include <updates.h>
#include <iostream>

using namespace std;

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        cerr << "# Usage: " << argv[0] << " <url>" << endl;
        return -1;
    }

    try
    {
        LatestVersionInfo info;
        info.download(argv[1]);
    }
    catch (const exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return -2;
    }

    return 0;
}

