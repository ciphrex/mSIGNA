///////////////////////////////////////////////////////////////////////////////
//
// vault.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include <command_interpreter.h>
#include "vault_commands.h"

int main(int argc, char** argv)
{
    initCommands();
    addCommand("newkey", &vault_newkey);
    addCommand("pubkey", &vault_pubkey);
    addCommand("addr", &vault_addr);
    addCommand("newwallet", &vault_newwallet);
    addCommand("importkeys", &vault_importkeys);
    addCommand("dumpkeys", &vault_dumpkeys);
    addCommand("walletkey", &vault_walletkey);
    addCommand("walletsign", &vault_walletsign);
    return startInterpreter(argc, argv);
}
