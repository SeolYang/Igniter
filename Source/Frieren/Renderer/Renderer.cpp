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
		frameFences.reserve(NumFramesInFlight);
		for (uint8_t idx = 0; idx < NumFramesInFlight; ++idx)
		{
			frameFences.emplace_back(
				std::make_unique<dx::Fence>(device->CreateFence(std::format("FrameFence_{}", localFrameIdx)).value()));
		}
	}

	Renderer::~Renderer()
	{
		directCmdQueue->FlushQueue(*device);
	}

	void Renderer::BeginFrame()
	{
		frameFences[localFrameIdx]->WaitOnCPU();
	}

	void Renderer::Render() {}

	void Renderer::EndFrame()
	{
		directCmdQueue->FlushPendingContexts();
		swapchain->Present();
		directCmdQueue->NextSignalTo(*frameFences[localFrameIdx]);

		++globalFrameIdx;
		localFrameIdx = globalFrameIdx % NumFramesInFlight;
	}
} // namespace fe