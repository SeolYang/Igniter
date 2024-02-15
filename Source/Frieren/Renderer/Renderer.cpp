#include <Renderer/Renderer.h>
#include <D3D12/Device.h>
#include <D3D12/CommandQueue.h>
#include <D3D12/CommandContext.h>
#include <D3D12/CommandContextPool.h>
#include <D3D12/Fence.h>
#include <D3D12/DescriptorHeap.h>
#include <D3D12/Swapchain.h>

// #test
#include <D3D12/GPUBuffer.h>
#include <D3D12/GPUBufferDesc.h>
struct SimpleVertex
{
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;
};

namespace fe
{
	FE_DECLARE_LOG_CATEGORY(RendererInfo, ELogVerbosiy::Info)
	FE_DECLARE_LOG_CATEGORY(RendererWarn, ELogVerbosiy::Warning)
	FE_DECLARE_LOG_CATEGORY(RendererFatal, ELogVerbosiy::Fatal)

	Renderer::Renderer(const FrameManager& engineFrameManager, Window& window, dx::Device& device)
		: frameManager(engineFrameManager), renderDevice(device), directCmdQueue(std::make_unique<dx::CommandQueue>(device.CreateCommandQueue(dx::EQueueType::Direct).value())), directCmdCtxPool(std::make_unique<dx::CommandContextPool>(device, dx::EQueueType::Direct)), swapchain(std::make_unique<dx::Swapchain>(window, device, *directCmdQueue, NumFramesInFlight))
	{
		frameFences.reserve(NumFramesInFlight);
		for (uint8_t localFrameIdx = 0; localFrameIdx < NumFramesInFlight; ++localFrameIdx)
		{
			frameFences.emplace_back(
				std::make_unique<dx::Fence>(device.CreateFence(std::format("FrameFence_{}", localFrameIdx)).value()));
		}

		dx::GPUBufferDesc vbDesc{};
		vbDesc.AsVertexBuffer<SimpleVertex>(4);
		quadVB = std::make_unique<dx::GPUBuffer>(renderDevice.CreateBuffer(vbDesc).value());

		dx::GPUBufferDesc ibDesc{};
		ibDesc.AsIndexBuffer<uint16_t>(6);
		quadIB = std::make_unique<dx::GPUBuffer>(renderDevice.CreateBuffer(ibDesc).value());
	}

	Renderer::~Renderer()
	{
		directCmdQueue->FlushQueue(renderDevice);
	}

	void Renderer::BeginFrame()
	{
		frameFences[frameManager.GetLocalFrameIndex()]->WaitOnCPU();
	}

	void Renderer::Render(FrameResourceManager& /*frameResourceManager*/)
	{
	}

	void Renderer::EndFrame()
	{
		directCmdQueue->FlushPendingContexts();
		swapchain->Present();
		directCmdQueue->NextSignalTo(*frameFences[frameManager.GetLocalFrameIndex()]);
	}
} // namespace fe