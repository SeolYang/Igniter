#include <Igniter.h>
#include <Core/Window.h>
#include <Core/Log.h>
#include <Core/ContainerUtils.h>
#include <Core/Engine.h>
#include <D3D12/CommandContext.h>
#include <D3D12/DescriptorHeap.h>
#include <D3D12/GPUBuffer.h>
#include <D3D12/GPUBufferDesc.h>
#include <D3D12/GPUTexture.h>
#include <D3D12/GPUTextureDesc.h>
#include <D3D12/GPUView.h>
#include <D3D12/PipelineState.h>
#include <D3D12/PipelineStateDesc.h>
#include <D3D12/RenderDevice.h>
#include <D3D12/RootSignature.h>
#include <D3D12/ShaderBlob.h>
#include <Render/Utils.h>
#include <Render/Renderer.h>
#include <Render/RenderContext.h>
#include <Asset/StaticMesh.h>
#include <Asset/Material.h>
#include <Asset/AssetManager.h>
#include <Component/CameraComponent.h>
#include <Component/StaticMeshComponent.h>
#include <Component/TransformComponent.h>

struct BasicRenderResources
{
    uint32_t VertexBufferIdx;
    uint32_t PerFrameBufferIdx;
    uint32_t PerObjectBufferIdx;
    uint32_t DiffuseTexIdx;
    uint32_t DiffuseTexSamplerIdx;
};
#pragma endregion

IG_DEFINE_LOG_CATEGORY(Renderer);

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

    Renderer::Renderer(const FrameManager& frameManager, Window& window, RenderContext& renderContext)
        : frameManager(frameManager)
        , renderContext(renderContext)
        , tempConstantBufferAllocator(frameManager, renderContext)
        , mainViewport(window.GetViewport())
        , swapchain(window, renderContext, NumFramesInFlight)
    {
        RenderDevice& renderDevice = renderContext.GetRenderDevice();
#pragma region test
        bindlessRootSignature = MakePtr<RootSignature>(renderDevice.CreateBindlessRootSignature().value());

        const ShaderCompileDesc vsDesc{.SourcePath = String("Assets/Shader/BasicVertexShader.hlsl"),
            .Type = EShaderType::Vertex,
            .OptimizationLevel = EShaderOptimizationLevel::None};

        const ShaderCompileDesc psDesc{.SourcePath = String("Assets/Shader/BasicPixelShader.hlsl"), .Type = EShaderType::Pixel};

        vs = MakePtr<ShaderBlob>(vsDesc);
        ps = MakePtr<ShaderBlob>(psDesc);

        GraphicsPipelineStateDesc psoDesc;
        psoDesc.SetVertexShader(*vs);
        psoDesc.SetPixelShader(*ps);
        psoDesc.SetRootSignature(*bindlessRootSignature);
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        pso = MakePtr<PipelineState>(renderDevice.CreateGraphicsPipelineState(psoDesc).value());

        GpuTextureDesc depthStencilDesc;
        depthStencilDesc.DebugName = String("DepthStencilBufferTex");
        depthStencilDesc.AsDepthStencil(static_cast<uint32_t>(mainViewport.width), static_cast<uint32_t>(mainViewport.height), DXGI_FORMAT_D32_FLOAT);

        auto initialCmdCtx = renderContext.GetMainGfxCommandContextPool().Request("Initial Transition"_fs);
        initialCmdCtx->Begin();
        {
            for (const uint8_t localFrameIdx : views::iota(0Ui8, NumFramesInFlight))
            {
                depthStencils[localFrameIdx] = renderContext.CreateTexture(depthStencilDesc);
                dsvs[localFrameIdx] = renderContext.CreateDepthStencilView(depthStencils[localFrameIdx], D3D12_TEX2D_DSV{.MipSlice = 0});

                initialCmdCtx->AddPendingTextureBarrier(*renderContext.Lookup(depthStencils[localFrameIdx]), D3D12_BARRIER_SYNC_NONE,
                    D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_LAYOUT_UNDEFINED,
                    D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE);
            }

            initialCmdCtx->FlushBarriers();
        }
        initialCmdCtx->End();

        CommandContext* cmdCtxs[1]{initialCmdCtx.get()};
        CommandQueue& mainGfxQueue{renderContext.GetMainGfxQueue()};
        mainGfxQueue.ExecuteContexts(cmdCtxs);
#pragma endregion
    }

    Renderer::~Renderer()
    {
        CommandQueue& mainGfxQueue{renderContext.GetMainGfxQueue()};
        GpuSync mainGfxQueueSync = mainGfxQueue.MakeSync();
        mainGfxQueueSync.WaitOnCpu();

        for (const auto localFrameIdx : views::iota(0Ui8, NumFramesInFlight))
        {
            renderContext.DestroyGpuView(dsvs[localFrameIdx]);
            renderContext.DestroyTexture(depthStencils[localFrameIdx]);
        }
    }

    void Renderer::BeginFrame(const uint8_t localFrameIdx)
    {
        ZoneScoped;
        GpuSync& mainGfxLocalFrameSync = mainGfxFrameSyncs[localFrameIdx];
        if (mainGfxLocalFrameSync)
        {
            mainGfxLocalFrameSync.WaitOnCpu();
        }
        tempConstantBufferAllocator.BeginFrame(localFrameIdx);
    }

    void Renderer::Render(const uint8_t localFrameIdx, Registry& registry)
    {
        ZoneScoped;
        TempConstantBuffer2 perFrameConstantBuffer = tempConstantBufferAllocator.Allocate<PerFrameBuffer>();

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

        CommandQueue& mainGfxQueue{renderContext.GetMainGfxQueue()};
        auto renderCmdCtx = renderContext.GetMainGfxCommandContextPool().Request("MainGfx"_fs);
        renderCmdCtx->Begin(pso.get());
        {
            auto bindlessDescHeaps = renderContext.GetBindlessDescriptorHeaps();
            renderCmdCtx->SetDescriptorHeaps(bindlessDescHeaps);
            renderCmdCtx->SetRootSignature(*bindlessRootSignature);

            GpuTexture* backBufferPtr = renderContext.Lookup(swapchain.GetBackBuffer());
            const GpuView* backBufferRtvPtr = renderContext.Lookup(swapchain.GetRenderTargetView());
            renderCmdCtx->AddPendingTextureBarrier(*backBufferPtr, D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_RENDER_TARGET,
                D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_RENDER_TARGET, D3D12_BARRIER_LAYOUT_PRESENT, D3D12_BARRIER_LAYOUT_RENDER_TARGET);
            renderCmdCtx->FlushBarriers();

            renderCmdCtx->ClearRenderTarget(*backBufferRtvPtr);
            GpuView& dsv = *renderContext.Lookup(dsvs[localFrameIdx]);
            renderCmdCtx->ClearDepth(dsv);
            renderCmdCtx->SetRenderTarget(*backBufferRtvPtr, dsv);
            renderCmdCtx->SetViewport(mainViewport);
            renderCmdCtx->SetScissorRect(mainViewport);
            renderCmdCtx->SetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            AssetManager& assetManager = Igniter::GetAssetManager();

            const auto renderableView = registry.view<StaticMeshComponent, TransformComponent>();
            GpuView* perFrameCbvPtr = renderContext.Lookup(perFrameConstantBuffer.GetConstantBufferView());
            for (auto [entity, staticMeshComponent, transform] : renderableView.each())
            {
                if (staticMeshComponent.Mesh)
                {
                    StaticMesh* staticMeshPtr = assetManager.Lookup(staticMeshComponent.Mesh);
                    GpuBuffer* indexBufferPtr = renderContext.Lookup(staticMeshPtr->GetIndexBuffer());
                    GpuView* vertexBufferSrvPtr = renderContext.Lookup(staticMeshPtr->GetVertexBufferSrv());
                    renderCmdCtx->SetIndexBuffer(*indexBufferPtr);
                    {
                        TempConstantBuffer2 perObjectConstantBuffer = tempConstantBufferAllocator.Allocate<PerObjectBuffer>();
                        GpuView* perObjectCBViewPtr = renderContext.Lookup(perObjectConstantBuffer.GetConstantBufferView());
                        const auto perObjectBuffer = PerObjectBuffer{.LocalToWorld = ConvertToShaderSuitableForm(transform.CreateTransformation())};
                        perObjectConstantBuffer.Write(perObjectBuffer);

                        /* #sy_todo 각각의 Material이나 Diffuse가 Invalid 하다면 Engine Default로 fallback 될 수 있도록 조치 */
                        Material* materialPtr = assetManager.Lookup(staticMeshPtr->GetMaterial());
                        Texture* diffuseTexPtr = assetManager.Lookup(materialPtr->GetDiffuse());
                        GpuView* diffuseTexSrvPtr = renderContext.Lookup(diffuseTexPtr->GetShaderResourceView());
                        GpuView* diffuseTexSamplerPtr = renderContext.Lookup(diffuseTexPtr->GetSampler());

                        const BasicRenderResources params{.VertexBufferIdx = vertexBufferSrvPtr->Index,
                            .PerFrameBufferIdx = perFrameCbvPtr->Index,
                            .PerObjectBufferIdx = perObjectCBViewPtr->Index,
                            .DiffuseTexIdx = diffuseTexSrvPtr->Index,
                            .DiffuseTexSamplerIdx = diffuseTexSamplerPtr->Index};

                        renderCmdCtx->SetRoot32BitConstants(0, params, 0);
                    }

                    const StaticMesh::Desc& snapshot = staticMeshPtr->GetSnapshot();
                    const StaticMesh::LoadDesc& loadDesc = snapshot.LoadDescriptor;
                    renderCmdCtx->DrawIndexed(loadDesc.NumIndices);
                }
            }
        }
        renderCmdCtx->End();

        Ref<CommandContext> renderCmdCtxs[] = {*renderCmdCtx};
        mainGfxQueue.ExecuteContexts(renderCmdCtxs);
    }

    void Renderer::EndFrame(const uint8_t localFrameIdx)
    {
        ZoneScoped;
        CommandQueue& mainGfxQueue{renderContext.GetMainGfxQueue()};
        mainGfxFrameSyncs[localFrameIdx] = mainGfxQueue.MakeSync();
        swapchain.Present();
    }
}    // namespace ig
