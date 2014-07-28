///////////////////////////////////////////////////////////////////////////////
//
// SignalQueue.h 
//
// Copyright (c) 2012-2014 Eric Lombrozo
//
// All Rights Reserved.
//

#pragma once

#include <functional>
#include <queue>
#include <mutex>

namespace Signals
{

class SignalQueue
{
public:
    void push(std::function<void()> f);
    void flush();

private:
    std::mutex mutex_;
    std::queue<std::function<void()>> queue_;
};

inline void SignalQueue::push(std::function<void()> f)
{
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(f);
}

inline void SignalQueue::flush()
{
    std::lock_guard<std::mutex> lock(mutex_);
    while (!queue_.empty())
    {
        queue_.front()();
        queue_.pop();
    } 
}

}
