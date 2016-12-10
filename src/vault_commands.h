///////////////////////////////////////////////////////////////////////////////
//
// vault_commands.h
//
// Copyright (c) 2011-2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#ifndef VAULT_COMMANDS_H
#define VAULT_COMMANDS_H

#include <command_interpreter.h>

result_t vault_newkey(bool, const params_t&);
result_t vault_pubkey(bool, const params_t&);
result_t vault_addr(bool, const params_t&);
result_t vault_newwallet(bool, const params_t&);
result_t vault_importkeys(bool, const params_t&);
result_t vault_dumpkeys(bool, const params_t&);
result_t vault_walletkey(bool, const params_t&);
result_t vault_walletsign(bool, const params_t&);

#endif // VAULT_COMMANDS_H
