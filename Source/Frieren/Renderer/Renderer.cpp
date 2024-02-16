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

		// #test #todo upload to buffer/texture
		dx::GPUBufferDesc vbDesc{};
		vbDesc.AsVertexBuffer<SimpleVertex>(4);
		quadVB = std::make_unique<dx::GPUBuffer>(renderDevice.CreateBuffer(vbDesc).value());

		dx::GPUBufferDesc ibDesc{};
		ibDesc.AsIndexBuffer<uint16_t>(6);
		quadIB = std::make_unique<dx::GPUBuffer>(renderDevice.CreateBuffer(ibDesc).value());

		dx::Fence		   uploadFence = device.CreateFence("UploadFence").value();
		dx::CommandContext uploadCtx = device.CreateCommandContext(dx::EQueueType::Direct).value();

		dx::GPUBufferDesc quadUploadDesc{};
		quadUploadDesc.AsUploadBuffer(static_cast<uint32_t>(vbDesc.GetSizeAsBytes() + ibDesc.GetSizeAsBytes()));
		dx::GPUBuffer	   quadUploadBuffer = renderDevice.CreateBuffer(quadUploadDesc).value();
		const SimpleVertex vertices[4] = {
			{ -.5f, 0.f, 0.f },
			{ -0.5f, 0.5f, 0.f },
			{ .5f, 0.5f, 0.f },
			{ .5f, 0.f, 0.f }
		};

		const uint16_t indices[6] = {
			0,
			1,
			2,
			2,
			1,
			3
		};

		{
			dx::GPUResourceMapGuard mappedUploadBuffer = quadUploadBuffer.Map();
			std::memcpy(mappedUploadBuffer.get(), vertices, vbDesc.GetSizeAsBytes());
			std::memcpy(mappedUploadBuffer.get() + vbDesc.GetSizeAsBytes(), indices, ibDesc.GetSizeAsBytes());
		}

		uploadCtx.Begin();
		uploadCtx.AddPendingBufferBarrier(
			*quadVB,
			D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COPY,
			D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_COPY_DEST);

		uploadCtx.AddPendingBufferBarrier(
			*quadIB,
			D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COPY,
			D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_COPY_DEST);

		uploadCtx.AddPendingBufferBarrier(
			quadUploadBuffer,
			D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COPY,
			D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_COPY_SOURCE);

		uploadCtx.FlushPendingBufferBarriers();

		uploadCtx.CopyBuffer(quadUploadBuffer, 0, vbDesc.GetSizeAsBytes(), *quadVB, 0);
		uploadCtx.CopyBuffer(quadUploadBuffer, vbDesc.GetSizeAsBytes(), ibDesc.GetSizeAsBytes(), *quadIB, 0);

		uploadCtx.AddPendingBufferBarrier(
			*quadVB,
			D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_SYNC_VERTEX_SHADING,
			D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_VERTEX_BUFFER);

		uploadCtx.AddPendingBufferBarrier(
			*quadIB,
			D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_SYNC_INDEX_INPUT,
			D3D12_BARRIER_ACCESS_COPY_DEST, D3D12_BARRIER_ACCESS_INDEX_BUFFER);

		uploadCtx.FlushPendingBufferBarriers();
		uploadCtx.End();

		directCmdQueue->AddPendingContext(uploadCtx);
		directCmdQueue->FlushPendingContexts();
		directCmdQueue->NextSignalTo(uploadFence);
		uploadFence.WaitOnCPU();
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