///////////////////////////////////////////////////////////////////////////////
//
// txbuilder_commands.h
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

#ifndef TXBUILDER_COMMANDS_H

#include <string>
#include <vector>
#include <map>

typedef std::vector<std::string>        params_t;
typedef std::string                     (*fAction)(bool, params_t&);
typedef std::map<std::string, fAction>  command_map_t;

std::string help(bool bHelp, params_t& params);
std::string newtx(bool bHelp, params_t& params);
std::string createmultisig(bool bHelp, params_t& params);
std::string parsemultisig(bool bHelp, params_t& params);
std::string addoutput(bool bHelp, params_t& params);
std::string removeoutput(bool bHelp, params_t& params);
std::string addaddressinput(bool bHelp, params_t& params);
std::string addmofninput(bool bHelp, params_t& params);
std::string adddeps(bool bHelp, params_t& params);
std::string listdeps(bool bHelp, params_t& params);
std::string stripdeps(bool bHelp, params_t& params);
std::string addinput(bool bHelp, params_t& params);
std::string removeinput(bool bHelp, params_t& params);
std::string sign(bool bHelp, params_t& params);
std::string getmissingsigs(bool bHelp, params_t& params);
std::string getbroadcast(bool bHelp, params_t& params);
std::string getsign(bool bHelp, params_t& params);

#endif //TXBUILDER_COMMANDS_H
