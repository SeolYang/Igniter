#include <Core/FrameResource.h>
#include <Core/FrameResourceManager.h>

namespace fe::Private
{
	void RequestDeallocation(DeferredDeallocator& deferredDeallocator, DefaultCallback&& requester)
	{
		deferredDeallocator.RequestDeallocation(std::move(requester));
	}
} // namespace fe::Private