#pragma once
#include <Core/Container.h>

namespace fe
{
	class DeferredDeallocator;
	namespace Private
	{
		void RequestDeallocation(DeferredDeallocator& deferredDeallocator, DefaultCallback&& requester);
	} // namespace Private

} // namespace fe