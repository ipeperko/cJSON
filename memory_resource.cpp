#include "memory_resource.h"

bool memory_resource::enable_debug = false;
std::atomic<std::size_t> memory_resource::m_initial_length = 64;
std::mutex memory_resource::m_mutex;