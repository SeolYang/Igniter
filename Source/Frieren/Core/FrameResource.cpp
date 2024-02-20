#include <Core/FrameResource.h>
#include <Core/DeferredDeallocator.h>

namespace fe::Private
{
	void RequestDeallocation(DeferredDeallocator& deferredDeallocator, DefaultCallback&& requester)
	{
		deferredDeallocator.RequestDeallocation(std::move(requester));
	}
} // namespace fe::Private