///////////////////////////////////////////////////////////////////
//
// filesystem.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

#include <string>

namespace sysutils {
    namespace filesystem {
        std::string getUserProfileDir();
        std::string getDefaultDataDir(const std::string& appName);
    }
}
