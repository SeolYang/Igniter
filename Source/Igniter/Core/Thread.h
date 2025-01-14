#pragma once
#include "Igniter/Igniter.h"

namespace ig
{
    class ThreadUIDGenerator final
    {
      public:
        static size_t GetUID();
    };

    class ThreadInfo final
    {
      public:
        static void RegisterMainThreadID();
        static bool IsMainThread();
    };
} // namespace ig
