///////////////////////////////////////////////////////////////////////////////
//
// coindb.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.
//

#include <cli.hpp>

#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <Vault.h>
#include <Schema-odb.hxx>

#include <iostream>
#include <sstream>

using namespace odb::core;
using namespace CoinDB;

cli::result_t cmd_create(bool bHelp, const cli::params_t& params)
{
    if (bHelp || params.size() != 1) {
        return "create <filename> - create a new vault.";
    }

    Vault vault(params[0], true);

    std::stringstream ss;
    ss << "Vault " << params[0] << " created.";
    return ss.str();
}

int main(int argc, char* argv[])
{
    cli::command_map cmds("CoinDB by Eric Lombrozo v0.2.0");
    cmds.add("create", &cmd_create);

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

