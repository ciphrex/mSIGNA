///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_slots.h 
//
// Copyright (c) 2012-2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#ifndef _COINQ_SLOTS_H_
#define _COINQ_SLOTS_H_

#include <functional>

#include <CoinCore/CoinNodeData.h>

// slot types
typedef std::function<void()>                                       void_slot_t;
typedef std::function<void(int)>                                    int_slot_t;
typedef std::function<void(const std::string&)>                     string_slot_t;
typedef std::function<void(const std::string&, int)>                error_slot_t;

typedef std::function<void(const Coin::HeadersMessage&)>            headers_slot_t;
typedef std::function<void(const Coin::CoinBlock&)>                 block_slot_t;
typedef std::function<void(const Coin::Transaction&)>               tx_slot_t;
typedef std::function<void(const Coin::AddrMessage&)>               addr_slot_t;

#endif // _COINQ_SLOTS_H_

