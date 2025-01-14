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

IG_DECLARE_LOG_CATEGORY(RendererLog);
IG_DEFINE_LOG_CATEGORY(RendererLog);

namespace ig
{
    struct DrawOpaqueStaticMeshCommand
    {
        U32 PerFrameDataCbv = IG_NUMERIC_MAX_OF(PerFrameDataCbv);
        U32 InstancingId = IG_NUMERIC_MAX_OF(InstancingId);
        U32 MaterialIdx = IG_NUMERIC_MAX_OF(MaterialIdx);
        U32 VertexOffset = IG_NUMERIC_MAX_OF(VertexOffset);
        U32 VertexIdxStorageOffset = IG_NUMERIC_MAX_OF(VertexIdxStorageOffset);
        U32 TransformIdxStorageOffset = IG_NUMERIC_MAX_OF(TransformIdxStorageOffset);
        D3D12_DRAW_ARGUMENTS DrawIndexedArguments;
    };

    // 나중에 여러 카메라에 대해 렌더링하게 되면 Per Frame은 아닐수도?
    struct PerFrameBuffer
    {
        Matrix View{};
        Matrix ViewProj{};

        // [1]: Real-Time Rendering 4th ed. 22.14.1 Frustum Plane Extraction
        // [2]: https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
        // [3]: https://iquilezles.org/articles/frustum/
        // (CameraPos.xyz, InverseAspectRatio)
        Vector4 CamPosInvAspectRatio{};
        // (cos(fovy*0.5f), sin(fovy*0.5f), Near, Far)
        // View Frustum (in view space) =>
        // r = 1/aspect_ratio
        // Left = (r*cos, 0, sin, 0)
        // Right = (-r*cos, 0, sin, 0)
        // Bottom = (0, cos, sin, 0)
        // Top = (0, -cos, sin, 0)
        // Near = (0, 0, 1, -N)
        // Far = (0, 0, -1, F)
        Vector4 ViewFrustumParams{};

        U32 StaticMeshVertexStorageSrv = IG_NUMERIC_MAX_OF(StaticMeshVertexStorageSrv);
        U32 VertexIndexStorageSrv = IG_NUMERIC_MAX_OF(VertexIndexStorageSrv);

        U32 TransformStorageSrv = IG_NUMERIC_MAX_OF(TransformStorageSrv);
        U32 MaterialStorageSrv = IG_NUMERIC_MAX_OF(MaterialStorageSrv);
        U32 MeshStorageSrv = IG_NUMERIC_MAX_OF(MeshStorageSrv);

        U32 InstancingDataStorageSrv = IG_NUMERIC_MAX_OF(InstancingDataStorageSrv);
        U32 InstancingDataStorageUav = IG_NUMERIC_MAX_OF(InstancingDataStorageUav);

        U32 TransformIdxStorageSrv = IG_NUMERIC_MAX_OF(TransformIdxStorageSrv);
        U32 TransformIdxStorageUav = IG_NUMERIC_MAX_OF(TransformIdxStorageUav);

        U32 RenderableStorageSrv = IG_NUMERIC_MAX_OF(RenderableStorageSrv);

        U32 RenderableIndicesBufferSrv = IG_NUMERIC_MAX_OF(RenderableIndicesBufferSrv);
        U32 NumMaxRenderables = IG_NUMERIC_MAX_OF(NumMaxRenderables);

        U32 PerFrameDataCbv = IG_NUMERIC_MAX_OF(PerFrameDataCbv);
        // TODO Light Sttorage/Indices/NumMaxLights

        U32 Padding[3];
    };

    struct ComputeCullingConstants
    {
        U32 PerFrameDataCbv;
    };

    struct GenDrawCmdConstants
    {
        U32 PerFrameDataCbv;
        U32 DrawOpaqueStaticMeshCmdBufferUav;
    };

    Renderer::Renderer(const Window& window, RenderContext& renderContext, const MeshStorage& meshStorage, const SceneProxy& sceneProxy)
        : window(&window)
        , renderContext(&renderContext)
        , meshStorage(&meshStorage)
        , sceneProxy(&sceneProxy)
        , mainViewport(window.GetViewport())
        , tempConstantBufferAllocator(MakePtr<TempConstantBufferAllocator>(renderContext))
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
        mainGfxQueue.ExecuteCommandLists(cmdLists);

        GpuBufferDesc uavCounterResetBufferDesc{};
        uavCounterResetBufferDesc.AsUploadBuffer(GpuBufferDesc::kUavCounterSize);
        uavCounterResetBufferDesc.DebugName = "UavCounterResetBuffer"_fs;
        uavCounterResetBuffer = MakePtr<GpuBuffer>(gpuDevice.CreateBuffer(uavCounterResetBufferDesc).value());
        auto const mappedUavCounterResetBuffer = reinterpret_cast<GpuBufferDesc::UavCounter*>(uavCounterResetBuffer->Map());
        ZeroMemory(mappedUavCounterResetBuffer, sizeof(GpuBufferDesc::kUavCounterSize));
        uavCounterResetBuffer->Unmap();

        const ShaderCompileDesc computeCullingShaderDesc{.SourcePath = "Assets/Shaders/ComputeCulling.hlsl"_fs, .Type = EShaderType::Compute};
        computeCullingShader = MakePtr<ShaderBlob>(computeCullingShaderDesc);

        ComputePipelineStateDesc computeCullingPsoDesc{};
        computeCullingPsoDesc.SetComputeShader(*computeCullingShader);
        computeCullingPsoDesc.SetRootSignature(*bindlessRootSignature);
        computeCullingPsoDesc.Name = "ComputeCullingPipelineState"_fs;
        computeCullingPso = MakePtr<PipelineState>(gpuDevice.CreateComputePipelineState(computeCullingPsoDesc).value());
        IG_CHECK(computeCullingPso->IsCompute());

        const ShaderCompileDesc genDrawCmdsShaderDesc{.SourcePath = "Assets/Shaders/GenDrawCommands.hlsl"_fs, .Type = EShaderType::Compute};
        genDrawCmdsShader = MakePtr<ShaderBlob>(genDrawCmdsShaderDesc);
        ComputePipelineStateDesc genDrawCmdPsoDesc{};
        genDrawCmdPsoDesc.SetComputeShader(*genDrawCmdsShader);
        genDrawCmdPsoDesc.SetRootSignature(*bindlessRootSignature);
        genDrawCmdPsoDesc.Name = "GenDrawCmdsPso"_fs;
        genDrawCmdsPso = MakePtr<PipelineState>(gpuDevice.CreateComputePipelineState(genDrawCmdPsoDesc).value());
        IG_CHECK(genDrawCmdsPso->IsCompute());

        CommandSignatureDesc commandSignatureDesc{};
        commandSignatureDesc.AddConstant(0, 0, 6)
            .AddDrawArgument()
            .SetCommandByteStride<DrawOpaqueStaticMeshCommand>();
        commandSignature = MakePtr<CommandSignature>(gpuDevice.CreateCommandSignature("IndirectCommandSignature", commandSignatureDesc, *bindlessRootSignature).value());

        for (const LocalFrameIndex localFrameIdx : views::iota(0Ui8, NumFramesInFlight))
        {
            drawOpaqueStaticMeshCmdStorage[localFrameIdx] = MakePtr<GpuStorage>(
                renderContext,
                GpuStorageDesc{
                    .DebugName = String(std::format("DrawRenderableCmdBuffer.{}", localFrameIdx)),
                    .ElementSize = (U32)sizeof(DrawOpaqueStaticMeshCommand),
                    .NumInitElements = kInitNumDrawCommands,
                    .Flags =
                        EGpuStorageFlags::ShaderReadWrite |
                        EGpuStorageFlags::EnableUavCounter |
                        EGpuStorageFlags::EnableLinearAllocation});
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
            drawCmdSpace[localFrameIdx] = {};
            // 블럭을 합쳐서 fragmentation 최소화
            drawOpaqueStaticMeshCmdStorage[localFrameIdx]->ForceReset();
        }

        const Size numRequiredCmds = sceneProxy->GetMaxNumRenderables(localFrameIdx);
        if (numRequiredCmds > 0)
        {
            drawCmdSpace[localFrameIdx] =
                drawOpaqueStaticMeshCmdStorage[localFrameIdx]->Allocate(numRequiredCmds);
        }
    }

    GpuSyncPoint Renderer::Render(const LocalFrameIndex localFrameIdx, const World& world, [[maybe_unused]] GpuSyncPoint sceneProxyRepSyncPoint)
    {
        ZoneScoped;

        IG_CHECK(renderContext != nullptr);
        IG_CHECK(meshStorage != nullptr);
        IG_CHECK(sceneProxy != nullptr);

        // 프레임 마다 업데이트 되는 버퍼 데이터 업데이트
        PerFrameBuffer perFrameBuffer{};

        const Registry& registry = world.GetRegistry();
        const auto cameraView = registry.view<const CameraComponent, const TransformComponent>();
        bool bMainCameraExists = false;
        for (auto [entity, camera, transformComponent] : cameraView.each())
        {
            /* #sy_todo Multiple Camera, OnImGui Target per camera */
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
            perFrameBuffer.View = ConvertToShaderSuitableForm(viewMatrix);
            perFrameBuffer.ViewProj = ConvertToShaderSuitableForm(viewMatrix * projMatrix);
            perFrameBuffer.CamPosInvAspectRatio = Vector4{
                transformComponent.Position.x,
                transformComponent.Position.y,
                transformComponent.Position.z,
                1.f/camera.CameraViewport.AspectRatio()};

            const float halfFovYRad = Deg2Rad(camera.Fov * 0.5f);
            const float cosHalfFovY{std::cosf(halfFovYRad)};
            const float sinHalfFovY{std::sinf(halfFovYRad)};
            perFrameBuffer.ViewFrustumParams = Vector4{
                cosHalfFovY, sinHalfFovY,
                camera.NearZ, camera.FarZ};

            perFrameBuffer.Padding[0] = camera.bEnableFrustumCull;
            bMainCameraExists = true;
            break;
        }

        if (!bMainCameraExists)
        {
            return GpuSyncPoint::Invalid();
        }

        Swapchain& swapchain = renderContext->GetSwapchain();
        GpuTexture* backBuffer = renderContext->Lookup(swapchain.GetBackBuffer());
        const GpuView* backBufferRtv = renderContext->Lookup(swapchain.GetRenderTargetView());

        CommandQueue& mainGfxQueue{renderContext->GetMainGfxQueue()};
        CommandListPool& mainGfxCmdListPool = renderContext->GetMainGfxCommandListPool();
        auto renderCmdList = mainGfxCmdListPool.Request(localFrameIdx, "MainGfxRender"_fs);
        if (sceneProxy->GetMaxNumRenderables(localFrameIdx) == 0)
        {
            renderCmdList->Open(pso.get());

            renderCmdList->AddPendingTextureBarrier(
                *backBuffer,
                D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_RENDER_TARGET,
                D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_RENDER_TARGET,
                D3D12_BARRIER_LAYOUT_PRESENT, D3D12_BARRIER_LAYOUT_RENDER_TARGET);
            renderCmdList->FlushBarriers();

            renderCmdList->ClearRenderTarget(*backBufferRtv);
            GpuView* dsv = renderContext->Lookup(dsvs[localFrameIdx]);
            renderCmdList->ClearDepth(*dsv);
            renderCmdList->AddPendingTextureBarrier(
                *backBuffer,
                D3D12_BARRIER_SYNC_RENDER_TARGET, D3D12_BARRIER_SYNC_NONE,
                D3D12_BARRIER_ACCESS_RENDER_TARGET, D3D12_BARRIER_ACCESS_NO_ACCESS,
                D3D12_BARRIER_LAYOUT_RENDER_TARGET, D3D12_BARRIER_LAYOUT_PRESENT);
            renderCmdList->FlushBarriers();

            renderCmdList->Close();
            CommandList* renderCmdLists[]{renderCmdList};
            mainGfxQueue.ExecuteCommandLists(renderCmdLists);
            return mainGfxQueue.MakeSyncPointWithSignal(renderContext->GetMainGfxFence());
        }

        const GpuView* staticMeshVertexStorageSrv = renderContext->Lookup(meshStorage->GetStaticMeshVertexStorageSrv());
        IG_CHECK(staticMeshVertexStorageSrv != nullptr && staticMeshVertexStorageSrv->IsValid());
        perFrameBuffer.StaticMeshVertexStorageSrv = staticMeshVertexStorageSrv->Index;

        const GpuView* vertexIndexStorageSrv = renderContext->Lookup(meshStorage->GetIndexStorageSrv());
        IG_CHECK(vertexIndexStorageSrv != nullptr && vertexIndexStorageSrv->IsValid());
        perFrameBuffer.VertexIndexStorageSrv = vertexIndexStorageSrv->Index;

        const GpuView* transformStorageSrv = renderContext->Lookup(sceneProxy->GetTransformProxyStorageSrv(localFrameIdx));
        IG_CHECK(transformStorageSrv != nullptr && transformStorageSrv->IsValid());
        perFrameBuffer.TransformStorageSrv = transformStorageSrv->Index;

        const GpuView* materialStorageSrv = renderContext->Lookup(sceneProxy->GetMaterialProxyStorageSrv(localFrameIdx));
        IG_CHECK(materialStorageSrv != nullptr && materialStorageSrv->IsValid());
        perFrameBuffer.MaterialStorageSrv = materialStorageSrv->Index;

        const GpuView* meshProxyStorageSrv = renderContext->Lookup(sceneProxy->GetMeshProxySrv(localFrameIdx));
        IG_CHECK(meshProxyStorageSrv != nullptr && meshProxyStorageSrv->IsValid());
        perFrameBuffer.MeshStorageSrv = meshProxyStorageSrv->Index;

        const GpuView* instancingStorageSrv = renderContext->Lookup(sceneProxy->GetInstancingStorageSrv(localFrameIdx));
        IG_CHECK(instancingStorageSrv != nullptr && instancingStorageSrv->IsValid());
        perFrameBuffer.InstancingDataStorageSrv = instancingStorageSrv->Index;

        const GpuView* instancingStorageUav = renderContext->Lookup(sceneProxy->GetInstancingStorageUav(localFrameIdx));
        IG_CHECK(instancingStorageUav != nullptr && instancingStorageUav->IsValid());
        perFrameBuffer.InstancingDataStorageUav = instancingStorageUav->Index;

        const GpuView* transformIdxStorageSrv = renderContext->Lookup(sceneProxy->GetTransformIndexStorageSrv(localFrameIdx));
        IG_CHECK(transformIdxStorageSrv != nullptr && transformIdxStorageSrv->IsValid());
        perFrameBuffer.TransformIdxStorageSrv = transformIdxStorageSrv->Index;

        const GpuView* transformIdxStorageUav = renderContext->Lookup(sceneProxy->GetTransformIndexStorageUav(localFrameIdx));
        IG_CHECK(transformIdxStorageUav != nullptr && transformIdxStorageUav->IsValid());
        perFrameBuffer.TransformIdxStorageUav = transformIdxStorageUav->Index;

        const GpuView* renderableStorageSrv = renderContext->Lookup(sceneProxy->GetRenderableProxyStorageSrv(localFrameIdx));
        IG_CHECK(renderableStorageSrv != nullptr && renderableStorageSrv->IsValid());
        perFrameBuffer.RenderableStorageSrv = renderableStorageSrv->Index;

        const GpuView* renderableIndicesBufferSrv = renderContext->Lookup(sceneProxy->GetRenderableIndicesSrv(localFrameIdx));
        IG_CHECK(renderableIndicesBufferSrv != nullptr && renderableIndicesBufferSrv->IsValid());
        perFrameBuffer.RenderableIndicesBufferSrv = renderableIndicesBufferSrv->Index;
        perFrameBuffer.NumMaxRenderables = sceneProxy->GetMaxNumRenderables(localFrameIdx);

        TempConstantBuffer perFrameConstantBuffer = tempConstantBufferAllocator->Allocate<PerFrameBuffer>(localFrameIdx);
        const GpuView* perFrameBufferCbv = renderContext->Lookup(perFrameConstantBuffer.GetConstantBufferView());
        IG_CHECK(perFrameBufferCbv != nullptr && perFrameBufferCbv->IsValid());
        perFrameBuffer.PerFrameDataCbv = perFrameBufferCbv->Index;
        perFrameConstantBuffer.Write(perFrameBuffer);

        GpuBuffer* drawOpaqueStaticMeshCmdBuffer = renderContext->Lookup(drawOpaqueStaticMeshCmdStorage[localFrameIdx]->GetGpuBuffer());
        IG_CHECK(drawOpaqueStaticMeshCmdBuffer != nullptr && drawOpaqueStaticMeshCmdBuffer->IsValid());
        const GpuBufferDesc& drawOpaqueStaticMeshCmdBufferDesc = drawOpaqueStaticMeshCmdBuffer->GetDesc();
        IG_CHECK(drawOpaqueStaticMeshCmdBufferDesc.IsUavCounterEnabled());
        GpuBuffer* instancingStorageBuffer = renderContext->Lookup(sceneProxy->GetInstancingStorageBuffer(localFrameIdx));
        IG_CHECK(instancingStorageBuffer != nullptr && instancingStorageBuffer->IsValid());
        GpuBuffer* transformIdxStorageBuffer = renderContext->Lookup(sceneProxy->GetTransformIndexStorageBuffer(localFrameIdx));
        IG_CHECK(transformIdxStorageBuffer != nullptr && transformIdxStorageBuffer->IsValid());
        auto bindlessDescHeaps = renderContext->GetBindlessDescriptorHeaps();
        CommandQueue& asyncComputeQueue{renderContext->GetAsyncComputeQueue()};
        CommandListPool& asyncComputeCmdListPool{renderContext->GetAsyncComputeCommandListPool()};

        {
            auto computeCullingCmdList = asyncComputeCmdListPool.Request(localFrameIdx, "ComputeCullingCmdList"_fs);
            computeCullingCmdList->Open(computeCullingPso.get());
            computeCullingCmdList->SetDescriptorHeaps(bindlessDescHeaps);
            computeCullingCmdList->SetRootSignature(*bindlessRootSignature);

            computeCullingCmdList->AddPendingBufferBarrier(
                *instancingStorageBuffer,
                D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COMPUTE_SHADING,
                D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS);
            computeCullingCmdList->AddPendingBufferBarrier(
                *transformIdxStorageBuffer,
                D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COMPUTE_SHADING,
                D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS);
            computeCullingCmdList->FlushBarriers();

            const ComputeCullingConstants computeCullingConstants{.PerFrameDataCbv = perFrameBuffer.PerFrameDataCbv};
            computeCullingCmdList->SetRoot32BitConstants(0, computeCullingConstants, 0);

            constexpr U32 kNumThreads = 16;
            const Size numRenderables = sceneProxy->GetMaxNumRenderables(localFrameIdx);
            const U32 numThreadGroup = ((U32)numRenderables - 1) / kNumThreads + 1;
            computeCullingCmdList->Dispatch(numThreadGroup, 1, 1);

            computeCullingCmdList->AddPendingBufferBarrier(
                *instancingStorageBuffer,
                D3D12_BARRIER_SYNC_COMPUTE_SHADING, D3D12_BARRIER_SYNC_NONE,
                D3D12_BARRIER_ACCESS_UNORDERED_ACCESS, D3D12_BARRIER_ACCESS_NO_ACCESS);
            computeCullingCmdList->AddPendingBufferBarrier(
                *transformIdxStorageBuffer,
                D3D12_BARRIER_SYNC_COMPUTE_SHADING, D3D12_BARRIER_SYNC_NONE,
                D3D12_BARRIER_ACCESS_UNORDERED_ACCESS, D3D12_BARRIER_ACCESS_NO_ACCESS);
            computeCullingCmdList->FlushBarriers();

            computeCullingCmdList->Close();

            asyncComputeQueue.Wait(sceneProxyRepSyncPoint);
            CommandList* cmdLists[]{computeCullingCmdList};
            asyncComputeQueue.ExecuteCommandLists(cmdLists);
        }
        GpuSyncPoint computeCullingSync = asyncComputeQueue.MakeSyncPointWithSignal(renderContext->GetAsyncComputeFence());

        {
            auto genDrawCmdsCmdList = asyncComputeCmdListPool.Request(localFrameIdx, "GenDrawCmdCmdList"_fs);
            genDrawCmdsCmdList->Open(genDrawCmdsPso.get());
            genDrawCmdsCmdList->SetDescriptorHeaps(bindlessDescHeaps);
            genDrawCmdsCmdList->SetRootSignature(*bindlessRootSignature);

            genDrawCmdsCmdList->AddPendingBufferBarrier(
                *uavCounterResetBuffer,
                D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COPY,
                D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_COPY_SOURCE);
            genDrawCmdsCmdList->AddPendingBufferBarrier(
                *drawOpaqueStaticMeshCmdBuffer,
                D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COPY,
                D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_COPY_DEST);
            genDrawCmdsCmdList->FlushBarriers();

            genDrawCmdsCmdList->CopyBuffer(
                *uavCounterResetBuffer, 0, GpuBufferDesc::kUavCounterSize,
                *drawOpaqueStaticMeshCmdBuffer, drawOpaqueStaticMeshCmdBufferDesc.GetUavCounterOffset());

            genDrawCmdsCmdList->AddPendingBufferBarrier(
                *uavCounterResetBuffer,
                D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_SYNC_NONE,
                D3D12_BARRIER_ACCESS_COPY_SOURCE, D3D12_BARRIER_ACCESS_NO_ACCESS);
            genDrawCmdsCmdList->AddPendingBufferBarrier(
                *drawOpaqueStaticMeshCmdBuffer,
                D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_SYNC_COMPUTE_SHADING,
                D3D12_BARRIER_ACCESS_COPY_DEST, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS);
            genDrawCmdsCmdList->AddPendingBufferBarrier(
                *instancingStorageBuffer,
                D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COMPUTE_SHADING,
                D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS);
            genDrawCmdsCmdList->FlushBarriers();

            const GpuView* drawOpaqueStaticMeshCmd = renderContext->Lookup(drawOpaqueStaticMeshCmdStorage[localFrameIdx]->GetUnorderedResourceView());
            IG_CHECK(drawOpaqueStaticMeshCmd != nullptr && drawOpaqueStaticMeshCmd->IsValid());
            const GenDrawCmdConstants genDrawCmdConstants{.PerFrameDataCbv = perFrameBuffer.PerFrameDataCbv, .DrawOpaqueStaticMeshCmdBufferUav = drawOpaqueStaticMeshCmd->Index};
            genDrawCmdsCmdList->SetRoot32BitConstants(0, genDrawCmdConstants, 0);

            const U32 numThreadGroup = sceneProxy->GetNumInstancing();
            genDrawCmdsCmdList->Dispatch(numThreadGroup, 1, 1);

            genDrawCmdsCmdList->AddPendingBufferBarrier(
                *drawOpaqueStaticMeshCmdBuffer,
                D3D12_BARRIER_SYNC_COMPUTE_SHADING, D3D12_BARRIER_SYNC_NONE,
                D3D12_BARRIER_ACCESS_UNORDERED_ACCESS, D3D12_BARRIER_ACCESS_NO_ACCESS);
            genDrawCmdsCmdList->AddPendingBufferBarrier(
                *instancingStorageBuffer,
                D3D12_BARRIER_SYNC_COMPUTE_SHADING, D3D12_BARRIER_SYNC_NONE,
                D3D12_BARRIER_ACCESS_UNORDERED_ACCESS, D3D12_BARRIER_ACCESS_NO_ACCESS);
            genDrawCmdsCmdList->FlushBarriers();

            genDrawCmdsCmdList->Close();

            asyncComputeQueue.Wait(computeCullingSync);
            CommandList* cmdLists[]{genDrawCmdsCmdList};
            asyncComputeQueue.ExecuteCommandLists(cmdLists);
        }
        GpuSyncPoint genDrawCmdsSync = asyncComputeQueue.MakeSyncPointWithSignal(renderContext->GetAsyncComputeFence());

        // Culling이 완료되면 Command Buffer를 가지고 ExecuteIndirect!

        renderCmdList->Open(pso.get());
        {
            renderCmdList->SetDescriptorHeaps(bindlessDescHeaps);
            renderCmdList->SetRootSignature(*bindlessRootSignature);

            renderCmdList->AddPendingTextureBarrier(
                *backBuffer,
                D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_RENDER_TARGET,
                D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_RENDER_TARGET,
                D3D12_BARRIER_LAYOUT_PRESENT, D3D12_BARRIER_LAYOUT_RENDER_TARGET);
            renderCmdList->AddPendingBufferBarrier(
                *drawOpaqueStaticMeshCmdBuffer,
                D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_EXECUTE_INDIRECT,
                D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT);
            renderCmdList->FlushBarriers();

            renderCmdList->ClearRenderTarget(*backBufferRtv);
            GpuView* dsv = renderContext->Lookup(dsvs[localFrameIdx]);
            renderCmdList->ClearDepth(*dsv);
            renderCmdList->SetRenderTarget(*backBufferRtv, *dsv);
            renderCmdList->SetViewport(mainViewport);
            renderCmdList->SetScissorRect(mainViewport);
            renderCmdList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            renderCmdList->ExecuteIndirect(*commandSignature, *drawOpaqueStaticMeshCmdBuffer);

            renderCmdList->AddPendingTextureBarrier(
                *backBuffer,
                D3D12_BARRIER_SYNC_RENDER_TARGET, D3D12_BARRIER_SYNC_NONE,
                D3D12_BARRIER_ACCESS_RENDER_TARGET, D3D12_BARRIER_ACCESS_NO_ACCESS,
                D3D12_BARRIER_LAYOUT_RENDER_TARGET, D3D12_BARRIER_LAYOUT_PRESENT);
            renderCmdList->AddPendingBufferBarrier(
                *drawOpaqueStaticMeshCmdBuffer,
                D3D12_BARRIER_SYNC_EXECUTE_INDIRECT, D3D12_BARRIER_SYNC_NONE,
                D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT, D3D12_BARRIER_ACCESS_NO_ACCESS);
            renderCmdList->FlushBarriers();
        }
        renderCmdList->Close();
        CommandList* mainPassCmdLists[]{renderCmdList};
        mainGfxQueue.Wait(genDrawCmdsSync);
        mainGfxQueue.ExecuteCommandLists(mainPassCmdLists);

        ImDrawData* imGuiDrawData = ImGui::GetDrawData();
        if (imGuiDrawData != nullptr)
        {
            auto imguiCmdList = mainGfxCmdListPool.Request(localFrameIdx, "RenderImGui"_fs);
            imguiCmdList->Open();
            {
                imguiCmdList->AddPendingTextureBarrier(
                    *backBuffer,
                    D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_RENDER_TARGET,
                    D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_RENDER_TARGET,
                    D3D12_BARRIER_LAYOUT_PRESENT, D3D12_BARRIER_LAYOUT_RENDER_TARGET);
                imguiCmdList->FlushBarriers();

                imguiCmdList->SetRenderTarget(*backBufferRtv);
                imguiCmdList->SetDescriptorHeap(renderContext->GetCbvSrvUavDescriptorHeap());
                ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), &imguiCmdList->GetNative());

                imguiCmdList->AddPendingTextureBarrier(
                    *backBuffer,
                    D3D12_BARRIER_SYNC_RENDER_TARGET, D3D12_BARRIER_SYNC_NONE,
                    D3D12_BARRIER_ACCESS_RENDER_TARGET, D3D12_BARRIER_ACCESS_NO_ACCESS,
                    D3D12_BARRIER_LAYOUT_RENDER_TARGET, D3D12_BARRIER_LAYOUT_PRESENT);
                imguiCmdList->FlushBarriers();
            }
            imguiCmdList->Close();

            GpuSyncPoint mainPassSync = mainGfxQueue.MakeSyncPointWithSignal(renderContext->GetMainGfxFence());
            mainGfxQueue.Wait(mainPassSync);
            CommandList* imguiPassCmdLists[]{imguiCmdList};
            mainGfxQueue.ExecuteCommandLists(imguiPassCmdLists);
        }

        swapchain.Present();
        return mainGfxQueue.MakeSyncPointWithSignal(renderContext->GetMainGfxFence());
    }
} // namespace ig
