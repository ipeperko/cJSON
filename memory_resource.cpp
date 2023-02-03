#include "memory_resource.h"

bool memory_resource::enable_debug = false;
std::atomic<std::size_t> memory_resource::initial_length_ = 256;
std::mutex memory_resource::mutex_;