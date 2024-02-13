#include <Core/FrameResource.h>
#include <Core/FrameResourceManager.h>

namespace fe::Private
{
	void RequestDeallocation(FrameResourceManager& frameResourceManger, Requester&& requester)
	{
		static_assert(std::is_same_v<FrameResourceManager::Requester, Requester>);
		frameResourceManger.RequestDeallocation(std::move(requester));
	}
} // namespace fe::Private