#pragma once
#include <Core/Container.h>

namespace fe
{
	class DeferredDeallocator;
	void RequestDeallocation(DeferredDeallocator& deferredDeallocator, DefaultCallback requester);
} // namespace fe