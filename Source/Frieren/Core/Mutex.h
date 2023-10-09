#pragma once
#include <shared_mutex>

namespace fe
{
    using SharedMutex = std::shared_mutex;
    using ReadOnlyLock = std::shared_lock<SharedMutex>;
    using WriteLock = std::unique_lock<SharedMutex>;
}
