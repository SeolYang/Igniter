#include <Renderer/Renderer.h>
#include <D3D12/Device.h>
#include <D3D12/DescriptorHeap.h>

namespace fe
{
	Renderer::Renderer()
		: device(std::make_unique<Device>())
	{
	}

	Renderer::~Renderer()
	{
	}

} // namespace fe