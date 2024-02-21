#pragma once
#include <Core/Handle.h>

namespace fe
{
	class HandleManager;
    class DeferredDeallocator;
}

namespace fe::dx
{
	class Device;
    class DescriptorHeap;
	class GPUView;
    class GPUViewManager
    {
	public:
		GPUViewManager(HandleManager&, DeferredDeallocator&, Device&);

    };
}