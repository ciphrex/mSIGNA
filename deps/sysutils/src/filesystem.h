///////////////////////////////////////////////////////////////////
//
// filesystem.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include <string>

namespace sysutils {
    namespace filesystem {
        std::string getUserProfileDir();
        std::string getDefaultDataDir(const std::string& appName);
    }
}
