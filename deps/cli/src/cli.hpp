///////////////////////////////////////////////////////////////////////////////
//
// cli.hpp
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

#ifndef _INTERPRETER__HPP_
#define _INTERPRETER__HPP_

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <sstream>
#include <iostream>

namespace cli {

typedef std::vector<std::string>    params_t;
typedef std::string                 result_t;
typedef result_t                    (*fAction)(bool, const params_t&);

class command_map
{
public:
    command_map(const std::string& proginfo) : proginfo_(proginfo) { }

    void clear() { command_map_.clear(); }
    void add(const std::string& cmdname, fAction cmdfunc) { command_map_[cmdname] = cmdfunc; }
    int exec(int argc, char** argv);

private:
    std::string proginfo_;

    typedef std::map<std::string, fAction>  command_map_t;
    command_map_t command_map_;

    void help();
};

inline int command_map::exec(int argc, char** argv)
{
    if (argc == 1) {
        std::cout << proginfo_ << std::endl;
        std::cout << "Use " << argv[0] << " --help for list of commands." << std::endl;
        return 0;
    }

    std::string command(argv[1]);

    try {
        if (command == "-h" || command == "--help") {
            help();
            return 0;
        }

        params_t params;
        for (int i = 2; i < argc; i++) {
            params.push_back(argv[i]);
        }

        command_map_t::iterator it = command_map_.find(command);
        if (it == command_map_.end()) {
            std::stringstream ss;
            ss << "Invalid command " << command << ".";
            throw std::runtime_error(ss.str());
        }

        bool bHelp = (params.size() == 1 && (params[0] == "-h" || params[0] == "--help"));
        std::cout << it->second(bHelp, params) << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}

inline void command_map::help()
{
    params_t params;
    std::stringstream out;
    out << "List of commands:";
    command_map_t::iterator it = command_map_.begin();
    for (; it != command_map_.end(); ++it) {
        out << std::endl << it->second(true, params);
    }
    std::cout << out.str() << std::endl;
}

}

#endif // _INTERPRETER__HPP_

