///////////////////////////////////////////////////////////////////////////////
//
// vault.cpp
//
// Copyright (c) 2013 Eric Lombrozo
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
