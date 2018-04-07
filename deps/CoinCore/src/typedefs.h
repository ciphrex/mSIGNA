////////////////////////////////////////////////////////////////////////////////
//
// typedefs.h
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

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
