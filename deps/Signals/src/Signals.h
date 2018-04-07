///////////////////////////////////////////////////////////////////////////////
//
// Signals.h 
//
// Copyright (c) 2012-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
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
    std::function<void()> bind(Values... values) const;
    void operator()(Values... values) const { exec(values...); }

#ifdef SIGNALS_TEST
    std::string getTextualState()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::stringstream ss;
        ss << "next_: " << next_ << std::endl << "available_:";
        for (auto n: available_) ss << " " << n;
        ss << std::endl << "slots_:";
        for (auto& slot: slots_) ss << " " << slot.first;
        ss << std::endl;
        return ss.str(); 
    }
#endif

private:
    void exec(Values... values) const;

    mutable std::mutex mutex_;
    Connection next_;
    std::set<Connection> available_;
    std::map<Connection, Slot> slots_;
};

template<typename... Values>
inline Signal<Values...>::Signal() : next_(0)
{
}

template<typename... Values>
inline Signal<Values...>::~Signal()
{
    std::lock_guard<std::mutex> lock(mutex_);
}

template<typename... Values>
inline Connection Signal<Values...>::connect(Slot slot)
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
inline bool Signal<Values...>::disconnect(Connection connection)
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
inline void Signal<Values...>::clear()
{
    std::lock_guard<std::mutex> lock(mutex_);
    slots_.clear();
    available_.clear();
    next_ = 0;
}

template<typename... Values>
inline void Signal<Values...>::exec(Values... values) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto slot: slots_) slot.second(values...); 
}

template<typename... Values>
inline std::function<void()> Signal<Values...>::bind(Values... values) const
{
    return std::bind([this](Values... values) { exec(values...); }, values...);
}

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
    std::function<void()> bind() const;
    void operator()() const { exec(); }

#ifdef SIGNALS_TEST
    std::string getTextualState()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::stringstream ss;
        ss << "next_: " << next_ << std::endl << "available_:";
        for (auto n: available_) ss << " " << n;
        ss << std::endl << "slots_:";
        for (auto& slot: slots_) ss << " " << slot.first;
        ss << std::endl;
        return ss.str(); 
    }
#endif

private:
    void exec() const;

    mutable std::mutex mutex_;
    Connection next_;
    std::set<Connection> available_;
    std::map<Connection, Slot> slots_;
};

template<>
inline Signal<>::Signal() : next_(0)
{
}

template<>
inline Signal<>::~Signal()
{
    std::lock_guard<std::mutex> lock(mutex_);
}

template<>
inline Connection Signal<>::connect(Slot slot)
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
inline bool Signal<>::disconnect(Connection connection)
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

template<>
inline void Signal<>::clear()
{
    std::lock_guard<std::mutex> lock(mutex_);
    slots_.clear();
    available_.clear();
    next_ = 0;
}

template<>
inline void Signal<>::exec() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto slot: slots_) slot.second(); 
}

template<>
inline std::function<void()> Signal<>::bind() const
{
    return [this]() { exec(); };
}

}
