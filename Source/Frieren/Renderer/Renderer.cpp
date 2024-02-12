#include <Renderer/Renderer.h>
#include <D3D12/Device.h>
#include <D3D12/CommandQueue.h>
#include <D3D12/Fence.h>
#include <D3D12/DescriptorHeap.h>
#include <D3D12/Swapchain.h>

namespace fe
{
	Renderer::Renderer(const Window& window)
		: device(std::make_unique<dx::Device>())
		, directCmdQueue(std::make_unique<dx::CommandQueue>(device->CreateCommandQueue(dx::EQueueType::Direct).value()))
		, swapchain(std::make_unique<dx::Swapchain>(window, *device, *directCmdQueue, NumFramesInFlight))
		, globalFrameIdx(0)
		, localFrameIdx(0)
	{
		frameResources.reserve(NumFramesInFlight);
		for (uint8_t idx = 0; idx < NumFramesInFlight; ++idx)
		{
			frameResources.emplace_back(*device);
		}
	}

	Renderer::~Renderer()
	{
		directCmdQueue->FlushQueue(*device);
	}

	void Renderer::BeginFrame()
	{
		auto& localFrameFence = frameResources[localFrameIdx].GetFence();
		localFrameFence.WaitOnCPU();
	}

	void Renderer::Render() {}

	void Renderer::EndFrame()
	{
		auto& localFrameFence = frameResources[localFrameIdx].GetFence();
		swapchain->Present();
		++globalFrameIdx;
		localFrameIdx = globalFrameIdx % NumFramesInFlight;
		directCmdQueue->NextSignalTo(localFrameFence);
	}
} // namespace fe