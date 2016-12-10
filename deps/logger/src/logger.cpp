///////////////////////////////////////////////////////////////////////////////
//
// logger.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "logger.h"

#include <fstream>
#include <time.h>

namespace logger {
    std::ofstream file;
    void init_logger(const char* filename)
    {
        file.open(filename, std::ios_base::app);
    }

    std::string timestamp()
    {
        time_t rawtime;
        time(&rawtime);
        struct tm* timeinfo = gmtime(&rawtime);

        char buffer[20];
        strftime(buffer, 20, "%F %T",timeinfo);
        return std::string(buffer);
    }

    std::ostream out(file.rdbuf());
    std::ostream no_out(NULL);
}
