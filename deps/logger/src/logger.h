///////////////////////////////////////////////////////////////////////////////
//
// logger.h
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#ifndef _LOGGER_H__
#define _LOGGER_H__

#include <sstream>
#include <string>

namespace logger {
    void init_logger(const char* filename);
    std::string timestamp();
    extern "C" std::ostream out;
    extern "C" std::ostream no_out;
}

#define INIT_LOGGER(filename) logger::init_logger(filename)

#if !defined(LOGGER_TRACE) && !defined(LOGGER_DEBUG) && !defined(LOGGER_INFO) && !defined(LOGGER_WARNING) && !defined(LOGGER_ERROR) && !defined(LOGGER_FATAL)
    #define LOGGER_TRACE
#endif

#define LOGGER(level) LOGGER_##level

#if defined(LOGGER_TRACE)
    #define LOGGER_trace logger::out << logger::timestamp() << " [trace] "
#else
    #define LOGGER_trace logger::no_out
#endif

#if defined(LOGGER_TRACE) || defined(LOGGER_DEBUG)
    #define LOGGER_debug logger::out << logger::timestamp() << " [debug] "
#else
    #define LOGGER_debug logger::no_out
#endif

#if defined(LOGGER_TRACE) || defined(LOGGER_DEBUG) || defined(LOGGER_INFO)
    #define LOGGER_info logger::out << logger::timestamp() << " [info] "
#else
    #define LOGGER_info logger::no_out
#endif

#if defined(LOGGER_TRACE) || defined(LOGGER_DEBUG) || defined(LOGGER_INFO) || defined(LOGGER_WARNING)
    #define LOGGER_warning logger::out << logger::timestamp() << " [warning] "
#else
    #define LOGGER_warning logger::no_out
#endif

#if defined(LOGGER_TRACE) || defined(LOGGER_DEBUG) || defined(LOGGER_INFO) || defined(LOGGER_WARNING) || defined(LOGGER_ERROR)
    #define LOGGER_error logger::out << logger::timestamp() << " [error] "
#else
    #define LOGGER_error logger:no_out
#endif

#define LOGGER_fatal logger::out << logger::timestamp() << " [fatal] "

#endif // _LOGGER_H__
