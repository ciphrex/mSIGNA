///////////////////////////////////////////////////////////////////
//
// filesystemtest.cpp
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.

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
