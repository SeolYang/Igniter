#pragma once
#include <Igniter.h>

namespace ig
{
    class ThreadUIDGenerator final
    {
    public:
        static size_t GetUID();
    };
}    // namespace ig
