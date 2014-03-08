///////////////////////////////////////////////////////////////////////////////
//
// coindb.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.
//

#include <cli.hpp>

#include <iostream>

int main(int argc, char* argv[])
{
    cli::command_map cmds("CoinDB by Eric Lombrozo v0.2.0");
    try 
    {
        return cmds.exec(argc, argv);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}

