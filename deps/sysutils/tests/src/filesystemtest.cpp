///////////////////////////////////////////////////////////////////
//
// filesystemtest.cpp
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include <filesystem.h>
#include <iostream>

using namespace std;

int main()
{
    namespace sufs = sysutils::filesystem;
    cout << "User profile dir: " << sufs::getUserProfileDir() << endl;
    cout << "Default data dir: " << sufs::getDefaultDataDir("<appname>") << endl;
    return 0;    
}
