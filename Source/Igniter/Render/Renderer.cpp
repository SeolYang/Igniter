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
#include "Igniter/Render/RenderPass/LightClusteringPass.h"
#include "Igniter/Render/RenderPass/FrustumCullingPass.h"
#include "Igniter/Render/RenderPass/CompactMeshLodInstancesPass.h"
#include "Igniter/Render/RenderPass/GenerateMeshLodDrawCommandsPass.h"
#include "Igniter/Render/RenderPass/ZPrePass.h"
#include "Igniter/Render/RenderPass/TestForwardShadingPass.h"
#include "Igniter/Render/RenderPass/ImGuiRenderPass.h"
#include "Igniter/Render/RenderPass/DebugLightClusterPass.h"
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

        U32 LightStorageSrv = IG_NUMERIC_MAX_OF(LightStorageSrv);
        U32 LightIdxListSrv = IG_NUMERIC_MAX_OF(LightIdxListSrv);
        U32 LightTileBitfieldBufferSrv = IG_NUMERIC_MAX_OF(LightTileBitfieldBufferSrv);
        U32 LightDepthBinBufferSrv = IG_NUMERIC_MAX_OF(LightDepthBinBufferSrv);
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

        GpuTextureDesc depthStencilDesc;
        depthStencilDesc.DebugName = "DepthStencilBufferTex"_fs;
        depthStencilDesc.AsDepthStencil(static_cast<U32>(mainViewport.width), static_cast<U32>(mainViewport.height), DXGI_FORMAT_D32_FLOAT, true);
        depthStencilDesc.InitialLayout = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ;

        depthStencil = renderContext.CreateTexture(depthStencilDesc);
        dsv = renderContext.CreateDepthStencilView(depthStencil,
                                                   D3D12_TEX2D_DSV{.MipSlice = 0});

        GpuBufferDesc zeroFilledBufferDesc{};
        zeroFilledBufferDesc.AsUploadBuffer(kZeroFilledBufferSize);
        zeroFilledBufferDesc.DebugName = "ZeroFilledBuffer"_fs;
        zeroFilledBuffer = MakePtr<GpuBuffer>(gpuDevice.CreateBuffer(zeroFilledBufferDesc).value());
        U8* const mappedZeroFilledBuffer = zeroFilledBuffer->Map();
        ZeroMemory(mappedZeroFilledBuffer, kZeroFilledBufferSize);
        zeroFilledBuffer->Unmap();

        lightClusteringPass = MakePtr<LightClusteringPass>(renderContext, sceneProxy, *bindlessRootSignature, mainViewport);
        frustumCullingPass = MakePtr<FrustumCullingPass>(renderContext, *bindlessRootSignature);
        compactMeshLodInstancesPass = MakePtr<CompactMeshLodInstancesPass>(renderContext, *bindlessRootSignature);
        generateMeshLodDrawCmdsPass = MakePtr<GenerateMeshLodDrawCommandsPass>(renderContext, *bindlessRootSignature);
        zPrePass = MakePtr<ZPrePass>(renderContext, *bindlessRootSignature);
        testForwardShadingPass = MakePtr<TestForwardShadingPass>(renderContext, *bindlessRootSignature);
        imguiRenderPass = MakePtr<ImGuiRenderPass>(renderContext);
    }

    Renderer::~Renderer()
    {
        renderContext->DestroyGpuView(dsv);
        renderContext->DestroyTexture(depthStencil);
    }

    void Renderer::PreRender(const LocalFrameIndex localFrameIdx)
    {
        tempConstantBufferAllocator->Reset(localFrameIdx);
    }

    GpuSyncPoint Renderer::Render(const LocalFrameIndex localFrameIdx, const World& world, GpuSyncPoint sceneProxyRepSyncPoint)
    {
        IG_CHECK(renderContext != nullptr);
        IG_CHECK(meshStorage != nullptr);
        IG_CHECK(sceneProxy != nullptr);
        ZoneScoped;

        // 프레임 마다 업데이트 되는 버퍼 데이터 업데이트
        PerFrameConstants perFrameConstants{};

        const Registry& registry = world.GetRegistry();
        const auto cameraView = registry.view<const CameraComponent, const TransformComponent>();
        bool bMainCameraExists = false;
        Vector3 camWorldPos;
        Matrix camViewMat{};
        Matrix camProjMat{};
        float camNearPlane = 0.f;
        float camFarPlane = 0.f;
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

            camWorldPos = transformComponent.Position;
            camViewMat = transformComponent.CreateView();
            camProjMat = camera.CreatePerspectiveForReverseZ();
            camNearPlane = camera.NearZ;
            camFarPlane = camera.FarZ;

            perFrameConstants.View = ConvertToShaderSuitableForm(camViewMat);
            perFrameConstants.ViewProj = ConvertToShaderSuitableForm(camViewMat * camProjMat);
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

        const GpuView* staticMeshVertexStorageSrv = renderContext->Lookup(meshStorage->GetStaticMeshVertexStorageSrv());
        IG_CHECK(staticMeshVertexStorageSrv != nullptr && staticMeshVertexStorageSrv->IsValid());
        perFrameConstants.StaticMeshVertexStorageSrv = staticMeshVertexStorageSrv->Index;

        const GpuView* vertexIndexStorageSrv = renderContext->Lookup(meshStorage->GetIndexStorageSrv());
        IG_CHECK(vertexIndexStorageSrv != nullptr && vertexIndexStorageSrv->IsValid());
        perFrameConstants.VertexIndexStorageSrv = vertexIndexStorageSrv->Index;

        const GpuView* transformStorageSrv = renderContext->Lookup(sceneProxy->GetTransformProxyStorageSrv());
        IG_CHECK(transformStorageSrv != nullptr && transformStorageSrv->IsValid());
        perFrameConstants.TransformStorageSrv = transformStorageSrv->Index;

        const GpuView* materialStorageSrv = renderContext->Lookup(sceneProxy->GetMaterialProxyStorageSrv());
        IG_CHECK(materialStorageSrv != nullptr && materialStorageSrv->IsValid());
        perFrameConstants.MaterialStorageSrv = materialStorageSrv->Index;

        const GpuView* meshProxyStorageSrv = renderContext->Lookup(sceneProxy->GetMeshProxySrv());
        IG_CHECK(meshProxyStorageSrv != nullptr && meshProxyStorageSrv->IsValid());
        perFrameConstants.MeshStorageSrv = meshProxyStorageSrv->Index;

        const GpuView* instancingStorageSrv = renderContext->Lookup(sceneProxy->GetInstancingStorageSrv());
        const GpuView* instancingStorageUav = renderContext->Lookup(sceneProxy->GetInstancingStorageUav());
        IG_CHECK(instancingStorageSrv != nullptr && instancingStorageSrv->IsValid());
        IG_CHECK(instancingStorageUav != nullptr && instancingStorageUav->IsValid());
        perFrameConstants.InstancingDataStorageSrv = instancingStorageSrv->Index;
        perFrameConstants.InstancingDataStorageUav = instancingStorageUav->Index;

        const GpuView* indirectTransformStorageSrv = renderContext->Lookup(sceneProxy->GetIndirectTransformStorageSrv());
        const GpuView* indirectTransformStorageUav = renderContext->Lookup(sceneProxy->GetIndirectTransformStorageUav());
        IG_CHECK(indirectTransformStorageSrv != nullptr && indirectTransformStorageSrv->IsValid());
        IG_CHECK(indirectTransformStorageUav != nullptr && indirectTransformStorageUav->IsValid());
        perFrameConstants.IndirectTransformStorageSrv = indirectTransformStorageSrv->Index;
        perFrameConstants.IndirectTransformStorageUav = indirectTransformStorageUav->Index;

        const GpuView* renderableStorageSrv = renderContext->Lookup(sceneProxy->GetRenderableProxyStorageSrv());
        IG_CHECK(renderableStorageSrv != nullptr && renderableStorageSrv->IsValid());
        perFrameConstants.RenderableStorageSrv = renderableStorageSrv->Index;

        const GpuView* renderableIndicesBufferSrv = renderContext->Lookup(sceneProxy->GetRenderableIndicesSrv());
        IG_CHECK(renderableIndicesBufferSrv != nullptr && renderableIndicesBufferSrv->IsValid());
        perFrameConstants.RenderableIndicesBufferSrv = renderableIndicesBufferSrv->Index;
        perFrameConstants.NumMaxRenderables = sceneProxy->GetNumRenderables();

        const GpuView* lightStorageSrv = renderContext->Lookup(sceneProxy->GetLightStorageSrv());
        IG_CHECK(lightStorageSrv != nullptr);
        perFrameConstants.LightStorageSrv = lightStorageSrv->Index;

        const GpuView* lightIdxListSrv = renderContext->Lookup(lightClusteringPass->GetLightIdxListSrv());
        IG_CHECK(lightIdxListSrv != nullptr);
        perFrameConstants.LightIdxListSrv = lightIdxListSrv->Index;

        const GpuView* lightTileBitfieldBufferSrv = renderContext->Lookup(lightClusteringPass->GetTilesBufferSrv());
        IG_CHECK(lightTileBitfieldBufferSrv != nullptr);
        perFrameConstants.LightTileBitfieldBufferSrv = lightTileBitfieldBufferSrv->Index;

        const GpuView* lightDepthBinBufferSrv = renderContext->Lookup(lightClusteringPass->GetDepthBinsBufferSrv());
        IG_CHECK(lightDepthBinBufferSrv != nullptr);
        perFrameConstants.LightDepthBinBufferSrv = lightDepthBinBufferSrv->Index;

        perFrameConstants.MinMeshLod = minMeshLod;

        TempConstantBuffer perFrameConstantBuffer = tempConstantBufferAllocator->Allocate<PerFrameConstants>(localFrameIdx);
        const GpuView* perFrameCbv = renderContext->Lookup(perFrameConstantBuffer.GetConstantBufferView());
        IG_CHECK(perFrameCbv != nullptr && perFrameCbv->IsValid());
        perFrameConstantBuffer.Write(perFrameConstants);

        Swapchain& swapchain = renderContext->GetSwapchain();
        GpuTexture* backBuffer = renderContext->Lookup(swapchain.GetBackBuffer());
        const GpuView* backBufferRtv = renderContext->Lookup(swapchain.GetBackBufferRtv());

        CommandQueue& mainGfxQueue{renderContext->GetMainGfxQueue()};
        GpuFence& mainGfxFence = renderContext->GetMainGfxFence();
        CommandListPool& mainGfxCmdListPool = renderContext->GetMainGfxCommandListPool();
        auto renderCmdList = mainGfxCmdListPool.Request(localFrameIdx, "MainGfxRender"_fs);
        if (sceneProxy->GetNumRenderables() == 0 || !bMainCameraExists)
        {
            renderCmdList->Open();

            renderCmdList->AddPendingTextureBarrier(
                *backBuffer,
                D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_RENDER_TARGET,
                D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_RENDER_TARGET,
                D3D12_BARRIER_LAYOUT_PRESENT, D3D12_BARRIER_LAYOUT_RENDER_TARGET);
            renderCmdList->FlushBarriers();

            renderCmdList->ClearRenderTarget(*backBufferRtv);
            GpuView* dsvPtr = renderContext->Lookup(dsv);
            renderCmdList->ClearDepth(*dsvPtr, 0.f);
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
            IG_CHECK(frustumCullingPass != nullptr);
            IG_CHECK(compactMeshLodInstancesPass != nullptr);
            IG_CHECK(generateMeshLodDrawCmdsPass != nullptr);
            IG_CHECK(testForwardShadingPass != nullptr);

            CommandQueue& asyncComputeQueue = renderContext->GetAsyncComputeQueue();
            GpuFence& asyncComputeFence = renderContext->GetAsyncComputeFence();
            CommandListPool& asyncComputeCmdListPool = renderContext->GetAsyncComputeCommandListPool();

            CommandQueue& asyncCopyQueue = renderContext->GetAsyncCopyQueue();
            GpuFence& asyncCopyFence = renderContext->GetAsyncCopyFence();
            CommandListPool& asyncCopyCmdListPool = renderContext->GetAsyncCopyCommandListPool();

            auto lightClusteringInitCopyCmdList = asyncCopyCmdListPool.Request(localFrameIdx, "LightClusteringInitCopyCmdList"_fs);
            auto clearTileDwordsBufferCmdList = asyncComputeCmdListPool.Request(localFrameIdx, "ClearTileDwordsBufferCmdList"_fs);
            auto lightClusteringCmdList = asyncComputeCmdListPool.Request(localFrameIdx, "LightClusteringCmdList"_fs);
            {
                auto gpuParamsConstantBuffer = tempConstantBufferAllocator->Allocate<LightClusteringPassGpuParams>(localFrameIdx);

                lightClusteringPass->SetParams(
                    {.InitCopyCmdList = lightClusteringInitCopyCmdList,
                     .ClearTileDwordsBufferCmdList = clearTileDwordsBufferCmdList,
                     .LightClusteringCmdList = lightClusteringCmdList,
                     .CamWorldPos = camWorldPos,
                     .ViewMat = camViewMat,
                     .ProjMat = camProjMat,
                     .NearPlane = camNearPlane,
                     .FarPlane = camFarPlane,
                     .TargetViewport = mainViewport,
                     .PerFrameCbvPtr = perFrameCbv,
                     .GpuParamsConstantBuffer = &gpuParamsConstantBuffer});

                lightClusteringPass->Execute(localFrameIdx);

                CommandList* lightIdxListCopyCmdLists[]{lightClusteringInitCopyCmdList};
                asyncCopyQueue.ExecuteCommandLists(lightIdxListCopyCmdLists);
                GpuSyncPoint lightIdxListCopySyncPoint = asyncCopyQueue.MakeSyncPointWithSignal(asyncCopyFence);

                CommandList* clearTileDwordsBufferCmdLists[]{clearTileDwordsBufferCmdList};
                asyncComputeQueue.ExecuteCommandLists(clearTileDwordsBufferCmdLists);
                GpuSyncPoint clearTileDwordsBufferSyncPoint = asyncComputeQueue.MakeSyncPointWithSignal(asyncComputeFence);

                CommandList* lightClusteringCmdLists[]{lightClusteringCmdList};
                asyncComputeQueue.Wait(lightIdxListCopySyncPoint);
                asyncComputeQueue.Wait(clearTileDwordsBufferSyncPoint);
                asyncComputeQueue.Wait(sceneProxyRepSyncPoint);
                asyncComputeQueue.ExecuteCommandLists(lightClusteringCmdLists);
            }

            auto frustumCullingCmdList =
                asyncComputeCmdListPool.Request(localFrameIdx, "FrustumCullingCmdList"_fs);
            {
                frustumCullingPass->SetParams({.CmdList = frustumCullingCmdList,
                                               .ZeroFilledBufferPtr = zeroFilledBuffer.get(),
                                               .PerFrameCbvPtr = perFrameCbv,
                                               .NumRenderables =
                                                   sceneProxy->GetNumRenderables()});
                frustumCullingPass->Execute(localFrameIdx);

                CommandList* cmdLists[]{frustumCullingCmdList};
                asyncComputeQueue.Wait(sceneProxyRepSyncPoint);
                asyncComputeQueue.ExecuteCommandLists(cmdLists);
            }

            auto compactMeshLodInstancesCmdList =
                asyncComputeCmdListPool.Request(localFrameIdx, "CompactMeshLodInstancesCmdList"_fs);
            {
                compactMeshLodInstancesPass->SetParams({.CmdList = compactMeshLodInstancesCmdList,
                                                        .PerFrameCbvPtr = perFrameCbv,
                                                        .MeshLodInstanceStorageUavHandle =
                                                            frustumCullingPass->GetMeshLodInstanceStorageUav(),
                                                        .CullingDataBufferSrvHandle =
                                                            frustumCullingPass->GetCullingDataBufferSrv(),
                                                        .NumRenderables =
                                                            sceneProxy->GetNumRenderables()});
                compactMeshLodInstancesPass->Execute(localFrameIdx);

                CommandList* cmdLists[]{compactMeshLodInstancesCmdList};
                GpuSyncPoint prevPassSyncPoint = asyncComputeQueue.MakeSyncPointWithSignal(asyncComputeFence);
                asyncComputeQueue.Wait(prevPassSyncPoint);
                asyncComputeQueue.ExecuteCommandLists(cmdLists);
            }

            auto generateMeshLodDrawCmdsCmdList =
                asyncComputeCmdListPool.Request(localFrameIdx, "GenMeshLodDrawCmdsCmdList"_fs);
            {
                generateMeshLodDrawCmdsPass->SetParams({.CmdList = generateMeshLodDrawCmdsCmdList,
                                                        .PerFrameCbvPtr = perFrameCbv,
                                                        .CullingDataBufferSrv = frustumCullingPass->GetCullingDataBufferSrv(),
                                                        .ZeroFilledBufferPtr = zeroFilledBuffer.get(),
                                                        .NumInstancing = sceneProxy->GetNumInstancing()});
                generateMeshLodDrawCmdsPass->Execute(localFrameIdx);

                CommandList* cmdLists[]{generateMeshLodDrawCmdsCmdList};
                GpuSyncPoint prevPassSyncPoint = asyncComputeQueue.MakeSyncPointWithSignal(asyncComputeFence);
                asyncComputeQueue.Wait(prevPassSyncPoint);
                asyncComputeQueue.ExecuteCommandLists(cmdLists);
            }
            GpuSyncPoint generateMeshLodCmdsSyncPoint = asyncComputeQueue.MakeSyncPointWithSignal(asyncComputeFence);

            auto zPrePassCmdList =
                mainGfxCmdListPool.Request(localFrameIdx, "ZPrePass"_fs);
            {
                zPrePass->SetParams({.CmdList = zPrePassCmdList,
                                     .DrawInstanceCmdSignature =
                                         generateMeshLodDrawCmdsPass->GetCommandSignature(),
                                     .DrawInstanceCmdStorageBuffer =
                                         generateMeshLodDrawCmdsPass->GetDrawInstanceCmdStorageBuffer(),
                                     .DepthTex = depthStencil,
                                     .Dsv = dsv,
                                     .MainViewport = mainViewport});

                zPrePass->Execute(localFrameIdx);
                mainGfxQueue.Wait(generateMeshLodCmdsSyncPoint);
                CommandList* cmdLists[]{zPrePassCmdList};
                mainGfxQueue.ExecuteCommandLists(cmdLists);
            }

            auto testForwardPassCmdList =
                mainGfxCmdListPool.Request(localFrameIdx, "ForwardShading"_fs);
            {
                testForwardShadingPass->SetParams({.CmdList = testForwardPassCmdList,
                                                   .DrawInstanceCmdSignature =
                                                       generateMeshLodDrawCmdsPass->GetCommandSignature(),
                                                   .DrawInstanceCmdStorageBuffer =
                                                       generateMeshLodDrawCmdsPass->GetDrawInstanceCmdStorageBuffer(),
                                                   .BackBuffer = swapchain.GetBackBuffer(),
                                                   .BackBufferRtv = swapchain.GetBackBufferRtv(),
                                                   .DepthTex = depthStencil,
                                                   .Dsv = dsv,
                                                   .MainViewport = mainViewport});
                testForwardShadingPass->Execute(localFrameIdx);

                // 이미 이 시점에서 LightClustering은 완료 되었음.
                GpuSyncPoint prevPassSyncPoint = mainGfxQueue.MakeSyncPointWithSignal(mainGfxFence);
                mainGfxQueue.Wait(prevPassSyncPoint);
                CommandList* cmdLists[]{testForwardPassCmdList};
                mainGfxQueue.ExecuteCommandLists(cmdLists);
            }
        }

        IG_CHECK(imguiRenderPass != nullptr);
        auto imguiRenderCmdList =
            mainGfxCmdListPool.Request(localFrameIdx, "ImGuiRenderCmdList"_fs);
        {
            imguiRenderPass->SetParams({.CmdList = imguiRenderCmdList,
                                        .BackBuffer = swapchain.GetBackBuffer(),
                                        .BackBufferRtv = swapchain.GetBackBufferRtv(),
                                        .MainViewport = mainViewport});
            imguiRenderPass->Execute(localFrameIdx);

            CommandList* cmdLists[]{imguiRenderCmdList};
            GpuSyncPoint prevPassSyncPoint = mainGfxQueue.MakeSyncPointWithSignal(mainGfxFence);
            mainGfxQueue.Wait(prevPassSyncPoint);
            mainGfxQueue.ExecuteCommandLists(cmdLists);
        }

        swapchain.Present();
        return mainGfxQueue.MakeSyncPointWithSignal(mainGfxFence);
    }
} // namespace ig
