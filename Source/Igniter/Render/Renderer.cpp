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
#include "Igniter/Render/UnifiedMeshStorage.h"
#include "Igniter/Render/SceneProxy.h"
#include "Igniter/Render/RenderPass/LightClusteringPass.h"
#include "Igniter/Render/RenderPass/FrustumCullingPass.h"
#include "Igniter/Render/RenderPass/ZPrePass.h"
#include "Igniter/Render/RenderPass/ForwardOpaqueMeshRenderPass.h"
#include "Igniter/Render/RenderPass/ImGuiRenderPass.h"
#include "Igniter/Gameplay/World.h"
#include "Igniter/Component/TransformComponent.h"
#include "Igniter/Component/CameraComponent.h"
#include "Igniter/Render/Renderer.h"

IG_DECLARE_LOG_CATEGORY(RendererLog);

IG_DEFINE_LOG_CATEGORY(RendererLog);

namespace ig
{
    // 나중에 여러 카메라에 대해 렌더링하게 되면 Per Frame은 아닐수도?
    //struct PerFrameConstants
    //{
    //    Matrix View{};
    //    Matrix ViewProj{};

    //    // [1]: Real-Time Rendering 4th ed. 22.14.1 Frustum Plane Extraction
    //    // [2]: https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
    //    // [3]: https://iquilezles.org/articles/frustum/
    //    // (CameraPos.xyz, InverseAspectRatio)
    //    Vector4 CamPosInvAspectRatio{};
    //    // (cos(fovy*0.5f), sin(fovy*0.5f), Near, Far)
    //    // View Frustum (in view space) =>
    //    // r = 1/aspect_ratio
    //    // Left = (r*cos, 0, sin, 0)
    //    // Right = (-r*cos, 0, sin, 0)
    //    // Bottom = (0, cos, sin, 0)
    //    // Top = (0, -cos, sin, 0)
    //    // Near = (0, 0, 1, -N)
    //    // Far = (0, 0, -1, F)
    //    Vector4 ViewFrustumParams{};
    //    U32 EnableFrustumCulling = true;
    //    U32 MinMeshLod = 0;

    //    U32 StaticMeshVertexStorageSrv = IG_NUMERIC_MAX_OF(StaticMeshVertexStorageSrv);
    //    U32 VertexIndexStorageSrv = IG_NUMERIC_MAX_OF(VertexIndexStorageSrv);

    //    U32 TransformStorageSrv = IG_NUMERIC_MAX_OF(TransformStorageSrv);
    //    U32 MaterialStorageSrv = IG_NUMERIC_MAX_OF(MaterialStorageSrv);
    //    U32 MeshStorageSrv = IG_NUMERIC_MAX_OF(MeshStorageSrv);

    //    U32 InstancingDataStorageSrv = IG_NUMERIC_MAX_OF(InstancingDataStorageSrv);
    //    U32 InstancingDataStorageUav = IG_NUMERIC_MAX_OF(InstancingDataStorageUav);

    //    U32 IndirectTransformStorageSrv = IG_NUMERIC_MAX_OF(IndirectTransformStorageSrv);
    //    U32 IndirectTransformStorageUav = IG_NUMERIC_MAX_OF(IndirectTransformStorageUav);

    //    U32 RenderableStorageSrv = IG_NUMERIC_MAX_OF(RenderableStorageSrv);

    //    U32 RenderableIndicesBufferSrv = IG_NUMERIC_MAX_OF(RenderableIndicesBufferSrv);
    //    U32 NumMaxRenderables = IG_NUMERIC_MAX_OF(NumMaxRenderables);

    //    U32 LightStorageSrv = IG_NUMERIC_MAX_OF(LightStorageSrv);
    //    U32 LightIdxListSrv = IG_NUMERIC_MAX_OF(LightIdxListSrv);
    //    U32 LightTileBitfieldBufferSrv = IG_NUMERIC_MAX_OF(LightTileBitfieldBufferSrv);
    //    U32 LightDepthBinBufferSrv = IG_NUMERIC_MAX_OF(LightDepthBinBufferSrv);

    //    float ViewportWidth;
    //    float ViewportHeight;
    //};

    struct PerFrameParams
    {
        Matrix View;
        Matrix Proj;
        Matrix ViewProj;

        U32 VertexStorageSrv;
        U32 IndexStorageSrv;
        U32 TriangleStorageSrv;
        U32 MeshletStorageSrv;

        U32 LightStorageSrv;
        U32 MaterialStorageSrv;
        U32 StaticMeshStorageSrv;
        U32 MeshInstanceStorageSrv;

        U32 LightClusterParamsCbv;
        U32 Padding[3];

        /* (x,y,z): cam pos, w: inv aspect ratio */
        Vector4 CamWorldPosInvAspectRatio;
        /* x: cos(fovy/2), y: sin(fovy/2), z: near, w: far */
        Vector4 ViewFrustumParams;

        float ViewportWidth;
        float ViewportHeight;
    };

    struct DispatchMeshInstanceParams
    {
        U32 MeshInstanceIdx;
        U32 TargetLevelOfDetail;
        U32 PerFrameParamsCbv;
    };

    struct DispatchMeshInstance
    {
        DispatchMeshInstanceParams Params;
        U32 ThreadGroupCountX;
        U32 ThreadGroupCountY;
        U32 ThreadGroupCountZ;
    };

    struct MeshInstancePassParams
    {
        U32 PerFrameParamsCbv;
        U32 MeshInstanceIndicesBufferSrv;
        U32 NumMeshInstances;

        U32 OpaqueMeshInstanceDispatchBufferUav;
        U32 TransparentMeshInstanceDispatchBufferUav;
    };

    Renderer::Renderer(const Window& window, RenderContext& renderContext, const SceneProxy& sceneProxy)
        : window(&window)
        , renderContext(&renderContext)
        , sceneProxy(&sceneProxy)
        , mainViewport(window.GetViewport())
        , tempConstantBufferAllocator(MakePtr<TempConstantBufferAllocator>(renderContext))
    {
        GpuDevice& gpuDevice = renderContext.GetGpuDevice();
        bindlessRootSignature = MakePtr<RootSignature>(gpuDevice.CreateBindlessRootSignature().value());

        GpuTextureDesc depthStencilDesc;
        depthStencilDesc.DebugName = "DepthStencilBufferTex"_fs;
        depthStencilDesc.AsDepthStencil(static_cast<U32>(mainViewport.width), static_cast<U32>(mainViewport.height), DXGI_FORMAT_D32_FLOAT, true);
        depthStencilDesc.InitialLayout = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE;

        depthStencil = renderContext.CreateTexture(depthStencilDesc);
        dsv = renderContext.CreateDepthStencilView(depthStencil, D3D12_TEX2D_DSV{.MipSlice = 0});

        GpuBufferDesc zeroFilledBufferDesc{};
        zeroFilledBufferDesc.AsUploadBuffer(kZeroFilledBufferSize);
        zeroFilledBufferDesc.DebugName = "ZeroFilledBuffer"_fs;
        zeroFilledBuffer = MakePtr<GpuBuffer>(gpuDevice.CreateBuffer(zeroFilledBufferDesc).value());
        U8* const mappedZeroFilledBuffer = zeroFilledBuffer->Map();
        ZeroMemory(mappedZeroFilledBuffer, kZeroFilledBufferSize);
        zeroFilledBuffer->Unmap();

        imguiRenderPass = MakePtr<ImGuiRenderPass>(renderContext);

        const ShaderCompileDesc meshInstancePassShaderDesc
        {
            .SourcePath = "Assets/Shaders/MeshInstancePass.hlsl"_fs,
            .Type = EShaderType::Compute,
        };
        meshInstancePassShader = MakePtr<ShaderBlob>(meshInstancePassShaderDesc);
        ComputePipelineStateDesc meshInstancePassPsoDesc{};
        meshInstancePassPsoDesc.Name = "MeshInstancePassPSO"_fs;
        meshInstancePassPsoDesc.SetComputeShader(*meshInstancePassShader);
        meshInstancePassPsoDesc.SetRootSignature(*bindlessRootSignature);
        meshInstancePassPso = MakePtr<PipelineState>(gpuDevice.CreateComputePipelineState(meshInstancePassPsoDesc).value());

        GpuStorageDesc meshInstanceDispatchStorageDesc{};
        meshInstanceDispatchStorageDesc.ElementSize = sizeof(DispatchMeshInstance);
        meshInstanceDispatchStorageDesc.NumInitElements = kInitDispatchBufferCapacity;
        meshInstanceDispatchStorageDesc.Flags = EGpuStorageFlags::ShaderReadWrite |
            EGpuStorageFlags::EnableUavCounter |
            EGpuStorageFlags::EnableLinearAllocation;

        meshInstanceDispatchStorageDesc.DebugName = "OpaqueMeshInstanceDispatchStorage"_fs;
        opaqueMeshInstanceDispatchStorage = MakePtr<GpuStorage>(renderContext, meshInstanceDispatchStorageDesc);

        meshInstanceDispatchStorageDesc.DebugName = "TransparentMeshInstanceDispatchStorage"_fs;
        transparentMeshInstanceDispatchStorage = MakePtr<GpuStorage>(renderContext, meshInstanceDispatchStorageDesc);

        CommandSignatureDesc dispatchMeshInstanceCmdSignatureDesc{};
        dispatchMeshInstanceCmdSignatureDesc.SetCommandByteStride<DispatchMeshInstance>();
        dispatchMeshInstanceCmdSignatureDesc.AddConstant(0, 0, 3);
        dispatchMeshInstanceCmdSignatureDesc.AddDispatchMeshArgument();
        dispatchMeshInstanceCmdSignature = MakePtr<CommandSignature>(
            gpuDevice.CreateCommandSignature(
                "DispatchMeshInstanceCmdSignature",
                dispatchMeshInstanceCmdSignatureDesc,
                Ref{*bindlessRootSignature}).value());

        lightClusteringPass = MakePtr<LightClusteringPass>(renderContext, sceneProxy, *bindlessRootSignature, mainViewport);
        zPrePass = MakePtr<ZPrePass>(renderContext, *bindlessRootSignature);
        forwardOpaqueMeshRenderPass = MakePtr<ForwardOpaqueMeshRenderPass>(renderContext, *bindlessRootSignature, *dispatchMeshInstanceCmdSignature);
    }

    Renderer::~Renderer()
    {
        if (dsv)
        {
            renderContext->DestroyGpuView(dsv);
        }
        if (depthStencil)
        {
            renderContext->DestroyTexture(depthStencil);
        }
    }

    void Renderer::PreRender(const LocalFrameIndex localFrameIdx)
    {
        tempConstantBufferAllocator->Reset(localFrameIdx);

        const U32 numMeshInstances = sceneProxy->GetNumMeshInstances();
        if (numMeshInstances == 0)
        {
            return;
        }

        [[maybe_unused]] const auto opaqueMeshInstanceDispatchAlloc =
            opaqueMeshInstanceDispatchStorage->Allocate(numMeshInstances);
        [[maybe_unused]] const auto transparentMeshInstanceDispatchAlloc =
            transparentMeshInstanceDispatchStorage->Allocate(numMeshInstances);
    }

    GpuSyncPoint Renderer::Render(const LocalFrameIndex localFrameIdx, [[maybe_unused]] const World& world, [[maybe_unused]] GpuSyncPoint sceneProxyRepSyncPoint)
    {
        IG_CHECK(renderContext != nullptr);
        IG_CHECK(sceneProxy != nullptr);
        ZoneScoped;

        const Registry& registry = world.GetRegistry();

        PerFrameParams perFrameParams{};
        TempConstantBuffer perFrameParamsCb = tempConstantBufferAllocator->Allocate<PerFrameParams>(localFrameIdx);
        const UnifiedMeshStorage& unifiedMeshStorage = renderContext->GetUnifiedMeshStorage();
        perFrameParams.VertexStorageSrv = renderContext->Lookup(unifiedMeshStorage.GetVertexStorageSrv())->Index;
        perFrameParams.IndexStorageSrv = renderContext->Lookup(unifiedMeshStorage.GetIndexStorageSrv())->Index;
        perFrameParams.TriangleStorageSrv = renderContext->Lookup(unifiedMeshStorage.GetTriangleStorageSrv())->Index;
        perFrameParams.MeshletStorageSrv = renderContext->Lookup(unifiedMeshStorage.GetMeshletStorageSrv())->Index;

        perFrameParams.LightStorageSrv = renderContext->Lookup(sceneProxy->GetLightStorageSrv())->Index;
        perFrameParams.MaterialStorageSrv = renderContext->Lookup(sceneProxy->GetMaterialProxyStorageSrv())->Index;
        perFrameParams.StaticMeshStorageSrv = renderContext->Lookup(sceneProxy->GetStaticMeshProxyStorageSrv())->Index;
        perFrameParams.MeshInstanceStorageSrv = renderContext->Lookup(sceneProxy->GetMeshInstanceProxyStorageSrv())->Index;

        const auto camView = registry.view<const TransformComponent, const CameraComponent>();
        Matrix cpuCamViewMat{};
        Matrix gpuCamViewMat{};
        for (const auto& [entity, transform, camera] : camView.each())
        {
            cpuCamViewMat = transform.CreateView();
            const Matrix proj = camera.CreatePerspectiveForReverseZ();
            gpuCamViewMat = ConvertToShaderSuitableForm(cpuCamViewMat);
            perFrameParams.View = gpuCamViewMat;
            perFrameParams.Proj = ConvertToShaderSuitableForm(proj);
            perFrameParams.ViewProj = ConvertToShaderSuitableForm(cpuCamViewMat * proj);
            perFrameParams.CamWorldPosInvAspectRatio = Vector4{
                transform.Position.x, transform.Position.y, transform.Position.z,
                1.f / mainViewport.AspectRatio()
            };

            const float cosHalfFovY = std::cos(Deg2Rad(camera.Fov * 0.5f));
            const float sinHalfFovY = std::sin(Deg2Rad(camera.Fov * 0.5f));
            perFrameParams.ViewFrustumParams = Vector4{cosHalfFovY, sinHalfFovY, camera.NearZ, camera.FarZ};
        }
        perFrameParams.ViewportWidth = mainViewport.width;
        perFrameParams.ViewportHeight = mainViewport.height;

        TempConstantBuffer lightClusterParamsCb = tempConstantBufferAllocator->Allocate<LightClusterParams>(localFrameIdx);
        perFrameParams.LightClusterParamsCbv = renderContext->Lookup(lightClusterParamsCb.GetConstantBufferView())->Index;

        perFrameParamsCb.Write(perFrameParams);

        CommandQueue& mainGfxQueue = renderContext->GetMainGfxQueue();
        CommandListPool& mainGfxCmdListPool = renderContext->GetMainGfxCommandListPool();
        CommandQueue& asyncComputeQueue = renderContext->GetAsyncComputeQueue();
        CommandListPool& asyncComputeCmdListPool = renderContext->GetAsyncComputeCommandListPool();
        CommandQueue& frameCritCopyQueue = renderContext->GetFrameCriticalAsyncCopyQueue();
        CommandListPool& asyncCopyCmdListPool = renderContext->GetAsyncCopyCommandListPool();

        Swapchain& swapchain = renderContext->GetSwapchain();
        GpuTexture* backBuffer = renderContext->Lookup(swapchain.GetBackBuffer());
        const GpuView* backBufferRtv = renderContext->Lookup(swapchain.GetBackBufferRtv());

        const GpuView* perFrameParamsCbvPtr = renderContext->Lookup(perFrameParamsCb.GetConstantBufferView());
        IG_CHECK(perFrameParamsCbvPtr != nullptr);

        /* Light Clustering Pass */
        {
            auto copyLightIdxListCmdList = asyncCopyCmdListPool.Request(localFrameIdx, "LightClustering.CopyLightIdxList"_fs);
            auto clearTileBitfieldsCmdList = asyncComputeCmdListPool.Request(localFrameIdx, "LightClustering.ClearTileBitfields"_fs);
            auto lightClusteringCmdList = asyncComputeCmdListPool.Request(localFrameIdx, "LightClustering.Main"_fs);
            lightClusteringPass->SetParams(LightClusteringPassParams{
                .CopyLightIdxListCmdList = copyLightIdxListCmdList,
                .ClearTileBitfieldsCmdList = clearTileBitfieldsCmdList,
                .LightClusteringCmdList = lightClusteringCmdList,
                .View = cpuCamViewMat,
                .TargetViewport = mainViewport,
                .PerFrameParamsCbvPtr = perFrameParamsCbvPtr
            });

            lightClusteringPass->Record(localFrameIdx);
            lightClusterParamsCb.Write(lightClusteringPass->GetLightClusterParams());

            frameCritCopyQueue.ExecuteCommandList(*copyLightIdxListCmdList);
            GpuSyncPoint copyLightIdxSyncPoint = frameCritCopyQueue.MakeSyncPointWithSignal();
            asyncComputeQueue.ExecuteCommandList(*clearTileBitfieldsCmdList);
            GpuSyncPoint clearTileBitsSyncPoint = asyncComputeQueue.MakeSyncPointWithSignal();
            asyncComputeQueue.Wait(copyLightIdxSyncPoint);
            asyncComputeQueue.Wait(clearTileBitsSyncPoint);
            asyncComputeQueue.Wait(sceneProxyRepSyncPoint);
            asyncComputeQueue.ExecuteCommandList(*lightClusteringCmdList);
        }
        GpuSyncPoint lightClusteringSyncPoint = asyncComputeQueue.MakeSyncPointWithSignal();
        /*********************/

        /* Mesh Instance Pass */
        const U32 numMeshInstances = sceneProxy->GetNumMeshInstances();
        GpuSyncPoint meshInstancePassSyncPoint{};
        {
            MeshInstancePassParams meshInstancePassParams{};
            meshInstancePassParams.PerFrameParamsCbv = perFrameParamsCbvPtr->Index;
            meshInstancePassParams.MeshInstanceIndicesBufferSrv = renderContext->Lookup(sceneProxy->GetMeshInstanceIndicesBufferSrv())->Index;
            meshInstancePassParams.NumMeshInstances = sceneProxy->GetNumMeshInstances();
            meshInstancePassParams.OpaqueMeshInstanceDispatchBufferUav = renderContext->Lookup(opaqueMeshInstanceDispatchStorage->GetUnorderedResourceView())->Index;
            meshInstancePassParams.TransparentMeshInstanceDispatchBufferUav = renderContext->Lookup(transparentMeshInstanceDispatchStorage->GetUnorderedResourceView())->Index;

            auto meshInstancePassCmdList = asyncComputeCmdListPool.Request(localFrameIdx, "MeshInstancePass"_fs);
            meshInstancePassCmdList->Open(meshInstancePassPso.get());

            GpuBuffer* opaqueMeshInstanceDispatchBufferPtr = renderContext->Lookup(opaqueMeshInstanceDispatchStorage->GetGpuBuffer());
            const GpuBufferDesc& opaqueMeshInstanceDispatchBufferDesc = opaqueMeshInstanceDispatchBufferPtr->GetDesc();
            meshInstancePassCmdList->AddPendingBufferBarrier(
                *zeroFilledBuffer,
                D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COPY,
                D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_COPY_SOURCE);
            meshInstancePassCmdList->AddPendingBufferBarrier(
                *opaqueMeshInstanceDispatchBufferPtr,
                D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COPY,
                D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_COPY_DEST);
            meshInstancePassCmdList->FlushBarriers();

            meshInstancePassCmdList->CopyBuffer(
                *zeroFilledBuffer, 0, GpuBufferDesc::kUavCounterSize,
                *opaqueMeshInstanceDispatchBufferPtr, opaqueMeshInstanceDispatchBufferDesc.GetUavCounterOffset());

            meshInstancePassCmdList->AddPendingBufferBarrier(
                *opaqueMeshInstanceDispatchBufferPtr,
                D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_SYNC_COMPUTE_SHADING,
                D3D12_BARRIER_ACCESS_COPY_DEST, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS);
            meshInstancePassCmdList->FlushBarriers();

            auto descriptorHeaps = renderContext->GetBindlessDescriptorHeaps();
            meshInstancePassCmdList->SetDescriptorHeaps(MakeSpan(descriptorHeaps));
            meshInstancePassCmdList->SetRootSignature(*bindlessRootSignature);
            meshInstancePassCmdList->SetRoot32BitConstants(0, meshInstancePassParams, 0);

            if (numMeshInstances > 0)
            {
                constexpr U32 kNumThreadsPerGroup = 32;
                const U32 numDispatchX = (meshInstancePassParams.NumMeshInstances + kNumThreadsPerGroup - 1) / kNumThreadsPerGroup;
                meshInstancePassCmdList->Dispatch(numDispatchX, 1, 1);
            }
            meshInstancePassCmdList->Close();

            CommandList* cmdLists[]{meshInstancePassCmdList};
            asyncComputeQueue.Wait(sceneProxyRepSyncPoint);
            asyncComputeQueue.ExecuteCommandLists(cmdLists);
            meshInstancePassSyncPoint = asyncComputeQueue.MakeSyncPointWithSignal();
        }
        /*********************/
        GpuBuffer* opaqueMeshInstanceDispatchStorageBuffer = renderContext->Lookup(opaqueMeshInstanceDispatchStorage->GetGpuBuffer());
        const GpuView* dsvPtr = renderContext->Lookup(dsv);
        /* Z-Pre Pass */
        {
            auto zPrePassCmdList = mainGfxCmdListPool.Request(localFrameIdx, "ZPrePass"_fs);
            zPrePass->SetParams(ZPrePassParams{
                .GfxCmdList = zPrePassCmdList,
                .DispatchMeshInstanceCmdSignature = dispatchMeshInstanceCmdSignature.get(),
                .DispatchOpaqueMeshInstanceStorageBuffer = opaqueMeshInstanceDispatchStorageBuffer,
                .Dsv = dsvPtr,
                .MainViewport = mainViewport
            });
            zPrePass->Record(localFrameIdx);

            mainGfxQueue.Wait(meshInstancePassSyncPoint);
            mainGfxQueue.ExecuteCommandList(*zPrePassCmdList);
        }
        GpuSyncPoint zPrePassSyncPoint = mainGfxQueue.MakeSyncPointWithSignal();

        /* Forward Mesh Instance Pass */
        {
            auto renderCmdList = mainGfxCmdListPool.Request(localFrameIdx, "ForwardMeshInstance"_fs);

            forwardOpaqueMeshRenderPass->SetParams(
                ForwardOpaqueMeshRenderPassParams{
                    .MainGfxCmdList = renderCmdList,
                    .DispatchOpaqueMeshInstanceStorageBuffer = opaqueMeshInstanceDispatchStorageBuffer,
                    .RenderTarget = backBuffer,
                    .RenderTargetView = backBufferRtv,
                    .Dsv = dsvPtr,
                    .TargetViewport = mainViewport
                });

            forwardOpaqueMeshRenderPass->Record(localFrameIdx);

            CommandList* renderCmdLists[]{renderCmdList};
            mainGfxQueue.Wait(zPrePassSyncPoint);
            mainGfxQueue.Wait(lightClusteringSyncPoint);
            mainGfxQueue.ExecuteCommandLists(renderCmdLists);
        }

        /*********************/

        /* ImGui Render Pass */
        IG_CHECK(imguiRenderPass != nullptr);
        auto imguiRenderCmdList = mainGfxCmdListPool.Request(localFrameIdx, "ImGuiRenderCmdList"_fs);
        imguiRenderPass->SetParams({
            .CmdList = imguiRenderCmdList,
            .BackBuffer = swapchain.GetBackBuffer(),
            .BackBufferRtv = swapchain.GetBackBufferRtv(),
            .MainViewport = mainViewport
        });
        imguiRenderPass->Record(localFrameIdx);

        CommandList* cmdLists[]{imguiRenderCmdList};
        GpuSyncPoint prevPassSyncPoint = mainGfxQueue.MakeSyncPointWithSignal();
        mainGfxQueue.Wait(prevPassSyncPoint);
        mainGfxQueue.ExecuteCommandLists(cmdLists);

        swapchain.Present();
        return mainGfxQueue.MakeSyncPointWithSignal();
        /*********************/
    }
} // namespace ig
