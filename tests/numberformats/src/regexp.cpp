///////////////////////////////////////////////////////////////////////////////
//
// regexp.cpp
//
// numberformats regexp test
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include <numberformats.h>
#include <iostream>

using namespace std;

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        cerr << "Usage: " << argv[0] << "<maxValue> <maxDecimals>\n";
        return -1;
    }

    uint64_t maxValue = strtoull(argv[1], NULL, 10);
    uint64_t maxDecimals = strtoull(argv[2], NULL, 10);
    cout << getDecimalRegExpString(maxValue, maxDecimals) << endl;
    return 0;
}
