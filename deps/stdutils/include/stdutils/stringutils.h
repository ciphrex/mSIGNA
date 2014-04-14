///////////////////////////////////////////////////////////////////////////////
//
// stringutils.h
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.
//

#pragma once

#include <vector>
#include <string>

namespace stdutils
{

inline std::string delimited_list(const std::vector<std::string>& items, const std::string& delimiter)
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

}
