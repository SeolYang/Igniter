#pragma once
#include <shared_mutex>

namespace fe
{
	using SharedMutex = std::shared_mutex;
	using ReadOnlyLock = std::shared_lock<SharedMutex>;
	using ReadWriteLock = std::unique_lock<SharedMutex>;
	using RecursiveMutex = std::recursive_mutex;
	using RecursiveLock = std::unique_lock<RecursiveMutex>;
} // namespace fe
