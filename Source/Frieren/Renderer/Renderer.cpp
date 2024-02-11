#include <Renderer/Renderer.h>
#include <D3D12/Device.h>
#include <D3D12/DescriptorHeap.h>
#include <D3D12/Swapchain.h>

namespace fe
{
	Renderer::Renderer(const Window& window)
		: device(std::make_unique<dx::Device>())
		, swapchain(std::make_unique<dx::Swapchain>(window, *device, NumFramesInFlight))
		, globalFrames(0)
		, localFrames(0)
	{
		frameResources.reserve(NumFramesInFlight);
		for (uint8_t idx = 0; idx < NumFramesInFlight; ++idx)
		{
			frameResources.emplace_back(*device);
		}
	}

	Renderer::~Renderer()
	{
		device->FlushGPU();
	}

	void Renderer::Render()
	{
		// BeginFrame();
		// swapchain->Present();
		// EndFrame();

		auto& localFrameFence = frameResources[localFrames].GetFence();
		device->Wait(localFrameFence, dx::EQueueType::Direct);

		swapchain->Present();

		++globalFrames;
		localFrames = globalFrames % NumFramesInFlight;
		device->NextSignal(localFrameFence, dx::EQueueType::Direct);
	}

} // namespace fe