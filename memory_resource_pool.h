#pragma once

#include "memory_resource.h"
#include <vector>
#include <unordered_map>

class memory_resource_pool
{
public:

    using resource_map = std::unordered_map<memory_resource*, std::unique_ptr<memory_resource>>;

    static constexpr std::size_t unlimited = 0;

    memory_resource* acquire();

    void release(memory_resource* session);

    void set_limit(std::size_t limit) { limit_ = limit; }

    auto limit() const { return limit_; }

    std::size_t active_size() const;

    std::size_t idle_size() const;

private:
    std::size_t limit_ {unlimited};
    std::mutex mutable mutex_;
    resource_map sessions_idle_;
    resource_map sessions_active_;
};

