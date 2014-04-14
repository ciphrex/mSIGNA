///////////////////////////////////////////////////////////////////////////////
//
// logger.h
//
// Copyright (c) 2013 Eric Lombrozo
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

#ifndef _LOGGER_H__
#define _LOGGER_H__

#include <sstream>
#include <string>

namespace logger {
    void init_logger(const char* filename);
    extern "C" std::ostream out;
    extern "C" std::ostream no_out;
}

#define INIT_LOGGER(filename) logger::init_logger(filename)

#if !defined(LOGGER_TRACE) && !defined(LOGGER_DEBUG) && !defined(LOGGER_INFO) && !defined(LOGGER_WARNING) && !defined(LOGGER_ERROR) && !defined(LOGGER_FATAL)
    #define LOGGER_TRACE
#endif

#define LOGGER(level) LOGGER_##level

#if defined(LOGGER_TRACE)
    #define LOGGER_trace logger::out << "[trace] "
#else
    #define LOGGER_trace logger::no_out
#endif

#if defined(LOGGER_TRACE) || defined(LOGGER_DEBUG)
    #define LOGGER_debug logger::out << "[debug] "
#else
    #define LOGGER_debug logger::no_out
#endif

#if defined(LOGGER_TRACE) || defined(LOGGER_DEBUG) || defined(LOGGER_INFO)
    #define LOGGER_info logger::out << "[info] "
#else
    #define LOGGER_info logger::no_out
#endif

#if defined(LOGGER_TRACE) || defined(LOGGER_DEBUG) || defined(LOGGER_INFO) || defined(LOGGER_WARNING)
    #define LOGGER_warning logger::out << "[warning] "
#else
    #define LOGGER_warning logger::no_out
#endif

#if defined(LOGGER_TRACE) || defined(LOGGER_DEBUG) || defined(LOGGER_INFO) || defined(LOGGER_WARNING) || defined(LOGGER_ERROR)
    #define LOGGER_error logger::out << "[error] "
#else
    #define LOGGER_error logger:no_out
#endif

#define LOGGER_fatal logger::out << "[fatal] "

#endif // _LOGGER_H__
