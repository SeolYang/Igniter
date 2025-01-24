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
    struct DrawInstance
    {
        U32 PerFrameCbv;
        U32 MaterialIdx;
        U32 VertexOffset;
        U32 VertexIdxOffset;
        U32 IndirectTransformOffset;

        D3D12_DRAW_ARGUMENTS DrawIndexedArguments;
    };

    struct StaticMeshLodInstance
    {
        U32 RenderableIdx = IG_NUMERIC_MAX_OF(RenderableIdx);
        U32 Lod = 0;
        U32 LodInstanceId = 0;
    };

    // 나중에 여러 카메라에 대해 렌더링하게 되면 Per Frame은 아닐수도?
    struct PerFrameConstants
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
        U32 EnableFrustumCulling = true;
        U32 MinMeshLod = 0;

        U32 StaticMeshVertexStorageSrv = IG_NUMERIC_MAX_OF(StaticMeshVertexStorageSrv);
        U32 VertexIndexStorageSrv = IG_NUMERIC_MAX_OF(VertexIndexStorageSrv);

        U32 TransformStorageSrv = IG_NUMERIC_MAX_OF(TransformStorageSrv);
        U32 MaterialStorageSrv = IG_NUMERIC_MAX_OF(MaterialStorageSrv);
        U32 MeshStorageSrv = IG_NUMERIC_MAX_OF(MeshStorageSrv);

        U32 InstancingDataStorageSrv = IG_NUMERIC_MAX_OF(InstancingDataStorageSrv);
        U32 InstancingDataStorageUav = IG_NUMERIC_MAX_OF(InstancingDataStorageUav);

        U32 IndirectTransformStorageSrv = IG_NUMERIC_MAX_OF(IndirectTransformStorageSrv);
        U32 IndirectTransformStorageUav = IG_NUMERIC_MAX_OF(IndirectTransformStorageUav);

        U32 RenderableStorageSrv = IG_NUMERIC_MAX_OF(RenderableStorageSrv);

        U32 RenderableIndicesBufferSrv = IG_NUMERIC_MAX_OF(RenderableIndicesBufferSrv);
        U32 NumMaxRenderables = IG_NUMERIC_MAX_OF(NumMaxRenderables);

        // TODO Light Sttorage/Indices/NumMaxLights
    };

    struct FrustumCullingConstants
    {
        U32 PerFrameCbv;
        U32 MeshLodInstanceStorageUav;
        U32 CullingDataBufferUav;
    };

    struct CompactMeshLodInstancesConstants
    {
        U32 PerFrameCbv;
        U32 MeshLodInstanceStorageUav;
        U32 CullingDataBufferSrv;
    };

    struct GenerateDrawInstanceConstants
    {
        U32 PerFrameCbv;
        U32 DrawInstanceBufferUav;
    };

    struct CullingData
    {
        U32 NumVisibleLodInstances;
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

        // Reverse-Z PSO & Viewport
        GraphicsPipelineStateDesc psoDesc;
        psoDesc.SetVertexShader(*vs);
        psoDesc.SetPixelShader(*ps);
        psoDesc.SetRootSignature(*bindlessRootSignature);
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC{CD3DX12_DEFAULT{}};
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
        pso = MakePtr<PipelineState>(gpuDevice.CreateGraphicsPipelineState(psoDesc).value());

        GpuTextureDesc depthStencilDesc;
        depthStencilDesc.DebugName = "DepthStencilBufferTex"_fs;
        depthStencilDesc.AsDepthStencil(static_cast<U32>(mainViewport.width), static_cast<U32>(mainViewport.height), DXGI_FORMAT_D32_FLOAT, true);

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

        const ShaderCompileDesc frustumCullingShaderDesc{.SourcePath = "Assets/Shaders/FrustumCulling.hlsl"_fs, .Type = EShaderType::Compute};
        frustumCullingShader = MakePtr<ShaderBlob>(frustumCullingShaderDesc);
        ComputePipelineStateDesc frustumCullingPsoDesc{};
        frustumCullingPsoDesc.Name = "FrustumCullingPSO"_fs;
        frustumCullingPsoDesc.SetComputeShader(*frustumCullingShader);
        frustumCullingPsoDesc.SetRootSignature(*bindlessRootSignature);
        frustumCullingPso = MakePtr<PipelineState>(gpuDevice.CreateComputePipelineState(frustumCullingPsoDesc).value());
        IG_CHECK(frustumCullingPso->IsCompute());

        GpuBufferDesc cullingDataBufferDesc{};
        cullingDataBufferDesc.AsStructuredBuffer<CullingData>(1, true);
        for (const LocalFrameIndex localFrameIdx : views::iota(0Ui8, NumFramesInFlight))
        {
            cullingDataBufferDesc.DebugName = std::format("CullingDataBuffer{}", localFrameIdx);
            cullingDataBuffer[localFrameIdx] = renderContext.CreateBuffer(cullingDataBufferDesc);
            cullingDataBufferSrv[localFrameIdx] = renderContext.CreateShaderResourceView(cullingDataBuffer[localFrameIdx]);
            cullingDataBufferUav[localFrameIdx] = renderContext.CreateUnorderedAccessView(cullingDataBuffer[localFrameIdx]);
        }

        const ShaderCompileDesc compactMeshLodInstancesDesc{.SourcePath = "Assets/Shaders/CompactMeshLodInstances.hlsl"_fs, .Type = EShaderType::Compute};
        compactMeshLodInstancesShader = MakePtr<ShaderBlob>(compactMeshLodInstancesDesc);
        ComputePipelineStateDesc compactMeshLodInstancesPsoDesc{};
        compactMeshLodInstancesPsoDesc.Name = "CompactMeshLodInstancesPSO"_fs;
        compactMeshLodInstancesPsoDesc.SetComputeShader(*compactMeshLodInstancesShader);
        compactMeshLodInstancesPsoDesc.SetRootSignature(*bindlessRootSignature);
        compactMeshLodInstancesPso = MakePtr<PipelineState>(gpuDevice.CreateComputePipelineState(compactMeshLodInstancesPsoDesc).value());
        IG_CHECK(compactMeshLodInstancesPso->IsCompute());

        const ShaderCompileDesc genDrawInstanceCmdShaderDesc{.SourcePath = "Assets/Shaders/GenerateDrawInstanceCommand.hlsl"_fs, .Type = EShaderType::Compute};
        generateDrawInstanceCmdShader = MakePtr<ShaderBlob>(genDrawInstanceCmdShaderDesc);
        ComputePipelineStateDesc generateDrawInstanceCmdPsoDesc{};
        generateDrawInstanceCmdPsoDesc.SetComputeShader(*generateDrawInstanceCmdShader);
        generateDrawInstanceCmdPsoDesc.SetRootSignature(*bindlessRootSignature);
        generateDrawInstanceCmdPsoDesc.Name = "GenerateDrawInstanceCmdPso"_fs;
        generateDrawInstanceCmdPso = MakePtr<PipelineState>(gpuDevice.CreateComputePipelineState(generateDrawInstanceCmdPsoDesc).value());
        IG_CHECK(generateDrawInstanceCmdPso->IsCompute());

        CommandSignatureDesc commandSignatureDesc{};
        commandSignatureDesc.AddConstant(0, 0, 5)
            .AddDrawArgument()
            .SetCommandByteStride<DrawInstance>();
        drawInstanceCmdSignature = MakePtr<CommandSignature>(
            gpuDevice.CreateCommandSignature(
                         "DrawInstanceCmdSignature",
                         commandSignatureDesc,
                         *bindlessRootSignature)
                .value());

        for (const LocalFrameIndex localFrameIdx : views::iota(0Ui8, NumFramesInFlight))
        {
            meshLodInstanceStorage[localFrameIdx] = MakePtr<GpuStorage>(
                renderContext,
                GpuStorageDesc{
                    .DebugName = String(std::format("StaticMeshLodInstanceStorage.{}", localFrameIdx)),
                    .ElementSize = (U32)sizeof(StaticMeshLodInstance),
                    .NumInitElements = kInitNumDrawCommands,
                    .Flags =
                        EGpuStorageFlags::ShaderReadWrite |
                        EGpuStorageFlags::EnableUavCounter |
                        EGpuStorageFlags::EnableLinearAllocation});

            drawInstanceCmdStorage[localFrameIdx] = MakePtr<GpuStorage>(
                renderContext,
                GpuStorageDesc{
                    .DebugName = String(std::format("DrawRenderableCmdBuffer.{}", localFrameIdx)),
                    .ElementSize = (U32)sizeof(DrawInstance),
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
            drawInstanceCmdStorage[localFrameIdx]->ForceReset();
            meshLodInstanceStorage[localFrameIdx]->ForceReset();

            renderContext->DestroyGpuView(cullingDataBufferSrv[localFrameIdx]);
            renderContext->DestroyGpuView(cullingDataBufferUav[localFrameIdx]);
            renderContext->DestroyBuffer(cullingDataBuffer[localFrameIdx]);

            renderContext->DestroyGpuView(dsvs[localFrameIdx]);
            renderContext->DestroyTexture(depthStencils[localFrameIdx]);
        }
    }

    void Renderer::PreRender(const LocalFrameIndex localFrameIdx)
    {
        tempConstantBufferAllocator->Reset(localFrameIdx);

        if (meshLodInstanceSpace[localFrameIdx].IsValid())
        {
            meshLodInstanceSpace[localFrameIdx] = {};
            meshLodInstanceStorage[localFrameIdx]->ForceReset();
        }

        if (drawInstanceCmdSpace[localFrameIdx].IsValid())
        {
            drawInstanceCmdSpace[localFrameIdx] = {};
            // 블럭을 합쳐서 fragmentation 최소화
            drawInstanceCmdStorage[localFrameIdx]->ForceReset();
        }

        const Size numMaxRenderables = sceneProxy->GetMaxNumRenderables(localFrameIdx);
        const Size numInstancing = sceneProxy->GetNumInstancing();
        if (numInstancing > 0 && numMaxRenderables > 0)
        {
            meshLodInstanceSpace[localFrameIdx] =
                meshLodInstanceStorage[localFrameIdx]->Allocate(numMaxRenderables);

            drawInstanceCmdSpace[localFrameIdx] =
                drawInstanceCmdStorage[localFrameIdx]->Allocate(numInstancing * StaticMesh::kMaxNumLods);
        }
    }

    GpuSyncPoint Renderer::Render(const LocalFrameIndex localFrameIdx, const World& world, GpuSyncPoint sceneProxyRepSyncPoint)
    {
        ZoneScoped;

        IG_CHECK(renderContext != nullptr);
        IG_CHECK(meshStorage != nullptr);
        IG_CHECK(sceneProxy != nullptr);

        // 프레임 마다 업데이트 되는 버퍼 데이터 업데이트
        PerFrameConstants perFrameConstants{};

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
            const Matrix projMatrix = camera.CreatePerspectiveForReverseZ();
            perFrameConstants.View = ConvertToShaderSuitableForm(viewMatrix);
            perFrameConstants.ViewProj = ConvertToShaderSuitableForm(viewMatrix * projMatrix);
            perFrameConstants.CamPosInvAspectRatio = Vector4{
                transformComponent.Position.x,
                transformComponent.Position.y,
                transformComponent.Position.z,
                1.f / camera.CameraViewport.AspectRatio()};

            const F32 halfFovYRad = Deg2Rad(camera.Fov * 0.5f);
            const F32 cosHalfFovY{std::cosf(halfFovYRad)};
            const F32 sinHalfFovY{std::sinf(halfFovYRad)};
            perFrameConstants.ViewFrustumParams = Vector4{
                cosHalfFovY, sinHalfFovY,
                camera.NearZ, camera.FarZ};

            perFrameConstants.EnableFrustumCulling = camera.bEnableFrustumCull;
            bMainCameraExists = true;
            break;
        }

        Swapchain& swapchain = renderContext->GetSwapchain();
        GpuTexture* backBuffer = renderContext->Lookup(swapchain.GetBackBuffer());
        const GpuView* backBufferRtv = renderContext->Lookup(swapchain.GetRenderTargetView());

        CommandQueue& mainGfxQueue{renderContext->GetMainGfxQueue()};
        CommandListPool& mainGfxCmdListPool = renderContext->GetMainGfxCommandListPool();
        auto renderCmdList = mainGfxCmdListPool.Request(localFrameIdx, "MainGfxRender"_fs);
        if (sceneProxy->GetMaxNumRenderables(localFrameIdx) == 0 || !bMainCameraExists)
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
            renderCmdList->ClearDepth(*dsv, 0.f);
            renderCmdList->AddPendingTextureBarrier(
                *backBuffer,
                D3D12_BARRIER_SYNC_RENDER_TARGET, D3D12_BARRIER_SYNC_NONE,
                D3D12_BARRIER_ACCESS_RENDER_TARGET, D3D12_BARRIER_ACCESS_NO_ACCESS,
                D3D12_BARRIER_LAYOUT_RENDER_TARGET, D3D12_BARRIER_LAYOUT_PRESENT);
            renderCmdList->FlushBarriers();

            renderCmdList->Close();
            CommandList* renderCmdLists[]{renderCmdList};
            mainGfxQueue.ExecuteCommandLists(renderCmdLists);
        }
        else
        {
            const GpuView* staticMeshVertexStorageSrv = renderContext->Lookup(meshStorage->GetStaticMeshVertexStorageSrv());
            IG_CHECK(staticMeshVertexStorageSrv != nullptr && staticMeshVertexStorageSrv->IsValid());
            perFrameConstants.StaticMeshVertexStorageSrv = staticMeshVertexStorageSrv->Index;

            const GpuView* vertexIndexStorageSrv = renderContext->Lookup(meshStorage->GetIndexStorageSrv());
            IG_CHECK(vertexIndexStorageSrv != nullptr && vertexIndexStorageSrv->IsValid());
            perFrameConstants.VertexIndexStorageSrv = vertexIndexStorageSrv->Index;

            const GpuView* transformStorageSrv = renderContext->Lookup(sceneProxy->GetTransformProxyStorageSrv(localFrameIdx));
            IG_CHECK(transformStorageSrv != nullptr && transformStorageSrv->IsValid());
            perFrameConstants.TransformStorageSrv = transformStorageSrv->Index;

            const GpuView* materialStorageSrv = renderContext->Lookup(sceneProxy->GetMaterialProxyStorageSrv(localFrameIdx));
            IG_CHECK(materialStorageSrv != nullptr && materialStorageSrv->IsValid());
            perFrameConstants.MaterialStorageSrv = materialStorageSrv->Index;

            const GpuView* meshProxyStorageSrv = renderContext->Lookup(sceneProxy->GetMeshProxySrv(localFrameIdx));
            IG_CHECK(meshProxyStorageSrv != nullptr && meshProxyStorageSrv->IsValid());
            perFrameConstants.MeshStorageSrv = meshProxyStorageSrv->Index;

            // GpuBuffer* instancingStorageBuffer = renderContext->Lookup(sceneProxy->GetInstancingStorageBuffer(localFrameIdx));
            const GpuView* instancingStorageSrv = renderContext->Lookup(sceneProxy->GetInstancingStorageSrv(localFrameIdx));
            const GpuView* instancingStorageUav = renderContext->Lookup(sceneProxy->GetInstancingStorageUav(localFrameIdx));
            // IG_CHECK(instancingStorageBuffer != nullptr && instancingStorageBuffer->IsValid());
            IG_CHECK(instancingStorageSrv != nullptr && instancingStorageSrv->IsValid());
            IG_CHECK(instancingStorageUav != nullptr && instancingStorageUav->IsValid());
            perFrameConstants.InstancingDataStorageSrv = instancingStorageSrv->Index;
            perFrameConstants.InstancingDataStorageUav = instancingStorageUav->Index;

            // GpuBuffer* indirectTransformStorageBuffer = renderContext->Lookup(sceneProxy->GetIndirectTransformStorageBuffer(localFrameIdx));
            const GpuView* indirectTransformStorageSrv = renderContext->Lookup(sceneProxy->GetIndirectTransformStorageSrv(localFrameIdx));
            const GpuView* indirectTransformStorageUav = renderContext->Lookup(sceneProxy->GetIndirectTransformStorageUav(localFrameIdx));
            // IG_CHECK(indirectTransformStorageBuffer != nullptr && indirectTransformStorageBuffer->IsValid());
            IG_CHECK(indirectTransformStorageSrv != nullptr && indirectTransformStorageSrv->IsValid());
            IG_CHECK(indirectTransformStorageUav != nullptr && indirectTransformStorageUav->IsValid());
            perFrameConstants.IndirectTransformStorageSrv = indirectTransformStorageSrv->Index;
            perFrameConstants.IndirectTransformStorageUav = indirectTransformStorageUav->Index;

            const GpuView* renderableStorageSrv = renderContext->Lookup(sceneProxy->GetRenderableProxyStorageSrv(localFrameIdx));
            IG_CHECK(renderableStorageSrv != nullptr && renderableStorageSrv->IsValid());
            perFrameConstants.RenderableStorageSrv = renderableStorageSrv->Index;

            const GpuView* renderableIndicesBufferSrv = renderContext->Lookup(sceneProxy->GetRenderableIndicesSrv(localFrameIdx));
            IG_CHECK(renderableIndicesBufferSrv != nullptr && renderableIndicesBufferSrv->IsValid());
            perFrameConstants.RenderableIndicesBufferSrv = renderableIndicesBufferSrv->Index;
            perFrameConstants.NumMaxRenderables = sceneProxy->GetMaxNumRenderables(localFrameIdx);

            perFrameConstants.MinMeshLod = minMeshLod;

            TempConstantBuffer perFrameConstantBuffer = tempConstantBufferAllocator->Allocate<PerFrameConstants>(localFrameIdx);
            const GpuView* perFrameBufferCbv = renderContext->Lookup(perFrameConstantBuffer.GetConstantBufferView());
            IG_CHECK(perFrameBufferCbv != nullptr && perFrameBufferCbv->IsValid());
            perFrameConstantBuffer.Write(perFrameConstants);

            GpuBuffer* meshLodInstanceBuffer = renderContext->Lookup(meshLodInstanceStorage[localFrameIdx]->GetGpuBuffer());
            const GpuBufferDesc& meshLodInstanceBufferDesc = meshLodInstanceBuffer->GetDesc();
            const GpuView* meshLodInstanceBufferUav = renderContext->Lookup(meshLodInstanceStorage[localFrameIdx]->GetUnorderedResourceView());
            IG_CHECK(meshLodInstanceBuffer != nullptr && meshLodInstanceBuffer->IsValid());
            IG_CHECK(meshLodInstanceBufferDesc.IsUavCounterEnabled());
            IG_CHECK(meshLodInstanceBufferUav != nullptr && meshLodInstanceBufferUav->IsValid());

            GpuBuffer* drawInstanceCmdBuffer = renderContext->Lookup(drawInstanceCmdStorage[localFrameIdx]->GetGpuBuffer());
            const GpuBufferDesc& drawInstanceCmdBufferDesc = drawInstanceCmdBuffer->GetDesc();
            const GpuView* drawInstanceCmdBufferUav = renderContext->Lookup(drawInstanceCmdStorage[localFrameIdx]->GetUnorderedResourceView());
            IG_CHECK(drawInstanceCmdBuffer != nullptr && drawInstanceCmdBuffer->IsValid());
            IG_CHECK(drawInstanceCmdBufferDesc.IsUavCounterEnabled());
            IG_CHECK(drawInstanceCmdBufferUav != nullptr && drawInstanceCmdBufferUav->IsValid());

            GpuBuffer* cullingDataBufferPtr = renderContext->Lookup(cullingDataBuffer[localFrameIdx]);
            const GpuView* cullingDataBufferSrvPtr = renderContext->Lookup(cullingDataBufferSrv[localFrameIdx]);
            const GpuView* cullingDataBufferUavPtr = renderContext->Lookup(cullingDataBufferUav[localFrameIdx]);
            IG_CHECK(cullingDataBufferPtr != nullptr && cullingDataBufferPtr->IsValid());
            IG_CHECK(cullingDataBufferSrvPtr != nullptr && cullingDataBufferSrvPtr->IsValid());
            IG_CHECK(cullingDataBufferUavPtr != nullptr && cullingDataBufferUavPtr->IsValid());

            auto bindlessDescHeaps = renderContext->GetBindlessDescriptorHeaps();
            CommandQueue& asyncComputeQueue{renderContext->GetAsyncComputeQueue()};
            CommandListPool& asyncComputeCmdListPool{renderContext->GetAsyncComputeCommandListPool()};
            {
                const FrustumCullingConstants frustumCullingConstants{
                    .PerFrameCbv = perFrameBufferCbv->Index,
                    .MeshLodInstanceStorageUav = meshLodInstanceBufferUav->Index,
                    .CullingDataBufferUav = cullingDataBufferUavPtr->Index};

                auto cmdList = asyncComputeCmdListPool.Request(localFrameIdx, "FrustumCullingCmdList"_fs);
                cmdList->Open(frustumCullingPso.get());
                cmdList->SetDescriptorHeaps(bindlessDescHeaps);
                cmdList->SetRootSignature(*bindlessRootSignature);

                cmdList->AddPendingBufferBarrier(
                    *uavCounterResetBuffer,
                    D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COPY,
                    D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_COPY_SOURCE);
                cmdList->AddPendingBufferBarrier(
                    *meshLodInstanceBuffer,
                    D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COPY,
                    D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_COPY_DEST);
                cmdList->AddPendingBufferBarrier(
                    *cullingDataBufferPtr,
                    D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COPY,
                    D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_COPY_DEST);
                cmdList->FlushBarriers();

                /*
                 * 나중에 아예 Zero Init 용 통합 버퍼, 예를들어 현재 상태에선
                 * Union(kUavCounterSize, CullingData) 크기에 해당하는 버퍼 1개만 할당!
                 */
                cmdList->CopyBuffer(
                    *uavCounterResetBuffer, 0, GpuBufferDesc::kUavCounterSize,
                    *meshLodInstanceBuffer, meshLodInstanceBufferDesc.GetUavCounterOffset());
                cmdList->CopyBuffer(
                    *uavCounterResetBuffer, 0, sizeof(CullingData),
                    *cullingDataBufferPtr, 0);

                cmdList->AddPendingBufferBarrier(
                    *meshLodInstanceBuffer,
                    D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_SYNC_COMPUTE_SHADING,
                    D3D12_BARRIER_ACCESS_COPY_DEST, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS);
                cmdList->AddPendingBufferBarrier(
                    *cullingDataBufferPtr,
                    D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_SYNC_COMPUTE_SHADING,
                    D3D12_BARRIER_ACCESS_COPY_DEST, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS);
                cmdList->FlushBarriers();

                cmdList->SetRoot32BitConstants(0, frustumCullingConstants, 0);

                constexpr U32 kNumThreadsPerGroup = 32;
                const U32 numRenderables = (U32)sceneProxy->GetMaxNumRenderables(localFrameIdx);
                const U32 numRemainThreads = numRenderables % kNumThreadsPerGroup;
                const U32 numThreadGroup = numRenderables / kNumThreadsPerGroup + (numRemainThreads > 0 ? 1 : 0);
                cmdList->Dispatch(numThreadGroup, 1, 1);

                cmdList->Close();

                CommandList* cmdLists[]{cmdList};
                asyncComputeQueue.Wait(sceneProxyRepSyncPoint);
                asyncComputeQueue.ExecuteCommandLists(cmdLists);
            }
            GpuSyncPoint frustumCullingSyncPoint = asyncComputeQueue.MakeSyncPointWithSignal(renderContext->GetAsyncComputeFence());

            {
                const CompactMeshLodInstancesConstants comapctMeshLodInstancesConstants{
                    .PerFrameCbv = perFrameBufferCbv->Index,
                    .MeshLodInstanceStorageUav = meshLodInstanceBufferUav->Index,
                    .CullingDataBufferSrv = cullingDataBufferSrvPtr->Index};

                auto cmdList = asyncComputeCmdListPool.Request(localFrameIdx, "CompactMeshLodInstacesCmdList"_fs);
                cmdList->Open(compactMeshLodInstancesPso.get());
                cmdList->SetDescriptorHeaps(bindlessDescHeaps);
                cmdList->SetRootSignature(*bindlessRootSignature);

                cmdList->SetRoot32BitConstants(0, comapctMeshLodInstancesConstants, 0);

                constexpr U32 kNumThreadsPerGroup = 32;
                const U32 numRenderables = (U32)sceneProxy->GetMaxNumRenderables(localFrameIdx);
                const U32 numRemainThreads = numRenderables % kNumThreadsPerGroup;
                const U32 numThreadGroup = numRenderables / kNumThreadsPerGroup + (numRemainThreads > 0 ? 1 : 0);
                cmdList->Dispatch(numThreadGroup, 1, 1);

                cmdList->Close();

                CommandList* cmdLists[]{cmdList};
                asyncComputeQueue.Wait(frustumCullingSyncPoint);
                asyncComputeQueue.ExecuteCommandLists(cmdLists);
            }
            GpuSyncPoint compactMeshLodInstancesSyncPoint = asyncComputeQueue.MakeSyncPointWithSignal(renderContext->GetAsyncComputeFence());

            {
                const GenerateDrawInstanceConstants generateInstanceDrawConstants{
                    .PerFrameCbv = perFrameBufferCbv->Index,
                    .DrawInstanceBufferUav = drawInstanceCmdBufferUav->Index};

                auto cmdList = asyncComputeCmdListPool.Request(localFrameIdx, "GenerateDrawInstanceCmdList"_fs);
                cmdList->Open(generateDrawInstanceCmdPso.get());
                cmdList->SetDescriptorHeaps(bindlessDescHeaps);
                cmdList->SetRootSignature(*bindlessRootSignature);

                cmdList->AddPendingBufferBarrier(
                    *uavCounterResetBuffer,
                    D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COPY,
                    D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_COPY_SOURCE);
                cmdList->AddPendingBufferBarrier(
                    *drawInstanceCmdBuffer,
                    D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COPY,
                    D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_COPY_DEST);
                cmdList->FlushBarriers();

                cmdList->CopyBuffer(
                    *uavCounterResetBuffer, 0, GpuBufferDesc::kUavCounterSize,
                    *drawInstanceCmdBuffer, drawInstanceCmdBufferDesc.GetUavCounterOffset());

                cmdList->AddPendingBufferBarrier(
                    *drawInstanceCmdBuffer,
                    D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_SYNC_COMPUTE_SHADING,
                    D3D12_BARRIER_ACCESS_COPY_DEST, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS);
                cmdList->FlushBarriers();

                cmdList->SetRoot32BitConstants(0, generateInstanceDrawConstants, 0);
                // constexpr U32 kNumThreadsPerGroup = 1;
                const U32 numLodInstancing = sceneProxy->GetNumInstancing() * StaticMesh::kMaxNumLods;
                cmdList->Dispatch(numLodInstancing, 1, 1);

                cmdList->Close();

                CommandList* cmdLists[]{cmdList};
                asyncComputeQueue.Wait(compactMeshLodInstancesSyncPoint);
                asyncComputeQueue.ExecuteCommandLists(cmdLists);
            }
            GpuSyncPoint generateDrawInstanceSyncPoint = asyncComputeQueue.MakeSyncPointWithSignal(renderContext->GetAsyncComputeFence());

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
                    *drawInstanceCmdBuffer,
                    D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_EXECUTE_INDIRECT,
                    D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT);
                renderCmdList->FlushBarriers();

                renderCmdList->ClearRenderTarget(*backBufferRtv);
                GpuView* dsv = renderContext->Lookup(dsvs[localFrameIdx]);
                renderCmdList->ClearDepth(*dsv, 0.f);
                renderCmdList->SetRenderTarget(*backBufferRtv, *dsv);
                renderCmdList->SetViewport(mainViewport);
                renderCmdList->SetScissorRect(mainViewport);
                renderCmdList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                renderCmdList->ExecuteIndirect(*drawInstanceCmdSignature, *drawInstanceCmdBuffer);

                renderCmdList->AddPendingTextureBarrier(
                    *backBuffer,
                    D3D12_BARRIER_SYNC_RENDER_TARGET, D3D12_BARRIER_SYNC_NONE,
                    D3D12_BARRIER_ACCESS_RENDER_TARGET, D3D12_BARRIER_ACCESS_NO_ACCESS,
                    D3D12_BARRIER_LAYOUT_RENDER_TARGET, D3D12_BARRIER_LAYOUT_PRESENT);
                renderCmdList->AddPendingBufferBarrier(
                    *drawInstanceCmdBuffer,
                    D3D12_BARRIER_SYNC_EXECUTE_INDIRECT, D3D12_BARRIER_SYNC_NONE,
                    D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT, D3D12_BARRIER_ACCESS_NO_ACCESS);
                renderCmdList->FlushBarriers();
            }
            renderCmdList->Close();
            CommandList* mainPassCmdLists[]{renderCmdList};
            mainGfxQueue.Wait(generateDrawInstanceSyncPoint);
            mainGfxQueue.ExecuteCommandLists(mainPassCmdLists);
        }

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
