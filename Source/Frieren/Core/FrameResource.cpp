#include <Core/FrameResource.h>
#include <Core/FrameResourceManager.h>

namespace fe::Private
{
	void RequestDeallocation(FrameResourceManager& frameResourceManger, DefaultCallback&& requester)
	{
		frameResourceManger.RequestDeallocation(std::move(requester));
	}
} // namespace fe::Private