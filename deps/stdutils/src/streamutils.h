///////////////////////////////////////////////////////////////////////////////
//
// streamutils.h
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include <typedefs.h>

#include <fstream>

namespace stdutils
{

template<typename T>
class serialize_out : public T
{
public:
    serialize_out<T>& operator<<(uint32_t val);
};

// most significant byte first 
inline void write_uint32_msbf(std::ofstream& f, uint32_t data)
{
    f.put((data & 0xff000000) >> 24);
    f.put((data & 0xff0000) >> 16);
    f.put((data & 0xff00) >> 8);
    f.put(data & 0xff);
}

// least significant byte first 
inline void write_uint32_lsbf(std::ofstream& f, uint32_t data)
{
    f.put((data & 0xff000000) >> 24);
    f.put((data & 0xff0000) >> 16);
    f.put((data & 0xff00) >> 8);
    f.put(data & 0xff);
}

}
