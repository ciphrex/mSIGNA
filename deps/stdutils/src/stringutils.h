///////////////////////////////////////////////////////////////////////////////
//
// stringutils.h
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.
//

#pragma once

#include <string>
#include <sstream>

namespace stdutils
{

template<class C>
inline std::string delimited_list(const C& items, const std::string& delimiter)
{
    std::string rval;
    bool addDelimiter = false;
    for (auto& item: items)
    {
        if (addDelimiter)   rval += delimiter;
        else                addDelimiter = true;

        rval += item;
    }
    return rval;
}

template<class OutIt>
inline void explode(const std::string& input, char delimiter, OutIt output)
{
    std::istringstream buffer(input);
    std::string item;
    while (std::getline(buffer, item, delimiter)) { *output++ = item; }
}

}
