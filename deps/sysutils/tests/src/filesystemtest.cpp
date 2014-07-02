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
    cout << getDefaultDataDir("appname") << endl;
    return 0;    
}
