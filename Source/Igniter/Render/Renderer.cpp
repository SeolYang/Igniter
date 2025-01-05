#include "Igniter/Igniter.h"
#include "Igniter/Core/Window.h"
#include "Igniter/Core/Log.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/D3D12/RootSignature.h"
#include "Igniter/D3D12/PipelineState.h"
#include "Igniter/D3D12/PipelineStateDesc.h"
#include "Igniter/D3D12/ShaderBlob.h"
#include "Igniter/D3D12/GpuBuffer.h"
#include "Igniter/D3D12/GpuTexture.h"
#include "Igniter/D3D12/CommandSignature.h"
#include "Igniter/D3D12/CommandSignatureDesc.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/Utils.h"
#include "Igniter/Render/TempConstantBufferAllocator.h"
#include "Igniter/Render/MeshStorage.h"
#include "Igniter/Render/SceneProxy.h"
#include "Igniter/Asset/AssetManager.h"
#include "Igniter/Gameplay/World.h"
#include "Igniter/Component/TransformComponent.h"
#include "Igniter/Component/CameraComponent.h"
#include "Igniter/Component/StaticMeshComponent.h"
#include "Igniter/Render/Renderer.h"

IG_DEFINE_LOG_CATEGORY(RendererLog);

namespace ig
{
    struct DrawOpaqueStaticMeshCommand
    {
        U32 PerFrameDataCbv = IG_NUMERIC_MAX_OF(PerFrameDataCbv);
        U32 StaticMeshDataIdx = IG_NUMERIC_MAX_OF(StaticMeshDataIdx);
        D3D12_DRAW_ARGUMENTS DrawIndexedArguments;
    };

    struct PerFrameBuffer
    {
        Matrix ViewProj{};
        Vector3 CameraPosition{};
        float Padding0;
        Vector3 CameraForward{};
        float Padding1;

        U32 StaticMeshVertexStorageSrv = IG_NUMERIC_MAX_OF(StaticMeshVertexStorageSrv);
        U32 VertexIndexStorageSrv = IG_NUMERIC_MAX_OF(VertexIndexStorageSrv);

        U32 TransformStorageSrv = IG_NUMERIC_MAX_OF(TransformStorageSrv);
        U32 MaterialStorageSrv = IG_NUMERIC_MAX_OF(MaterialStorageSrv);

        U32 StaticMeshStorageSrv = IG_NUMERIC_MAX_OF(StaticMeshStorageSrv);

        U32 RenderableStorageSrv = IG_NUMERIC_MAX_OF(RenderableStorageSrv);

        U32 RenderableIndicesBufferSrv = IG_NUMERIC_MAX_OF(RenderableIndicesBufferSrv);
        U32 NumMaxRenderables = IG_NUMERIC_MAX_OF(NumMaxRenderables);

        U32 PerFrameDataCbv = IG_NUMERIC_MAX_OF(PerFrameDataCbv);

        // TODO Light Sttorage/Indices/NumMaxLights
    };

    struct ComputeCullingConstants
    {
        U32 PerFrameDataCbv;
        U32 DrawOpaqueStaticMeshCmdBufferUav;
    };

    Renderer::Renderer(const Window& window, RenderContext& renderContext, const MeshStorage& meshStorage, const SceneProxy& sceneProxy) :
        window(&window), renderContext(&renderContext), meshStorage(&meshStorage), sceneProxy(&sceneProxy), mainViewport(window.GetViewport()), tempConstantBufferAllocator(MakePtr<TempConstantBufferAllocator>(renderContext))
    {
        GpuDevice& gpuDevice = renderContext.GetGpuDevice();
        bindlessRootSignature = MakePtr<RootSignature>(gpuDevice.CreateBindlessRootSignature().value());

        const ShaderCompileDesc vsDesc{
            .SourcePath = "Assets/Shaders/BasicVertexShader.hlsl"_fs,
            .Type = EShaderType::Vertex,
            .OptimizationLevel = EShaderOptimizationLevel::None};

        const ShaderCompileDesc psDesc{.SourcePath = "Assets/Shaders/BasicPixelShader.hlsl"_fs, .Type = EShaderType::Pixel};

        vs = MakePtr<ShaderBlob>(vsDesc);
        ps = MakePtr<ShaderBlob>(psDesc);

        GraphicsPipelineStateDesc psoDesc;
        psoDesc.SetVertexShader(*vs);
        psoDesc.SetPixelShader(*ps);
        psoDesc.SetRootSignature(*bindlessRootSignature);
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        pso = MakePtr<PipelineState>(gpuDevice.CreateGraphicsPipelineState(psoDesc).value());

        GpuTextureDesc depthStencilDesc;
        depthStencilDesc.DebugName = "DepthStencilBufferTex"_fs;
        depthStencilDesc.AsDepthStencil(static_cast<uint32_t>(mainViewport.width), static_cast<uint32_t>(mainViewport.height), DXGI_FORMAT_D32_FLOAT);

        auto initialCmdList = renderContext.GetMainGfxCommandListPool().Request(0, "Initial Transition"_fs);
        initialCmdList->Open();
        {
            for (const LocalFrameIndex localFrameIdx : views::iota(0Ui8, NumFramesInFlight))
            {
                depthStencils[localFrameIdx] = renderContext.CreateTexture(depthStencilDesc);
                dsvs[localFrameIdx] = renderContext.CreateDepthStencilView(depthStencils[localFrameIdx], D3D12_TEX2D_DSV{.MipSlice = 0});

                initialCmdList->AddPendingTextureBarrier(*renderContext.Lookup(depthStencils[localFrameIdx]),
                                                         D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_NONE,
                                                         D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_NO_ACCESS,
                                                         D3D12_BARRIER_LAYOUT_UNDEFINED, D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE);
            }

            initialCmdList->FlushBarriers();
        }
        initialCmdList->Close();

        CommandList* cmdLists[]{initialCmdList};
        CommandQueue& mainGfxQueue{renderContext.GetMainGfxQueue()};
        mainGfxQueue.ExecuteContexts(cmdLists);

        GpuBufferDesc uavCounterResetBufferDesc{};
        uavCounterResetBufferDesc.AsUploadBuffer(GpuBufferDesc::kUavCounterSize);
        uavCounterResetBufferDesc.DebugName = "UavCounterResetBuffer"_fs;
        uavCounterResetBuffer = MakePtr<GpuBuffer>(gpuDevice.CreateBuffer(uavCounterResetBufferDesc).value());
        auto const mappedUavCounterResetBuffer = reinterpret_cast<GpuBufferDesc::UavCounter*>(uavCounterResetBuffer->Map());
        ZeroMemory(mappedUavCounterResetBuffer, sizeof(GpuBufferDesc::kUavCounterSize));
        uavCounterResetBuffer->Unmap();

        const ShaderCompileDesc computeCullingShaderDesc{.SourcePath = "Assets/Shaders/ComputeCulling.hlsl"_fs, .Type = EShaderType::Compute};
        computeCullingShader = MakePtr<ShaderBlob>(computeCullingShaderDesc);

        ComputePipelineStateDesc computePipelineStateDesc{};
        computePipelineStateDesc.SetComputeShader(*computeCullingShader);
        computePipelineStateDesc.SetRootSignature(*bindlessRootSignature);
        computePipelineStateDesc.Name = "ComputeCullingPipelineState"_fs;
        computeCullingPso = MakePtr<PipelineState>(gpuDevice.CreateComputePipelineState(computePipelineStateDesc).value());
        IG_CHECK(computeCullingPso->IsCompute());

        CommandSignatureDesc commandSignatureDesc{};
        commandSignatureDesc.AddConstant(0, 0, 2)
            .AddDrawArgument()
            .SetCommandByteStride<DrawOpaqueStaticMeshCommand>();
        commandSignature = MakePtr<CommandSignature>(gpuDevice.CreateCommandSignature("IndirectCommandSignature", commandSignatureDesc, *bindlessRootSignature).value());

        for (const LocalFrameIndex localFrameIdx : views::iota(0Ui8, NumFramesInFlight))
        {
            drawOpaqueStaticMeshCmdStorage[localFrameIdx] = MakePtr<GpuStorage>(
                renderContext,
                String(std::format("DrawRenderableCmdBuffer.{}", localFrameIdx)),
                (U32)sizeof(DrawOpaqueStaticMeshCommand),
                kInitNumDrawCommands,
                true, true, true);
        }

        mainGfxQueue.MakeSyncPointWithSignal(renderContext.GetMainGfxFence()).WaitOnCpu();
    }

    Renderer::~Renderer()
    {
        for (const auto localFrameIdx : views::iota(0Ui8, NumFramesInFlight))
        {
            drawOpaqueStaticMeshCmdStorage[localFrameIdx]->ForceReset();
            renderContext->DestroyGpuView(dsvs[localFrameIdx]);
            renderContext->DestroyTexture(depthStencils[localFrameIdx]);
        }
    }

    void Renderer::PreRender(const LocalFrameIndex localFrameIdx)
    {
        tempConstantBufferAllocator->Reset(localFrameIdx);

        if (drawCmdSpace[localFrameIdx].IsValid())
        {
            drawOpaqueStaticMeshCmdStorage[localFrameIdx]->Deallocate(drawCmdSpace[localFrameIdx]);
            // 블럭을 합쳐서 fragmentation 최소화
            drawOpaqueStaticMeshCmdStorage[localFrameIdx]->ForceReset();
        }

        drawCmdSpace[localFrameIdx] =
            drawOpaqueStaticMeshCmdStorage[localFrameIdx]->Allocate(sceneProxy->GetNumMaxRenderables(localFrameIdx));
    }

    GpuSyncPoint Renderer::Render(const LocalFrameIndex localFrameIdx, const World& world, [[maybe_unused]] GpuSyncPoint sceneProxyRepSyncPoint)
    {
        ZoneScoped;

        IG_CHECK(renderContext != nullptr);
        IG_CHECK(meshStorage != nullptr);
        IG_CHECK(sceneProxy != nullptr);
        IG_CHECK(sceneProxyRepSyncPoint);

        // 프레임 마다 업데이트 되는 버퍼 데이터 업데이트
        PerFrameBuffer perFrameBuffer{};

        const Registry& registry = world.GetRegistry();
        const auto cameraView = registry.view<const CameraComponent, const TransformComponent>();
        bool bMainCameraExists = false;
        for (auto [entity, camera, transformComponent] : cameraView.each())
        {
            /* #sy_todo Multiple Camera, Render Target per camera */
            /* Column Vector: PVM; Row Vector: MVP  */
            if (!camera.bIsMainCamera)
            {
                continue;
            }

            if (camera.CameraViewport.width < 1.f || camera.CameraViewport.height < 1.f)
            {
                continue;
            }

            const Matrix viewMatrix = transformComponent.CreateView();
            const Matrix projMatrix = camera.CreatePerspective();
            perFrameBuffer.ViewProj = ConvertToShaderSuitableForm(viewMatrix * projMatrix);
            perFrameBuffer.CameraPosition = transformComponent.Position;
            perFrameBuffer.CameraForward = transformComponent.GetForwardDirection();
            bMainCameraExists = true;
            break;
        }

        if (!bMainCameraExists)
        {
            return GpuSyncPoint::Invalid();
        }

        const GpuView* staticMeshVertexStorageSrv = renderContext->Lookup(meshStorage->GetStaticMeshVertexStorageShaderResourceView());
        IG_CHECK(staticMeshVertexStorageSrv != nullptr && staticMeshVertexStorageSrv->IsValid());
        perFrameBuffer.StaticMeshVertexStorageSrv = staticMeshVertexStorageSrv->Index;

        const GpuView* vertexIndexStorageSrv = renderContext->Lookup(meshStorage->GetVertexIndexStorageShaderResourceView());
        IG_CHECK(vertexIndexStorageSrv != nullptr && vertexIndexStorageSrv->IsValid());
        perFrameBuffer.VertexIndexStorageSrv = vertexIndexStorageSrv->Index;

        const GpuView* transformStorageSrv = renderContext->Lookup(sceneProxy->GetTransformStorageShaderResourceView(localFrameIdx));
        IG_CHECK(transformStorageSrv != nullptr && transformStorageSrv->IsValid());
        perFrameBuffer.TransformStorageSrv = transformStorageSrv->Index;

        const GpuView* materialStorageSrv = renderContext->Lookup(sceneProxy->GetMaterialStorageShaderResourceView(localFrameIdx));
        IG_CHECK(materialStorageSrv != nullptr && materialStorageSrv->IsValid());
        perFrameBuffer.MaterialStorageSrv = materialStorageSrv->Index;

        const GpuView* staticMeshStorageSrv = renderContext->Lookup(sceneProxy->GetStaticMeshStorageSrv(localFrameIdx));
        IG_CHECK(staticMeshStorageSrv != nullptr && staticMeshStorageSrv->IsValid());
        perFrameBuffer.StaticMeshStorageSrv = staticMeshStorageSrv->Index;

        const GpuView* renderableStorageSrv = renderContext->Lookup(sceneProxy->GetRenderableStorageShaderResourceView(localFrameIdx));
        IG_CHECK(renderableStorageSrv != nullptr && renderableStorageSrv->IsValid());
        perFrameBuffer.RenderableStorageSrv = renderableStorageSrv->Index;

        const GpuView* renderableIndicesBufferSrv = renderContext->Lookup(sceneProxy->GetRenderableIndicesBufferShaderResourceView(localFrameIdx));
        IG_CHECK(renderableIndicesBufferSrv != nullptr && renderableIndicesBufferSrv->IsValid());
        perFrameBuffer.RenderableIndicesBufferSrv = renderableIndicesBufferSrv->Index;
        perFrameBuffer.NumMaxRenderables = sceneProxy->GetNumMaxRenderables(localFrameIdx);

        TempConstantBuffer perFrameConstantBuffer = tempConstantBufferAllocator->Allocate<PerFrameBuffer>(localFrameIdx);
        const GpuView* perFrameBufferCbv = renderContext->Lookup(perFrameConstantBuffer.GetConstantBufferView());
        IG_CHECK(perFrameBufferCbv != nullptr && perFrameBufferCbv->IsValid());
        perFrameBuffer.PerFrameDataCbv = perFrameBufferCbv->Index;

        perFrameConstantBuffer.Write(perFrameBuffer);

        GpuBuffer* drawOpaqueStaticMeshCmdBuffer = renderContext->Lookup(drawOpaqueStaticMeshCmdStorage[localFrameIdx]->GetGpuBuffer());
        IG_CHECK(drawOpaqueStaticMeshCmdBuffer != nullptr && drawOpaqueStaticMeshCmdBuffer->IsValid());
        auto bindlessDescHeaps = renderContext->GetBindlessDescriptorHeaps();
        CommandQueue& asyncComputeQueue{renderContext->GetAsyncComputeQueue()};
        {
            CommandListPool& asyncComputeCmdListPool{renderContext->GetAsyncComputeCommandListPool()};
            auto computeCullingCmdList = asyncComputeCmdListPool.Request(localFrameIdx, "ComputeCullingCmdList"_fs);
            computeCullingCmdList->Open(computeCullingPso.get());
            computeCullingCmdList->SetDescriptorHeaps(bindlessDescHeaps);
            computeCullingCmdList->SetRootSignature(*bindlessRootSignature);

            const GpuBufferDesc& drawOpaqueStaticMeshCmdBufferDesc = drawOpaqueStaticMeshCmdBuffer->GetDesc();
            IG_CHECK(drawOpaqueStaticMeshCmdBufferDesc.IsUavCounterEnabled());
            computeCullingCmdList->CopyBuffer(
                *uavCounterResetBuffer, 0, GpuBufferDesc::kUavCounterSize,
                *drawOpaqueStaticMeshCmdBuffer, drawOpaqueStaticMeshCmdBufferDesc.GetUavCounterOffset());

            computeCullingCmdList->AddPendingBufferBarrier(
                *drawOpaqueStaticMeshCmdBuffer,
                D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_SYNC_COMPUTE_SHADING,
                D3D12_BARRIER_ACCESS_COPY_DEST, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS);
            computeCullingCmdList->FlushBarriers();

            const GpuView* drawOpaqueStaticMeshCmd = renderContext->Lookup(drawOpaqueStaticMeshCmdStorage[localFrameIdx]->GetUnorderedResourceView());
            IG_CHECK(drawOpaqueStaticMeshCmd != nullptr && drawOpaqueStaticMeshCmd->IsValid());
            const ComputeCullingConstants computeCullingConstants{
                .PerFrameDataCbv = perFrameBuffer.PerFrameDataCbv,
                .DrawOpaqueStaticMeshCmdBufferUav = drawOpaqueStaticMeshCmd->Index};
            computeCullingCmdList->SetRoot32BitConstants(0, computeCullingConstants, 0);

            // todo Compute Shader 내부적으로 numthreads를 정하고, numElements%numThreads만큼의
            // 작업은 discard 하도록 하기
            constexpr U32 kNumThreads = 16;
            [[maybe_unused]] const U32 numThreadGroup = ((U32)drawCmdSpace[localFrameIdx].NumElements - 1) / kNumThreads + 1;
            computeCullingCmdList->Dispatch(numThreadGroup, 1, 1);
            computeCullingCmdList->Close();
            asyncComputeQueue.Wait(sceneProxyRepSyncPoint);

            CommandList* cmdLists[]{computeCullingCmdList};
            asyncComputeQueue.ExecuteContexts(cmdLists);
        }
        GpuSyncPoint computeCullingSync = asyncComputeQueue.MakeSyncPointWithSignal(renderContext->GetAsyncComputeFence());

        // Culling이 완료되면 Command Buffer를 가지고 ExecuteIndirect!
        Swapchain& swapchain = renderContext->GetSwapchain();
        GpuTexture* backBuffer = renderContext->Lookup(swapchain.GetBackBuffer());
        const GpuView* backBufferRtv = renderContext->Lookup(swapchain.GetRenderTargetView());

        CommandQueue& mainGfxQueue{renderContext->GetMainGfxQueue()};
        auto renderCmdList = renderContext->GetMainGfxCommandListPool().Request(localFrameIdx, "MainGfx"_fs);
        renderCmdList->Open(pso.get());
        renderCmdList->SetDescriptorHeaps(bindlessDescHeaps);
        renderCmdList->SetRootSignature(*bindlessRootSignature);

        renderCmdList->AddPendingTextureBarrier(*backBuffer,
                                                D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_RENDER_TARGET,
                                                D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_RENDER_TARGET,
                                                D3D12_BARRIER_LAYOUT_PRESENT, D3D12_BARRIER_LAYOUT_RENDER_TARGET);
        renderCmdList->FlushBarriers();

        renderCmdList->ClearRenderTarget(*backBufferRtv);
        GpuView* dsv = renderContext->Lookup(dsvs[localFrameIdx]);
        renderCmdList->ClearDepth(*dsv);
        renderCmdList->SetRenderTarget(*backBufferRtv, *dsv);
        renderCmdList->SetViewport(mainViewport);
        renderCmdList->SetScissorRect(mainViewport);
        renderCmdList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        renderCmdList->ExecuteIndirect(*commandSignature, *drawOpaqueStaticMeshCmdBuffer);

        renderCmdList->AddPendingTextureBarrier(*backBuffer,
                                                D3D12_BARRIER_SYNC_RENDER_TARGET, D3D12_BARRIER_SYNC_NONE,
                                                D3D12_BARRIER_ACCESS_RENDER_TARGET, D3D12_BARRIER_ACCESS_NO_ACCESS,
                                                D3D12_BARRIER_LAYOUT_RENDER_TARGET, D3D12_BARRIER_LAYOUT_PRESENT);
        renderCmdList->FlushBarriers();

        renderCmdList->Close();
        CommandList* renderCmdLists[]{renderCmdList};
        mainGfxQueue.Wait(computeCullingSync);
        mainGfxQueue.ExecuteContexts(renderCmdLists);
        return mainGfxQueue.MakeSyncPointWithSignal(renderContext->GetMainGfxFence());
    }
} // namespace ig
