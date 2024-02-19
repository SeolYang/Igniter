#include <Renderer/Renderer.h>
#include <D3D12/Device.h>
#include <D3D12/CommandQueue.h>
#include <D3D12/CommandContext.h>
#include <D3D12/CommandContextPool.h>
#include <D3D12/Fence.h>
#include <D3D12/DescriptorHeap.h>
#include <D3D12/Swapchain.h>

#pragma region test
// #test
#include <D3D12/GPUBuffer.h>
#include <D3D12/GPUBufferDesc.h>
#include <D3D12/ShaderBlob.h>
#include <D3D12/PipelineState.h>
#include <D3D12/PipelineStateDesc.h>
#include <D3D12/RootSignature.h>
#include <D3D12/GPUTexture.h>
#include <D3D12/GPUTextureDesc.h>
#include <Gameplay/World.h>
#include <Renderer/StaticMeshComponent.h>

struct SimpleVertex
{
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;
};
#pragma endregion

namespace fe
{
	FE_DECLARE_LOG_CATEGORY(RendererInfo, ELogVerbosiy::Info)
	FE_DECLARE_LOG_CATEGORY(RendererWarn, ELogVerbosiy::Warning)
	FE_DECLARE_LOG_CATEGORY(RendererFatal, ELogVerbosiy::Fatal)

	Renderer::Renderer(const FrameManager& engineFrameManager, FrameResourceManager& frameResourceManager, Window& window, dx::Device& device)
		: frameManager(engineFrameManager), frameResourceManager(frameResourceManager), renderDevice(device), directCmdQueue(std::make_unique<dx::CommandQueue>(device.CreateCommandQueue(dx::EQueueType::Direct).value())), directCmdCtxPool(std::make_unique<dx::CommandContextPool>(device, dx::EQueueType::Direct)), swapchain(std::make_unique<dx::Swapchain>(window, device, frameResourceManager, *directCmdQueue, NumFramesInFlight))
	{
		frameFences.reserve(NumFramesInFlight);
		for (uint8_t localFrameIdx = 0; localFrameIdx < NumFramesInFlight; ++localFrameIdx)
		{
			frameFences.emplace_back(device.CreateFence(std::format("FrameFence_{}", localFrameIdx)).value());
		}

#pragma region test
		dx::GPUBufferDesc vbDesc{};
		vbDesc.AsVertexBuffer<SimpleVertex>(4);
		quadVB = std::make_unique<dx::GPUBuffer>(renderDevice.CreateBuffer(vbDesc).value());

		dx::GPUBufferDesc ibDesc{};
		ibDesc.AsIndexBuffer<uint16_t>(6);
		quadIB = std::make_unique<dx::GPUBuffer>(renderDevice.CreateBuffer(ibDesc).value());

		dx::Fence		   initialFence = device.CreateFence("InitialFence").value();
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
			3,
			0
		};
		{
			dx::GPUResourceMapGuard mappedUploadBuffer = quadUploadBuffer.Map();
			std::memcpy(mappedUploadBuffer.get(), vertices, vbDesc.GetSizeAsBytes());
			std::memcpy(mappedUploadBuffer.get() + vbDesc.GetSizeAsBytes(), indices, ibDesc.GetSizeAsBytes());
		}

		bindlessRootSignature = std::make_unique<dx::RootSignature>(device.CreateBindlessRootSignature().value());

		const dx::ShaderCompileDesc vsDesc{
			.SourcePath = String("Assets/Shader/BasicVertexShader.hlsl"),
			.Type = dx::EShaderType::Vertex,
			.OptimizationLevel = dx::EShaderOptimizationLevel::None
		};

		const dx::ShaderCompileDesc psDesc{
			.SourcePath = String("Assets/Shader/BasicPixelShader.hlsl"),
			.Type = dx::EShaderType::Pixel
		};

		vs = std::make_unique<dx::ShaderBlob>(vsDesc);
		ps = std::make_unique<dx::ShaderBlob>(psDesc);

		dx::GraphicsPipelineStateDesc psoDesc;
		psoDesc.SetVertexShader(*vs);
		psoDesc.SetPixelShader(*ps);
		psoDesc.SetRootSignature(*bindlessRootSignature);
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

		// #todo encapsulate input layout?
		const D3D12_INPUT_ELEMENT_DESC inputElement{
			.SemanticName = "POSITION",
			.SemanticIndex = 0,
			.Format = DXGI_FORMAT_R32G32B32_FLOAT,
			.InputSlot = 0,
			.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
			.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			.InstanceDataStepRate = 0
		};

		psoDesc.InputLayout.NumElements = 1;
		psoDesc.InputLayout.pInputElementDescs = &inputElement;
		pso = std::make_unique<dx::PipelineState>(device.CreateGraphicsPipelineState(psoDesc).value());

		dx::GPUTextureDesc depthStencilDesc;
		depthStencilDesc.DebugName = String("DepthStencilBufferTex");
		depthStencilDesc.AsDepthStencil(1280, 642, DXGI_FORMAT_D32_FLOAT);
		depthStencilBuffer = std::make_unique<dx::GPUTexture>(device.CreateTexture(depthStencilDesc).value());
		dsv = device.CreateDepthStencilView(frameResourceManager, *depthStencilBuffer, { .MipSlice = 0 });

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

		uploadCtx.AddPendingTextureBarrier(
			*depthStencilBuffer,
			D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_DEPTH_STENCIL,
			D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE,
			D3D12_BARRIER_LAYOUT_UNDEFINED, D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE);

		uploadCtx.FlushBarriers();

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

		uploadCtx.FlushBarriers();
		uploadCtx.End();

		directCmdQueue->AddPendingContext(uploadCtx);
		directCmdQueue->FlushPendingContexts();
		directCmdQueue->NextSignalTo(initialFence);
		initialFence.WaitOnCPU();

#pragma endregion
	}

	Renderer::~Renderer()
	{
		directCmdQueue->FlushQueue(renderDevice);
	}

	void Renderer::WaitForFences()
	{
		for (auto& fence : frameFences)
		{
			fence.WaitOnCPU();
		}
	}

	void Renderer::BeginFrame()
	{
		frameFences[frameManager.GetLocalFrameIndex()].WaitOnCPU();
	}

	void Renderer::Render()
	{
		// 더 발전된 형태로는 수시로 CmdQueue에 CmdCtx를 제출하고, 필요하다면 서로다른
		// 형태의 CmdQueue와 병렬적으로 동작하며 동기화 될 수 있도록 하는 것이 최적.
		// 이 때, frame fences의 signal은 present 직전에 back buffer에 그리기 직전에 수행하는 것이 좋을 것 같음.
		check(directCmdQueue);
		check(directCmdCtxPool);
		FrameResource<dx::CommandContext> renderCmdCtx = directCmdCtxPool->Request(frameResourceManager);
		renderCmdCtx->Begin(pso.get());
		{
			auto bindlessDescriptorHeaps = renderDevice.GetBindlessDescriptorHeaps();
			renderCmdCtx->SetDescriptorHeaps(bindlessDescriptorHeaps);
			renderCmdCtx->SetRootSignature(*bindlessRootSignature);
			renderCmdCtx->SetVertexBuffer(*quadVB);
			renderCmdCtx->SetIndexBuffer(*quadIB);

			check(swapchain);
			dx::GPUTexture&	   backBuffer = swapchain->GetBackBuffer();
			const dx::GPUView& backBufferRTV = swapchain->GetRenderTargetView();
			renderCmdCtx->AddPendingTextureBarrier(backBuffer,
												   D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_RENDER_TARGET,
												   D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_RENDER_TARGET,
												   D3D12_BARRIER_LAYOUT_PRESENT, D3D12_BARRIER_LAYOUT_RENDER_TARGET);
			renderCmdCtx->FlushBarriers();

			renderCmdCtx->ClearRenderTarget(backBufferRTV);
			renderCmdCtx->ClearDepth(*dsv);
			renderCmdCtx->SetRenderTarget(backBufferRTV, *dsv);
			renderCmdCtx->SetViewport(0.f, 0.f, 1280.f, 642.f);
			renderCmdCtx->SetScissorRect(0, 0, 1280, 642);
			renderCmdCtx->SetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			renderCmdCtx->DrawIndexed(6);
		}
		renderCmdCtx->End();

		directCmdQueue->AddPendingContext(*renderCmdCtx);
	}

	void Renderer::Render(World& world)
	{
		check(directCmdQueue);
		check(directCmdCtxPool);

		FrameResource<dx::CommandContext> renderCmdCtx = directCmdCtxPool->Request(frameResourceManager);
		renderCmdCtx->Begin(pso.get());
		{
			auto bindlessDescriptorHeaps = renderDevice.GetBindlessDescriptorHeaps();
			renderCmdCtx->SetDescriptorHeaps(bindlessDescriptorHeaps);
			renderCmdCtx->SetRootSignature(*bindlessRootSignature);

			check(swapchain);
			dx::GPUTexture&	   backBuffer = swapchain->GetBackBuffer();
			const dx::GPUView& backBufferRTV = swapchain->GetRenderTargetView();
			renderCmdCtx->AddPendingTextureBarrier(backBuffer,
												   D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_RENDER_TARGET,
												   D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_RENDER_TARGET,
												   D3D12_BARRIER_LAYOUT_PRESENT, D3D12_BARRIER_LAYOUT_RENDER_TARGET);
			renderCmdCtx->FlushBarriers();

			renderCmdCtx->ClearRenderTarget(backBufferRTV);
			renderCmdCtx->ClearDepth(*dsv);
			renderCmdCtx->SetRenderTarget(backBufferRTV, *dsv);
			renderCmdCtx->SetViewport(0.f, 0.f, 1280.f, 642.f);
			renderCmdCtx->SetScissorRect(0, 0, 1280, 642);
			renderCmdCtx->SetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			world.Each<StaticMeshComponent>([&renderCmdCtx](StaticMeshComponent& component) {
				renderCmdCtx->SetVertexBuffer(*component.VertexBufferHandle);
				renderCmdCtx->SetIndexBuffer(*component.IndexBufferHandle);
				renderCmdCtx->DrawIndexed(6);
			});

			renderCmdCtx->SetVertexBuffer(*quadVB);
			renderCmdCtx->SetIndexBuffer(*quadIB);
		}
		renderCmdCtx->End();

		directCmdQueue->AddPendingContext(*renderCmdCtx);
	}

	void Renderer::EndFrame()
	{
		directCmdQueue->FlushPendingContexts();
		swapchain->Present();
		directCmdQueue->NextSignalTo(frameFences[frameManager.GetLocalFrameIndex()]);
	}
} // namespace fe