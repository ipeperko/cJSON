#pragma once

#include <memory>
#include <cstring>
#include <list>
#include <mutex>
#include <atomic>
#include <numeric>
#include <iostream>

#define LOG_DEBUG if (memory_resource::enable_debug) std::cout

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
        m_initial_length = len;
    }

    void* do_allocate(std::size_t sz)
    {
        LOG_DEBUG << "[*] Allocating " << sz << " bytes\n";

        buffer* buff;

        if (m_buffers.empty() || m_buffers.back()->size - m_buffers.back()->load < sz)
        {
            std::size_t size_to_allocate;
            std::size_t const initial_len = m_initial_length; // cache to prevent data race

            if (sz > initial_len) {
                size_to_allocate = sz;
                //m_initial_length = sz;
            }
            else {
                size_to_allocate = initial_len;
            }
            LOG_DEBUG << "[*] Creating new buffer of size " << size_to_allocate << "\n";
            m_buffers.emplace_back(new buffer{std::make_unique<std::byte[]>(size_to_allocate), size_to_allocate, 0});
            buff = m_buffers.back().get();
            memset(buff->data.get(), 0, buff->size);
        }
        else
        {
            buff = m_buffers.back().get();
            LOG_DEBUG << "[*] Using existing buffer which has " << buff->size - buff->load << " free bytes\n";
        }

        void* p = &buff->data[buff->load];
        buff->load += sz;
        LOG_DEBUG << "[*] Allocated at : " << p << "\n";
        return p;
    }

    void do_dealocate_all()
    {
        LOG_DEBUG << "[*] Free all\n";

        std::size_t const total_load = get_load();
        LOG_DEBUG << "[*] Total load was " << total_load << "\n";

        // Update initial length
        update_initial_buffer_length(total_load);

        m_buffers.clear();
    }

    void do_dealocate()
    {
        LOG_DEBUG << "[*] Free\n";

        if (m_buffers.size() > 1) {
            LOG_DEBUG << "[*] Removing all buffers\n";

            std::size_t const total_load = get_load();
            LOG_DEBUG << "[*] Total load was " << total_load << "\n";

            // Update initial length
            update_initial_buffer_length(total_load);

            // Clear buffers
            m_buffers.clear();

            LOG_DEBUG << "[*] Creating new buffer of size " << m_initial_length << "\n";
            m_buffers.emplace_back(new buffer{std::make_unique<std::byte[]>(total_load), total_load, 0});
        }
        else {
            LOG_DEBUG << "[*] Will keep the existing buffer\n";
        }

        if (!m_buffers.empty()) {
            auto &buf = m_buffers.front();
            memset(buf->data.get(), 0, buf->size);
            buf->load = 0;
        }
    }

private:
    static void update_initial_buffer_length(std::size_t load)
    {
        std::lock_guard lock(m_mutex);
        if (load > m_initial_length)
            m_initial_length = load;
    }

    std::size_t get_load() const
    {
        std::size_t const total_load = std::accumulate(m_buffers.begin(), m_buffers.end(), 0u, [](std::size_t load, const auto& item)
        {
            return load + item->load;
        });

        return total_load;
    }

    std::list<std::unique_ptr<buffer>> m_buffers;
    static std::atomic<std::size_t> m_initial_length;
    static std::mutex m_mutex;
};