#include "memory_resource_pool.h"

memory_resource* memory_resource_pool::acquire()
{
    std::unique_lock lock(mutex_);

    if (!sessions_idle_.empty())
    {
        auto it = sessions_idle_.begin();
        auto ptr = it->first;
        sessions_active_[ptr] = std::move(it->second);
        sessions_idle_.erase(it);
        return ptr;
    }
    else
    {
        auto uptr = std::make_unique<memory_resource>();
        auto ptr = uptr.get();
        sessions_active_[ptr] = std::move(uptr);
        return ptr;
    }
}

void memory_resource_pool::release(memory_resource* session)
{
    if (!session)
        return;

    std::unique_lock lock(mutex_);

    auto it = sessions_active_.find(session);
    auto ptr = it->first;
    if (!ptr)
        return;

    if (limit_ == unlimited || sessions_idle_.size() < limit_)
    {
        ptr->do_dealocate();
        sessions_idle_[ptr] = std::move(it->second);
    }

    sessions_active_.erase(it);
}

std::size_t memory_resource_pool::active_size() const
{
    std::unique_lock lock(mutex_);
    return sessions_active_.size();
}

std::size_t memory_resource_pool::idle_size() const
{
    std::unique_lock lock(mutex_);
    return sessions_idle_.size();
}