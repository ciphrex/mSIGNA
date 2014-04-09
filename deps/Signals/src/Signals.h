///////////////////////////////////////////////////////////////////////////////
//
// Signals.h 
//
// Copyright (c) 2012-2014 Eric Lombrozo
//
// All Rights Reserved.
//

#pragma once

#include <functional>
#include <set>
#include <map>
#include <mutex>

namespace Signals
{

typedef uint64_t Connection;

template<typename... Values>
class Signal
{
public:
    typedef std::function<void(Values...)> Slot;

    Signal();
    ~Signal();

    Connection connect(Slot slot);
    bool disconnect(Connection connection);
    void clear();
    void operator()(Values... values) const;

private:
    mutable std::mutex mutex_;
    Connection next_;
    std::set<Connection> available_;
    std::map<Connection, Slot> slots_;
};

template<typename... Values>
Signal<Values...>::Signal() : next_(0)
{
}

template<typename... Values>
Signal<Values...>::~Signal()
{
    std::lock_guard<std::mutex> lock(mutex_);
}

template<typename... Values>
Connection Signal<Values...>::connect(Slot slot)
{
    std::lock_guard<std::mutex> lock(mutex_);
    Connection connection;
    auto it = available_.begin();
    if (it == available_.end())
    {
        connection = next_++;
    }
    else
    {
        connection = *it;
        available_.erase(it);
    }
    slots_.insert(std::pair<Connection, Slot>(connection, slot));
    return connection;
}

template<typename... Values>
bool Signal<Values...>::disconnect(Connection connection)
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto& it = slots_.find(connection);
    if (it == slots_.end()) return false;

    slots_.erase(it);
    available_.insert(connection);

    // remove contiguous available connections from end
    auto rit = available_.rbegin();
    for (; rit != available_.rend() && *rit == next_ - 1; ++rit, --next_);
    available_.erase(rit.base(), available_.end());
    return true;
}

template<typename... Values>
void Signal<Values...>::clear()
{
    std::lock_guard<std::mutex> lock(mutex_);
    slots_.clear();
    available_.clear();
    next_ = 0;
}

template<typename... Values>
void Signal<Values...>::operator()(Values... values) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto slot: slots_) slot.second(values...); 
}
/*
template<>
class Signal<void>
{
public:
    typedef std::function<void()> Slot;

    Signal();
    ~Signal();

    Connection connect(Slot slot);
    bool disconnect(Connection connection);
    void clear();
    void operator()() const;

private:
    mutable std::mutex mutex_;
    Connection next_;
    std::set<Connection> available_;
    std::map<Connection, Slot> slots_;
};

template<>
Signal<>::Signal() : next_(0)
{
}

template<>
Signal<>::~Signal()
{
    std::lock_guard<std::mutex> lock(mutex_);
}

template<>
Connection Signal<>::connect(Slot slot)
{
    std::lock_guard<std::mutex> lock(mutex_);
    Connection connection;
    auto it = available_.begin();
    if (it == available_.end())
    {
        connection = next_++;
    }
    else
    {
        connection = *it;
        available_.erase(it);
    }
    slots_.insert(std::pair<Connection, Slot>(connection, slot));
    return connection;
}

template<>
bool Signal<>::disconnect(Connection connection)
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto& it = slots_.find(connection);
    if (it == slots_.end()) return false;

    slots_.erase(it);
    available_.insert(connection);

    // remove contiguous available connections from end
    auto& rit = available_.rbegin();
    for (; rit != available_.rend() && *rit == next_ - 1; ++rit, --next_);
    available_.erase(available_.rbegin(), rit); 
    return true;
}

template<>
void Signal<>::clear()
{
    std::lock_guard<std::mutex> lock(mutex_);
    slots_.clear();
    available_.clear();
    next_ = 0;
}

template<>
void Signal<>::operator()() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto slot: slots_) slot.second(); 
}
*/
}
