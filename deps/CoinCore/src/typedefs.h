////////////////////////////////////////////////////////////////////////////////
//
// typedefs.h
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

#ifndef __TYPEDEFS_H___
#define __TYPEDEFS_H___

#include <vector>
#include <set>
#include <string>

typedef std::vector<unsigned char> bytes_t;
typedef std::vector<unsigned char> secure_bytes_t;

typedef std::vector<bytes_t> hashvector_t;
typedef std::set<bytes_t> hashset_t;

typedef std::string secure_string_t;

typedef std::vector<int> ints_t;
typedef std::vector<int> secure_ints_t;

// TODO: use custom allocators for secure types

#endif // __TYPEDEFS_H__
