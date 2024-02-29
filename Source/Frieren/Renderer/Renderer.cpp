#include <Renderer/Renderer.h>
#include <D3D12/RenderDevice.h>
#include <D3D12/CommandQueue.h>
#include <D3D12/CommandContext.h>
#include <D3D12/CommandContextPool.h>
#include <D3D12/Fence.h>
#include <D3D12/DescriptorHeap.h>
#include <D3D12/Swapchain.h>
#include <D3D12/TempConstantBufferAllocator.h>

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
#include <D3D12/GPUViewManager.h>
#include <D3D12/GPUView.h>
#include <Gameplay/World.h>
#include <Gameplay/PositionComponent.h>
#include <Renderer/StaticMeshComponent.h>
#include <ranges>
#include <Engine.h>

struct PositionBuffer
{
	float Position[3] = { 0.f, 0.f, 0.f };
};

struct BasicRenderResources
{
	uint32_t PosBufferIdx;
	uint32_t VertexBufferIdx;
};
#pragma endregion

namespace fe
{
	FE_DEFINE_LOG_CATEGORY(RendererInfo, ELogVerbosiy::Info)
	FE_DEFINE_LOG_CATEGORY(RendererWarn, ELogVerbosiy::Warning)
	FE_DEFINE_LOG_CATEGORY(RendererFatal, ELogVerbosiy::Fatal)

	Renderer::Renderer(const FrameManager& engineFrameManager, DeferredDeallocator& engineDefferedDeallocator, Window& window, RenderDevice& device, HandleManager& handleManager, GpuViewManager& gpuViewManager)
		: frameManager(engineFrameManager),
		  deferredDeallocator(engineDefferedDeallocator),
		  renderDevice(device),
		  handleManager(handleManager),
		  gpuViewManager(gpuViewManager),
		  directCmdQueue(std::make_unique<CommandQueue>(device.CreateCommandQueue("Main Gfx Queue", EQueueType::Direct).value())),
		  directCmdCtxPool(std::make_unique<CommandContextPool>(deferredDeallocator, device, EQueueType::Direct)),
		  swapchain(std::make_unique<Swapchain>(window, gpuViewManager, *directCmdQueue, NumFramesInFlight)),
		  tempConstantBufferAllocator(std::make_unique<TempConstantBufferAllocator>(frameManager, device, handleManager, gpuViewManager))
	{
		frameFences.reserve(NumFramesInFlight);
		for (uint8_t localFrameIdx = 0; localFrameIdx < NumFramesInFlight; ++localFrameIdx)
		{
			frameFences.emplace_back(device.CreateFence(std::format("FrameFence_{}", localFrameIdx)).value());
		}

#pragma region test
		bindlessRootSignature = std::make_unique<RootSignature>(device.CreateBindlessRootSignature().value());

		const ShaderCompileDesc vsDesc{
			.SourcePath = String("Assets/Shader/BasicVertexShader.hlsl"),
			.Type = EShaderType::Vertex,
			.OptimizationLevel = EShaderOptimizationLevel::None
		};

		const ShaderCompileDesc psDesc{
			.SourcePath = String("Assets/Shader/BasicPixelShader.hlsl"),
			.Type = EShaderType::Pixel
		};

		vs = std::make_unique<ShaderBlob>(vsDesc);
		ps = std::make_unique<ShaderBlob>(psDesc);

		GraphicsPipelineStateDesc psoDesc;
		psoDesc.SetVertexShader(*vs);
		psoDesc.SetPixelShader(*ps);
		psoDesc.SetRootSignature(*bindlessRootSignature);
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		pso = std::make_unique<PipelineState>(device.CreateGraphicsPipelineState(psoDesc).value());

		GPUTextureDesc depthStencilDesc;
		depthStencilDesc.DebugName = String("DepthStencilBufferTex");
		depthStencilDesc.AsDepthStencil(1280, 642, DXGI_FORMAT_D32_FLOAT);
		depthStencilBuffer = std::make_unique<GpuTexture>(device.CreateTexture(depthStencilDesc).value());
		dsv = gpuViewManager.RequestDepthStencilView(*depthStencilBuffer, { .MipSlice = 0 });

		/* #todo 배리어만 적용하는 방법 찾아보기
		* D3D12 WARNING: ID3D12CommandQueue::ExecuteCommandLists: ExecuteCommandLists references command lists that have recorded only Barrier commands. Since there is no other GPU work to synchronize against, all barriers should use AccessAfter / AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS and SyncBefore / SyncAfter = D3D12_BARRIER_SYNC_NONE. This information can be used as an optimization hint by some drivers. [ EXECUTION WARNING #1356: NON_OPTIMAL_BARRIER_ONLY_EXECUTE_COMMAND_LISTS]
		D3D12: **BREAK** enabled for the previous message, which was: [ WARNING EXECUTION #1356: NON_OPTIMAL_BARRIER_ONLY_EXECUTE_COMMAND_LISTS ]
		*/
		auto cmdCtx = directCmdCtxPool->Request();
		cmdCtx->Begin();
		{
			cmdCtx->AddPendingTextureBarrier(
				*depthStencilBuffer,
				D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_DEPTH_STENCIL,
				D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE,
				D3D12_BARRIER_LAYOUT_UNDEFINED, D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE);
			cmdCtx->FlushBarriers();

			tempConstantBufferAllocator->InitBufferStateTransition(*cmdCtx);
		}
		cmdCtx->End();

		directCmdQueue->AddPendingContext(*cmdCtx);
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
		tempConstantBufferAllocator->DeallocateCurrentFrame();
	}

	void Renderer::Render(World& world)
	{
		check(directCmdQueue);
		check(directCmdCtxPool);

		auto renderCmdCtx = directCmdCtxPool->Request();
		renderCmdCtx->Begin(pso.get());
		{
			auto bindlessDescriptorHeaps = gpuViewManager.GetBindlessDescriptorHeaps();
			renderCmdCtx->SetDescriptorHeaps(bindlessDescriptorHeaps);
			renderCmdCtx->SetRootSignature(*bindlessRootSignature);

			check(swapchain);
			GpuTexture&	   backBuffer = swapchain->GetBackBuffer();
			const GpuView& backBufferRTV = swapchain->GetRenderTargetView();
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

			size_t			  idx = 0;
			GpuBufferDesc posBufferDesc;
			posBufferDesc.AsConstantBuffer<PositionBuffer>();

			world.Each<StaticMeshComponent, fe::PositionComponent>([&posBufferDesc, &renderCmdCtx, &idx, this](StaticMeshComponent& staticMesh, PositionComponent& position) {
				renderCmdCtx->SetIndexBuffer(*staticMesh.IndexBufferHandle);
				{
					TempConstantBuffer posBuffer = tempConstantBufferAllocator->Allocate(posBufferDesc);
					std::memcpy(posBuffer.Mapping->MappedPtr, &position, sizeof(fe::PositionComponent));
					const BasicRenderResources params{
						.PosBufferIdx = posBuffer.View->Index,
						.VertexBufferIdx = staticMesh.VerticesBufferSRV->Index
					};
					renderCmdCtx->SetRoot32BitConstants(0, params, 0);
				}

				renderCmdCtx->DrawIndexed(staticMesh.NumIndices);
			});
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