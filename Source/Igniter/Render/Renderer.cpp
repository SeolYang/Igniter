#include <Render/Renderer.h>
#include <D3D12/RenderDevice.h>
#include <D3D12/CommandContext.h>
#include <D3D12/DescriptorHeap.h>
#include <D3D12/GPUBuffer.h>
#include <D3D12/GPUBufferDesc.h>
#include <D3D12/ShaderBlob.h>
#include <D3D12/PipelineState.h>
#include <D3D12/PipelineStateDesc.h>
#include <D3D12/RootSignature.h>
#include <D3D12/GPUTexture.h>
#include <D3D12/GPUTextureDesc.h>
#include <D3D12/GPUView.h>
#include <Render/GPUViewManager.h>
#include <Render/StaticMeshComponent.h>
#include <Render/TransformComponent.h>
#include <Render/CameraComponent.h>
#include <Core/Window.h>
#include <Math/Common.h>
#include <Core/Igniter.h>

struct BasicRenderResources
{
    uint32_t VertexBufferIdx;
    uint32_t PerFrameBufferIdx;
    uint32_t PerObjectBufferIdx;
    uint32_t DiffuseTexIdx;
    uint32_t DiffuseTexSamplerIdx;
};
#pragma endregion

namespace ig
{
    struct PerFrameBuffer
    {
        Matrix ViewProj{};
    };

    struct PerObjectBuffer
    {
        Matrix LocalToWorld{};
    };

    IG_DEFINE_LOG_CATEGORY(RendererInfo, ELogVerbosity::Info)
    IG_DEFINE_LOG_CATEGORY(RendererWarn, ELogVerbosity::Warning)
    IG_DEFINE_LOG_CATEGORY(RendererFatal, ELogVerbosity::Fatal)

    Renderer::Renderer(const FrameManager& engineFrameManager, DeferredDeallocator& engineDefferedDeallocator, Window& window, RenderDevice& device, HandleManager& handleManager, GpuViewManager& gpuViewManager)
        : frameManager(engineFrameManager),
          deferredDeallocator(engineDefferedDeallocator),
          renderDevice(device),
          handleManager(handleManager),
          gpuViewManager(gpuViewManager),
          mainViewport(window.GetViewport()),
          mainGfxQueue(device.CreateCommandQueue("Main Gfx Queue", EQueueType::Direct).value()),
          gfxCmdCtxPool(deferredDeallocator, device, EQueueType::Direct),
          swapchain(window, gpuViewManager, mainGfxQueue, NumFramesInFlight),
          tempConstantBufferAllocator(frameManager, device, handleManager, gpuViewManager)
    {
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
        depthStencilDesc.AsDepthStencil(static_cast<uint32_t>(mainViewport.width), static_cast<uint32_t>(mainViewport.height), DXGI_FORMAT_D32_FLOAT);
        depthStencilBuffer = std::make_unique<GpuTexture>(device.CreateTexture(depthStencilDesc).value());
        dsv = gpuViewManager.RequestDepthStencilView(*depthStencilBuffer, D3D12_TEX2D_DSV{ .MipSlice = 0 });

        auto cmdCtx = gfxCmdCtxPool.Submit();
        cmdCtx->Begin();
        {
            cmdCtx->AddPendingTextureBarrier(
                *depthStencilBuffer,
                D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_DEPTH_STENCIL,
                D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE,
                D3D12_BARRIER_LAYOUT_UNDEFINED, D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE);
            cmdCtx->FlushBarriers();
        }
        cmdCtx->End();

        mainGfxQueue.AddPendingContext(*cmdCtx);
#pragma endregion
    }

    Renderer::~Renderer()
    {
        GpuSync mainGfxQueueSync = mainGfxQueue.Flush();
        mainGfxQueueSync.WaitOnCpu();
    }

    void Renderer::BeginFrame()
    {
        GpuSync& mainGfxLocalFrameSync = mainGfxFrameSyncs[frameManager.GetLocalFrameIndex()];
        if (mainGfxLocalFrameSync)
        {
            mainGfxLocalFrameSync.WaitOnCpu();
        }

        tempConstantBufferAllocator.DeallocateCurrentFrame();
    }

    void Renderer::Render(Registry& registry)
    {
        IG_CHECK(mainGfxQueue);

        TempConstantBuffer perFrameConstantBuffer = tempConstantBufferAllocator.Allocate<PerFrameBuffer>();

        PerFrameBuffer perFrameBuffer{};
        auto cameraView = registry.view<CameraComponent, TransformComponent, MainCameraTag>();
        IG_CHECK(cameraView.size_hint() == 1);
        for (auto [entity, camera, transformData] : cameraView.each())
        {
            /* #sy_todo Multiple Camera, Render Target per camera */
            /* Column Vector: PVM; Row Vector: MVP  */
            const Matrix viewMatrix = transformData.CreateView();
            const Matrix projMatrix = camera.CreatePerspective();
            perFrameBuffer.ViewProj = ConvertToShaderSuitableForm(viewMatrix * projMatrix);
        }
        perFrameConstantBuffer.Write(perFrameBuffer);

        auto renderCmdCtx = gfxCmdCtxPool.Submit();
        renderCmdCtx->Begin(pso.get());
        {
            auto bindlessDescriptorHeaps = gpuViewManager.GetBindlessDescriptorHeaps();
            renderCmdCtx->SetDescriptorHeaps(bindlessDescriptorHeaps);
            renderCmdCtx->SetRootSignature(*bindlessRootSignature);

            GpuTexture& backBuffer = swapchain.GetBackBuffer();
            const GpuView& backBufferRTV = swapchain.GetRenderTargetView();
            renderCmdCtx->AddPendingTextureBarrier(backBuffer,
                                                   D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_RENDER_TARGET,
                                                   D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_RENDER_TARGET,
                                                   D3D12_BARRIER_LAYOUT_PRESENT, D3D12_BARRIER_LAYOUT_RENDER_TARGET);
            renderCmdCtx->FlushBarriers();

            renderCmdCtx->ClearRenderTarget(backBufferRTV);
            renderCmdCtx->ClearDepth(*dsv);
            renderCmdCtx->SetRenderTarget(backBufferRTV, *dsv);
            renderCmdCtx->SetViewport(mainViewport);
            renderCmdCtx->SetScissorRect(mainViewport);
            renderCmdCtx->SetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            const auto renderableView = registry.view<StaticMeshComponent, TransformComponent>();
            for (auto [entity, staticMesh, transform] : renderableView.each())
            {
                renderCmdCtx->SetIndexBuffer(*staticMesh.IndexBufferHandle);
                {
                    TempConstantBuffer perObjectConstantBuffer = tempConstantBufferAllocator.Allocate<PerObjectBuffer>();
                    const auto perObjectBuffer = PerObjectBuffer{ .LocalToWorld = ConvertToShaderSuitableForm(transform.CreateTransformation()) };
                    perObjectConstantBuffer.Write(perObjectBuffer);

                    const BasicRenderResources params{
                        .VertexBufferIdx = staticMesh.VerticesBufferSRV->Index,
                        .PerFrameBufferIdx = perFrameConstantBuffer.View->Index,
                        .PerObjectBufferIdx = perObjectConstantBuffer.View->Index,
                        .DiffuseTexIdx = staticMesh.DiffuseTex->Index,
                        .DiffuseTexSamplerIdx = staticMesh.DiffuseTexSampler->Index
                    };

                    renderCmdCtx->SetRoot32BitConstants(0, params, 0);
                }

                renderCmdCtx->DrawIndexed(staticMesh.NumIndices);
            }
        }
        renderCmdCtx->End();
        mainGfxQueue.AddPendingContext(*renderCmdCtx);
    }

    void Renderer::EndFrame()
    {
        mainGfxFrameSyncs[frameManager.GetLocalFrameIndex()] = mainGfxQueue.Submit();
        swapchain.Present();
    }

    void Renderer::FlushQueues()
    {
        GpuSync mainGfxSync = mainGfxQueue.Flush();
        mainGfxSync.WaitOnCpu();
    }
} // namespace ig