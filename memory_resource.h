#pragma once

#include <memory>
#include <cstring>
#include <list>
#include <mutex>
#include <atomic>
#include <numeric>
#include <iostream>

#define MR_LOG_DEBUG if (memory_resource::enable_debug) std::cout

class memory_resource
{
public:

    struct buffer
    {
        std::unique_ptr<std::byte[]> data;
        const std::size_t size {0};
        std::size_t load {0};
    };

    static bool enable_debug;

    memory_resource() = default;

    ~memory_resource()
    {
        do_dealocate_all();
    }

    static void set_initial_buffer_length(std::size_t len)
    {
        initial_length_ = len;
    }

    void* do_allocate(std::size_t sz)
    {
        MR_LOG_DEBUG << "[*] Allocating " << sz << " bytes\n";

        buffer* buff;

        if (buffers_.empty() || buffers_.back()->size - buffers_.back()->load < sz)
        {
            std::size_t size_to_allocate;
            std::size_t const initial_len = initial_length_; // cache to prevent data race

            if (sz > initial_len) {
                size_to_allocate = sz;
                //initial_length_ = sz;
            }
            else {
                size_to_allocate = initial_len;
            }
            MR_LOG_DEBUG << "[*] Creating new buffer of size " << size_to_allocate << "\n";
            buffers_.emplace_back(new buffer{std::make_unique<std::byte[]>(size_to_allocate), size_to_allocate, 0});
            buff = buffers_.back().get();
            memset(buff->data.get(), 0, buff->size);
        }
        else
        {
            buff = buffers_.back().get();
            MR_LOG_DEBUG << "[*] Using existing buffer which has " << buff->size - buff->load << " free bytes\n";
        }

        void* p = &buff->data[buff->load];
        buff->load += sz;
        MR_LOG_DEBUG << "[*] Allocated at : " << p << "\n";
        return p;
    }

    void do_dealocate_all()
    {
        MR_LOG_DEBUG << "[*] Free all\n";

        std::size_t const total_load = get_load();
        MR_LOG_DEBUG << "[*] Total load was " << total_load << "\n";

        // Update initial length
        update_initial_buffer_length(total_load);

        buffers_.clear();
    }

    void do_dealocate()
    {
        MR_LOG_DEBUG << "[*] Free\n";

        if (buffers_.size() > 1) {
            MR_LOG_DEBUG << "[*] Removing all buffers\n";

            std::size_t const total_load = get_load();
            MR_LOG_DEBUG << "[*] Total load was " << total_load << "\n";

            // Update initial length
            update_initial_buffer_length(total_load);

            // Clear buffers
            buffers_.clear();

            MR_LOG_DEBUG << "[*] Creating new buffer of size " << initial_length_ << "\n";
            buffers_.emplace_back(new buffer{std::make_unique<std::byte[]>(total_load), total_load, 0});
        }
        else {
            MR_LOG_DEBUG << "[*] Keep the existing buffer\n";
        }

        if (!buffers_.empty()) {
            auto &buf = buffers_.front();
            memset(buf->data.get(), 0, buf->size);
            buf->load = 0;
        }
    }

private:
    static void update_initial_buffer_length(std::size_t load)
    {
        std::lock_guard lock(mutex_);
        if (load > initial_length_)
            initial_length_ = load;
    }

    std::size_t get_load() const
    {
        std::size_t const total_load = std::accumulate(buffers_.begin(), buffers_.end(), 0u, [](std::size_t load, const auto& item)
        {
            return load + item->load;
        });

        return total_load;
    }

    std::list<std::unique_ptr<buffer>> buffers_;
    static std::atomic<std::size_t> initial_length_;
    static std::mutex mutex_;
};