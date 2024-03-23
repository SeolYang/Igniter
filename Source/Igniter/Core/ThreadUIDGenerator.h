#pragma once
#include <Igniter.h>

namespace ig
{
    class ThreadUIDGenerator
    {
    public:
        static size_t GetUID();
    };
} // namespace ig