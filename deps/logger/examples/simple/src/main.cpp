///////////////////////////////////////////////////////////////////////////////
//
// logger simple example
//
// main.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include <logger.h>

int main()
{
    LOGGER(trace) << "trace test" << std::endl;
    LOGGER(debug) << "debug test" << std::endl;
    LOGGER(info) << "info test" << std::endl;
    LOGGER(warning) << "warning test" << std::endl;
    LOGGER(error) << "error test" << std::endl;
    LOGGER(fatal) << "fatal test" << std::endl;

    return 0;
}
