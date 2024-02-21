#include <Core/DeferredDeallocationUtils.h>
#include <Core/DeferredDeallocator.h>

namespace fe
{
	void RequestDeallocation(DeferredDeallocator& deferredDeallocator, DefaultCallback requester)
	{
		deferredDeallocator.RequestDeallocation(std::move(requester));
	}
}