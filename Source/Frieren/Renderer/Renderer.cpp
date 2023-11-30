#include <Renderer/Renderer.h>
#include <D3D12/Device.h>
#include <D3D12/DescriptorHeap.h>

namespace fe
{
	Renderer::Renderer()
		: device(std::make_unique<Device>()), srvDescHeap(std::make_unique<DescriptorHeap>(*device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 16, "TestHeap"))
	{
		{
			auto testdesc = srvDescHeap->AllocateDescriptor();
		}
		auto testdesc2 = srvDescHeap->AllocateDescriptor();
	}

	Renderer::~Renderer()
	{
	}

} // namespace fe