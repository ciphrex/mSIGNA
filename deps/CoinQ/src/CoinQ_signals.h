///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_signals.h 
//
// Copyright (c) 2012-2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#ifndef _COINQ_SIGNALS_H_
#define _COINQ_SIGNALS_H_

#include <functional>
#include <list>

template<typename... Values>
class CoinQSignal
{
private:
    std::list<std::function<void(Values...)>> fns;

public:
    void connect(std::function<void(Values...)> fn) { fns.push_back(fn); }
    void clear() { fns.clear(); }
    void operator()(Values... values) { for (auto fn : fns) fn(values...); }
};

template<>
class CoinQSignal<void>
{
private:
    std::list<std::function<void()>> fns;

public:
    void connect(std::function<void()> fn) { fns.push_back(fn); }
    void clear() { fns.clear(); }
    void operator()() { for (auto fn : fns) fn(); }
};

#endif // _COINQ_SIGNALS_H_

