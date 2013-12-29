///////////////////////////////////////////////////////////////////////////////
//
// vault_commands.h
//
// Copyright (c) 2011-2013 Eric Lombrozo
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

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
