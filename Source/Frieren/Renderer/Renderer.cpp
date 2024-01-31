#include <Renderer/Renderer.h>
#include <D3D12/Device.h>
#include <D3D12/DescriptorHeap.h>
#include <D3D12/Swapchain.h>

namespace fe
{
	Renderer::Renderer(const Window& window)
		: device(std::make_unique<dx::Device>())
		, swapchain(std::make_unique<dx::Swapchain>(window, *device, MaxFramesInFlight))
	{
	}

	Renderer::~Renderer()
	{
		device->FlushGPU();
	}

	void Renderer::Render()
	{
		//BeginFrame();
		swapchain->Present();
		//EndFrame();
	}

} // namespace fe