#pragma once

#include "memory_resource.h"
#include <vector>
#include <unordered_map>

class memory_resource_pool;

class memory_resource_pool_item
{
public:
    memory_resource_pool_item() = delete;

    memory_resource_pool_item(memory_resource_pool* pool, memory_resource* session)
        : pool_(pool)
        , session_(session)
    {}

    memory_resource_pool_item(memory_resource_pool_item const&) = delete;

    memory_resource_pool_item(memory_resource_pool_item&& other) noexcept;

    ~memory_resource_pool_item();

    memory_resource_pool_item& operator=(memory_resource_pool_item const&) = delete;

    memory_resource_pool_item& operator=(memory_resource_pool_item&& other) noexcept;

    auto const* pool() const { return pool_; }

    auto* get() const { return session_; }

    operator bool() const { return pool_ != nullptr && session_ != nullptr; }

    void release();

private:
    memory_resource_pool* pool_ {nullptr};
    memory_resource* session_ {nullptr};
};

class memory_resource_pool
{
    friend class memory_resource_pool_item;
public:

    using resource_map = std::unordered_map<memory_resource*, std::unique_ptr<memory_resource>>;

    static constexpr std::size_t unlimited = 0;

    memory_resource_pool_item acquire();

    void set_limit(std::size_t limit) { limit_ = limit; }

    auto limit() const { return limit_; }

    std::size_t active_size() const;

    std::size_t idle_size() const;

private:
    void release(memory_resource* session);

    std::size_t limit_ {unlimited};
    std::mutex mutable mutex_;
    resource_map sessions_idle_;
    resource_map sessions_active_;
};

