///////////////////////////////////////////////////////////////////////////////
//
// cli.hpp
//
// Copyright (c) 2011-2014 Eric Lombrozo
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

#pragma once

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <sstream>
#include <iostream>

#include <stdarg.h>

namespace cli
{

typedef std::vector<std::string>    params_t;
typedef std::string                 result_t;
typedef result_t                    (*fAction)(const params_t&);

class command
{
public:
    command(fAction cmdfunc, const std::string& cmdname, const std::string& cmddesc, const params_t& reqparams = params_t(), const params_t& optparams = params_t()) : cmdfunc_(cmdfunc), cmdname_(cmdname), cmddesc_(cmddesc), reqparams_(reqparams), optparams_(optparams), ellipsescount_(0)
    {
        for (auto& optparam: optparams_)
            if (optparam == "...") ellipsescount_++;
    }

    static params_t params(int count, ...)
    {
        va_list ap;
        va_start (ap, count);
        params_t params;
        for (int i = 0; i < count; i++) { params.push_back(va_arg (ap, const char*)); }
        va_end (ap);
        return params;
    }

    result_t operator()(const params_t& params) const { return cmdfunc_(params); }

    const std::string& getName() const { return cmdname_; }
    bool isValidParamCount(const params_t& params) const
    {
        std::size_t nParams = params.size();
        std::size_t min = reqparams_.size();
        return nParams >= min && (ellipsescount_ || nParams <= min + optparams_.size());
    }

    std::string getHelpTemplate() const
    {
        std::stringstream ss;
        ss << cmdname_;
        for (auto& param: reqparams_) { ss << " <" << param << ">"; }
        for (auto& param: optparams_)
        {
            if (param != "...")       { ss << " [" << param << "]"; }
            else                      { ss << " ...";               }
        }
        return ss.str();
    }

    unsigned int getMinHelpTab(unsigned int mintab = 0) const
    {
        unsigned int tab = cmdname_.size() + 1;
        for (auto& param: reqparams_) { tab += param.size() + 3; }
        for (auto& param: optparams_) { tab += param.size() + 3; }
        tab -= 2 * ellipsescount_;
        return tab > mintab ? tab : mintab;
    }
 
    std::string getHelpInfo(unsigned int tab) const
    {
        std::stringstream ss;
        ss << std::left << std::setw(tab) << getHelpTemplate() << cmddesc_;
        return ss.str();
    }

private:
    fAction cmdfunc_;
    std::string cmdname_;
    std::string cmddesc_;
    params_t reqparams_;
    params_t optparams_;
    unsigned int ellipsescount_;
};

class Shell
{
public:
    Shell(const std::string& proginfo) : proginfo_(proginfo), tab(0) { }

    void clear() { command_map_.clear(); tab = 0; }
    void add(const command& cmd)
    {
        command_map_.insert(std::pair<std::string, command>(cmd.getName(), cmd));
        tab = cmd.getMinHelpTab(tab);
    }
    result_t exec(const std::string& cmdname, const params_t& params);
    int exec(int argc, char** argv);

private:
    std::string proginfo_;

    typedef std::map<std::string, command>  command_map_t;
    command_map_t command_map_;
    unsigned int tab;

    result_t help();
};

inline result_t Shell::exec(const std::string& cmdname, const params_t& params)
{
    if (cmdname == "help")
    {
        return help();
    }

    command_map_t::iterator it = command_map_.find(cmdname);
    if (it == command_map_.end()) {
        std::stringstream ss;
        ss << "Invalid command " << cmdname << ".";
        throw std::runtime_error(ss.str());
    }

    bool bHelp = (params.size() == 1 && (params[0] == "-h" || params[0] == "--help"));

    if (!bHelp && it->second.isValidParamCount(params))
    {
        return it->second(params);
    }
    else
    {
        return it->second.getHelpInfo(it->second.getMinHelpTab() + 4);
    }
}

inline int Shell::exec(int argc, char** argv)
{
    if (argc == 1) {
        std::cout << proginfo_ << std::endl;
        std::cout << "Use " << argv[0] << " --help for list of commands." << std::endl;
        return 0;
    }

    std::string cmdname(argv[1]);

    try {
        if (cmdname == "-h" || cmdname == "--help")
        {
            std::cout << help() << std::endl;
            return 0;
        }

        command_map_t::iterator it = command_map_.find(cmdname);
        if (it == command_map_.end()) {
            std::stringstream ss;
            ss << "Invalid command " << cmdname << ".";
            throw std::runtime_error(ss.str());
        }

        params_t params;
        for (int i = 2; i < argc; i++) {
            params.push_back(argv[i]);
        }

        bool bHelp = (params.size() == 1 && (params[0] == "-h" || params[0] == "--help"));

        if (!bHelp && it->second.isValidParamCount(params))
        {
            std::cout << it->second(params) << std::endl;
        }
        else
        {
            std::cout << it->second.getHelpInfo(it->second.getMinHelpTab() + 4) << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}

inline result_t Shell::help()
{
    params_t params;
    std::stringstream out;
    out << "List of commands:";
    command_map_t::iterator it = command_map_.begin();
    for (; it != command_map_.end(); ++it) {
        out << std::endl << "  " << it->second.getHelpInfo(tab + 4);
    }
    return out.str();
}

}

