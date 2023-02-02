#include "cJSON.h"
#include <iostream>
#include <cassert>

#include "memory_resource_pool.h"

memory_resource memory;

void* custom_malloc(size_t sz)
{
    return memory.do_allocate(sz);
}

void custom_free(void *ptr)
{
}

int main()
{
    memory_resource::enable_debug = true;

    cJSON_Hooks hooks;
    hooks.malloc_fn = custom_malloc;
    hooks.free_fn = custom_free;

    cJSON_InitHooks(&hooks);

    std::cout << "--- hello world 1 ---\n";

    {
        auto obj = cJSON_CreateObject();
        cJSON_AddStringToObject(obj, "hello", "world1");
        auto str = cJSON_PrintUnformatted(obj);
        std::cout << str << "\n";
        assert(std::string_view(str) == R"({"hello":"world1"})");
        std::cout << "call cJSON_Delete\n";
        cJSON_Delete(obj); // has no effect in this case
        std::cout << "call memory.do_dealocate\n";
        memory.do_dealocate();
    }

    std::cout << "--- hello world 2 ---\n";

    {
        auto obj = cJSON_CreateObject();
        cJSON_AddStringToObject(obj, "hello", "world2");
        auto str = cJSON_PrintUnformatted(obj);
        std::cout << str << "\n";
        assert(std::string_view(str) == R"({"hello":"world2"})");
        std::cout << "call cJSON_Delete\n";
        cJSON_Delete(obj); // has no effect in this case
        std::cout << "call memory.do_dealocate\n";
        memory.do_dealocate();
    }

    std::cout << "--- pool acquire ---\n";

    memory_resource_pool pool;

    {
        auto mr1 = pool.acquire();
        assert(mr1 != nullptr);
        assert(pool.active_size() == 1);
        assert(pool.idle_size() == 0);

        auto mr2 = pool.acquire();
        assert(mr2 != nullptr);
        assert(pool.active_size() == 2);
        assert(pool.idle_size() == 0);

        pool.release(mr1);
        assert(pool.active_size() == 1);
        assert(pool.idle_size() == 1);

        pool.release(mr2);
        assert(pool.active_size() == 0);
        assert(pool.idle_size() == 2);

        auto mr3 = pool.acquire();
        assert(mr3 != nullptr);
        assert(mr3 == mr1 || mr3 == mr2);
        assert(pool.active_size() == 1);
        assert(pool.idle_size() == 1);

        pool.release(mr3);
        assert(pool.active_size() == 0);
        assert(pool.idle_size() == 2);
    }
}