///////////////////////////////////////////////////////////////////
//
// urlparser.cpp
//
// Unit test for UrlParser class
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.

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
        UrlParser urlParser(argv[1]);
        cout << "Service: \"" << urlParser.getService() << "\"" << endl;
        cout << "Host:    \"" << urlParser.getHost() << "\"" << endl;
        cout << "Path:    \"" << urlParser.getPath() << "\"" << endl;
    }
    catch (const exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return -2;
    }

    return 0;
}

