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
#include "Igniter/Render/RenderPass/MeshInstancePass.h"
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

        // [1]: Real-Time Rendering 4th ed. 22.14.1 Frustum Plane Extraction
        // [2]: https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
        // [3]: https://iquilezles.org/articles/frustum/
        // (CameraPos.xyz, InverseAspectRatio)
        // (cos(fovy*0.5f), sin(fovy*0.5f), Near, Far)
        // View Frustum (in view space) =>
        // r = 1/aspect_ratio
        // Left = (r*cos, 0, sin, 0)
        // Right = (-r*cos, 0, sin, 0)
        // Bottom = (0, cos, sin, 0)
        // Top = (0, -cos, sin, 0)
        // Near = (0, 0, 1, -N)
        // Far = (0, 0, -1, F)
        /* (x,y,z): cam pos, w: inv aspect ratio */
        Vector4 CamWorldPosInvAspectRatio;
        /* x: cos(fovy/2), y: sin(fovy/2), z: near, w: far */
        Vector4 ViewFrustumParams;

        float ViewportWidth;
        float ViewportHeight;
    };

    struct GenerateDepthPyramidConstants
    {
        U32 PrevMipUav;
        U32 CurrMipUav;

        U32 PrevMipWidth;
        U32 PrevMipHeight;
        U32 CurrMipWidth;
        U32 CurrMipHeight;
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
        depthBuffer = renderContext.CreateTexture(depthStencilDesc);
        depthBufferDsv = renderContext.CreateDepthStencilView(depthBuffer, D3D12_TEX2D_DSV{.MipSlice = 0});
        const GpuTexture* depthBufferPtr = renderContext.Lookup(depthBuffer);
        depthBufferFootprints = gpuDevice.GetCopyableFootprints(depthBufferPtr->GetDesc(), 0, 1, 0);

        /* @todo Depth Pyramid 클래스로 따로 빼내버리기 */
        const float mainViewportLongestExtent = std::max(mainViewport.width, mainViewport.height);
        /* MIP0을 포함 해야 하므로 +1 */
        const U16 maxDepthPyramidMipLevels = (U16)std::log2(mainViewportLongestExtent) + 1;
        IG_CHECK(maxDepthPyramidMipLevels > 0);
        GpuTextureDesc depthPyramidDesc{};
        depthPyramidDesc.DebugName = "DepthPyramid";
        depthPyramidDesc.AsTexture2D((U32)mainViewport.width, (U32)mainViewport.height, maxDepthPyramidMipLevels, DXGI_FORMAT_R32_FLOAT, true);
        depthPyramidDesc.InitialLayout = D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;
        depthPyramid = renderContext.CreateTexture(depthPyramidDesc);

        depthPyramidExtents.resize(maxDepthPyramidMipLevels);
        depthPyramidMipsUav.resize(maxDepthPyramidMipLevels);

        depthPyramidSrv = renderContext.CreateShaderResourceView(
            depthPyramid,
            D3D12_TEX2D_SRV{.MostDetailedMip = 0, .MipLevels = kAllMipLevels, .PlaneSlice = 0, .ResourceMinLODClamp = 0.f});

        depthPyramidExtents[0] = {.X = (U32)mainViewport.width, .Y = (U32)mainViewport.height};
        for (U16 depthPyramidMipLevel = 0; depthPyramidMipLevel < maxDepthPyramidMipLevels; ++depthPyramidMipLevel)
        {
            if (depthPyramidMipLevel > 0)
            {
                depthPyramidExtents[depthPyramidMipLevel] = {
                    .X = std::max(1Ui32, depthPyramidExtents[depthPyramidMipLevel - 1].X >> 1),
                    .Y = std::max(1Ui32, depthPyramidExtents[depthPyramidMipLevel - 1].Y >> 1)
                };
            }

            depthPyramidMipsUav[depthPyramidMipLevel] = renderContext.CreateUnorderedAccessView(
                depthPyramid,
                D3D12_TEX2D_UAV{.MipSlice = depthPyramidMipLevel, .PlaneSlice = 0});
        }
        const GpuTexture* depthPyramidPtr = renderContext.Lookup(depthPyramid);
        depthPyramidFootprints = gpuDevice.GetCopyableFootprints(depthPyramidPtr->GetDesc(), 0, maxDepthPyramidMipLevels, 0);

        GpuBufferDesc zeroFilledBufferDesc{};
        zeroFilledBufferDesc.AsUploadBuffer(kZeroFilledBufferSize);
        zeroFilledBufferDesc.DebugName = "ZeroFilledBuffer"_fs;
        zeroFilledBuffer = MakePtr<GpuBuffer>(gpuDevice.CreateBuffer(zeroFilledBufferDesc).value());
        U8* const mappedZeroFilledBuffer = zeroFilledBuffer->Map();
        ZeroMemory(mappedZeroFilledBuffer, kZeroFilledBufferSize);
        zeroFilledBuffer->Unmap();

        GpuStorageDesc dispatchMeshInstanceStorageDesc{};
        dispatchMeshInstanceStorageDesc.ElementSize = sizeof(DispatchMeshInstance);
        dispatchMeshInstanceStorageDesc.NumInitElements = kInitDispatchBufferCapacity;
        dispatchMeshInstanceStorageDesc.Flags = EGpuStorageFlags::ShaderReadWrite |
            EGpuStorageFlags::EnableUavCounter |
            EGpuStorageFlags::EnableLinearAllocation;

        dispatchMeshInstanceStorageDesc.DebugName = "DispatchOpaqueMeshInstanceStorage"_fs;
        opaqueMeshInstanceDispatchStorage = MakePtr<GpuStorage>(renderContext, dispatchMeshInstanceStorageDesc);

        dispatchMeshInstanceStorageDesc.DebugName = "DispatchTransparentMeshInstanceStorage"_fs;
        transparentMeshInstanceDispatchStorage = MakePtr<GpuStorage>(renderContext, dispatchMeshInstanceStorageDesc);

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
        meshInstancePass = MakePtr<MeshInstancePass>(renderContext, *bindlessRootSignature);
        zPrePass = MakePtr<ZPrePass>(renderContext, *bindlessRootSignature);
        forwardOpaqueMeshRenderPass = MakePtr<ForwardOpaqueMeshRenderPass>(renderContext, *bindlessRootSignature, *dispatchMeshInstanceCmdSignature);
        imguiRenderPass = MakePtr<ImGuiRenderPass>(renderContext);

        ShaderCompileDesc genDepthPyramidShaderDesc{.SourcePath = "Assets/Shaders/GenerateDepthPyramid.hlsl", .Type = EShaderType::Compute};
        generateDepthPyramidShader = MakePtr<ShaderBlob>(genDepthPyramidShaderDesc);

        ComputePipelineStateDesc genDepthPyramidPsoDesc{};
        genDepthPyramidPsoDesc.Name = "GenDepthPyramidPSO"_fs;
        genDepthPyramidPsoDesc.SetRootSignature(*bindlessRootSignature);
        genDepthPyramidPsoDesc.SetComputeShader(*generateDepthPyramidShader);
        generateDepthPyramidPso = MakePtr<PipelineState>(gpuDevice.CreateComputePipelineState(genDepthPyramidPsoDesc).value());

        depthPyramidSampler = renderContext.CreateSamplerView(
            D3D12_SAMPLER_DESC{
                .Filter = D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT,
                .AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                .AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                .AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                .MipLODBias = 0.f,
                .MinLOD = 0.f, .MaxLOD = FLT_MAX
            });
    }

    Renderer::~Renderer()
    {
        if (depthBufferDsv)
        {
            renderContext->DestroyGpuView(depthBufferDsv);
        }
        if (depthBuffer)
        {
            renderContext->DestroyTexture(depthBuffer);
        }
        if (depthPyramidSrv)
        {
            renderContext->DestroyGpuView(depthPyramidSrv);
        }
        for (const Handle<GpuView>& depthPyramidMipUav : depthPyramidMipsUav)
        {
            renderContext->DestroyGpuView(depthPyramidMipUav);
        }
        if (depthPyramidSampler)
        {
            renderContext->DestroyGpuView(depthPyramidSampler);
        }
        if (depthPyramid)
        {
            renderContext->DestroyTexture(depthPyramid);
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
        GpuTexture* backBufferPtr = renderContext->Lookup(swapchain.GetBackBuffer());
        const GpuView* backBufferRtvPtr = renderContext->Lookup(swapchain.GetBackBufferRtv());

        const GpuView* perFrameParamsCbvPtr = renderContext->Lookup(perFrameParamsCb.GetConstantBufferView());
        IG_CHECK(perFrameParamsCbvPtr != nullptr);

        /* Copy Depth Buffer for Depth Pyramid gen at next frame. */
        GpuTexture* depthBufferPtr = renderContext->Lookup(depthBuffer);
        GpuTexture* depthPyramidPtr = renderContext->Lookup(depthPyramid);
        {
            /*
             * 이 시점에 frameCritCopyQueue는 SceneProxy Replication 때문에 Busy한 상태
             * 해당 작업과 DepthPyramid 생성의 Overlap 최대화와,
             * Barrier 사용의 최소화(Queue 마다 호환되는 Layout set이 다르므로)를 위해
             * MainGfxQueue에서 Copy 및 Layout Transition 진행.
             * (GraphicsQueue는 모든 Layout의 전환이 가능하므로)
             */
            auto mainGfxCmdList = mainGfxCmdListPool.Request(localFrameIdx, "CopyDepthToDepthPyramidMip0"_fs);
            mainGfxCmdList->Open();
            mainGfxCmdList->AddPendingTextureBarrier(
                *depthBufferPtr,
                D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COPY,
                D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_COPY_SOURCE,
                D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE, D3D12_BARRIER_LAYOUT_COPY_SOURCE);
            /*
             * 이후, Depth Pyramid 생성 과정에서 매 과정마다 필요한 Mip Level에 대해 Barrier를 삽입 하는게 더 빠를 것 인가.
             * 아니면 Barrier를 최소화 하는게 더 빠를 것 인가.
             * 생성 => SHADER_RESOURCE
             * 복사 => COPY_DEST
             * Depth Pyramid Gen => UNORDERED_ACCESS
             * Occlusion Culling 사용 => SHADER_RESOURCE
             */
            mainGfxCmdList->AddPendingTextureBarrier(
                *depthPyramidPtr,
                D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COPY,
                D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_COPY_DEST,
                D3D12_BARRIER_LAYOUT_SHADER_RESOURCE, D3D12_BARRIER_LAYOUT_COPY_DEST);
            mainGfxCmdList->FlushBarriers();

            /* Depth Pyramid의 Mip 0에 이전 프레임의 Depth를 복사 */
            mainGfxCmdList->CopyTextureRegion(
                *depthBufferPtr, 0,
                *depthPyramidPtr, 0);

            /* Depth Buffer를 이후 과정에서 DCC의 혜택을 받을 수 있도록 미리 DEPTH_WRITE 상태로 변경 */
            mainGfxCmdList->AddPendingTextureBarrier(
                *depthBufferPtr,
                D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_SYNC_NONE,
                D3D12_BARRIER_ACCESS_COPY_SOURCE, D3D12_BARRIER_ACCESS_NO_ACCESS,
                D3D12_BARRIER_LAYOUT_COPY_SOURCE, D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE);
            mainGfxCmdList->AddPendingTextureBarrier(
                *depthPyramidPtr,
                D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_SYNC_NONE,
                D3D12_BARRIER_ACCESS_COPY_DEST, D3D12_BARRIER_ACCESS_NO_ACCESS,
                D3D12_BARRIER_LAYOUT_COPY_DEST, D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS);
            mainGfxCmdList->FlushBarriers();

            mainGfxCmdList->Close();

            mainGfxQueue.ExecuteCommandList(*mainGfxCmdList);
        }
        GpuSyncPoint copyDepthBufferSyncPoint = frameCritCopyQueue.MakeSyncPointWithSignal();

        /* Hi-Z Occlusion Culling을 위한 Depth Pyramid를 생성 */
        {
            auto descriptorHeaps = renderContext->GetBindlessDescriptorHeaps();
            const auto descriptorHeapsSpan = MakeSpan(descriptorHeaps);
            // 가장 간단한 방법으론 DepthPyramid.MipLevels-1 번 만큼의 Dispatch가 필요..
            // Dispatch를 최소화 하는 방법은?
            // SPD는 어떻게 구현되어 있는걸까?
            for (U16 depthPyramidMipLevel = 1; depthPyramidMipLevel < depthPyramidExtents.size(); ++depthPyramidMipLevel)
            {
                const GpuView* prevMipUav = renderContext->Lookup(depthPyramidMipsUav[depthPyramidMipLevel - 1]);
                const GpuView* currMipUav = renderContext->Lookup(depthPyramidMipsUav[depthPyramidMipLevel]);

                auto cmdList = asyncComputeCmdListPool.Request(localFrameIdx, String(std::format("GenDepthPyrmiad.{}", depthPyramidMipLevel)));
                cmdList->Open(generateDepthPyramidPso.get());
                cmdList->SetDescriptorHeaps(descriptorHeapsSpan);
                cmdList->SetRootSignature(*bindlessRootSignature);

                const Uint2 prevDepthPyramidMipExtent = depthPyramidExtents[depthPyramidMipLevel - 1];
                const Uint2 currDepthPyramidMipExtent = depthPyramidExtents[depthPyramidMipLevel];
                const GenerateDepthPyramidConstants constants
                {
                    .PrevMipUav = prevMipUav->Index,
                    .CurrMipUav = currMipUav->Index,
                    .PrevMipWidth = prevDepthPyramidMipExtent.X,
                    .PrevMipHeight = prevDepthPyramidMipExtent.Y,
                    .CurrMipWidth = currDepthPyramidMipExtent.X,
                    .CurrMipHeight = currDepthPyramidMipExtent.Y
                };
                cmdList->SetRoot32BitConstants(0, constants, 0);

                const Uint2 numThreadGroups{.X = (currDepthPyramidMipExtent.X + 7) / 8, .Y = (currDepthPyramidMipExtent.Y + 7) / 8};
                cmdList->Dispatch(numThreadGroups.X, numThreadGroups.Y, 1);

                // if depthPyramidMipLevel == last -> Barrier(UAV->SRV)
                if (depthPyramidMipLevel == (depthPyramidExtents.size() - 1))
                {
                    cmdList->AddPendingTextureBarrier(
                        *depthPyramidPtr,
                        D3D12_BARRIER_SYNC_COMPUTE_SHADING, D3D12_BARRIER_SYNC_NONE,
                        D3D12_BARRIER_ACCESS_UNORDERED_ACCESS, D3D12_BARRIER_ACCESS_NO_ACCESS,
                        D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS, D3D12_BARRIER_LAYOUT_SHADER_RESOURCE);
                    cmdList->FlushBarriers();
                }

                cmdList->Close();

                if (depthPyramidMipLevel == 1)
                {
                    asyncComputeQueue.Wait(copyDepthBufferSyncPoint);
                }
                else
                {
                    GpuSyncPoint prevMipSyncPoint = asyncComputeQueue.MakeSyncPointWithSignal();
                    asyncComputeQueue.Wait(prevMipSyncPoint);
                }
                asyncComputeQueue.ExecuteCommandList(*cmdList);
            }
        }
        GpuSyncPoint genDepthPyramidSyncPoint = asyncComputeQueue.MakeSyncPointWithSignal();

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
            auto meshInstancePassCmdList = asyncComputeCmdListPool.Request(localFrameIdx, "MeshInstancePass"_fs);
            meshInstancePass->SetParams(MeshInstancePassParams{
                .ComputeCmdList = meshInstancePassCmdList,
                .ZeroFilledBuffer = zeroFilledBuffer.get(),
                .PerFrameParamsCbv = perFrameParamsCbvPtr,
                .MeshInstanceIndicesBufferSrv = renderContext->Lookup(sceneProxy->GetMeshInstanceIndicesBufferSrv()),
                .DispatchOpaqueMeshInstanceStorageBuffer = renderContext->Lookup(opaqueMeshInstanceDispatchStorage->GetGpuBuffer()),
                .DispatchOpaqueMeshInstanceStorageUav = renderContext->Lookup(opaqueMeshInstanceDispatchStorage->GetUav()),
                .DispatchTransparentMeshInstanceStorageBuffer = nullptr,
                .DispatchTransparentMeshInstanceStorageUav = nullptr,
                .NumMeshInstances = numMeshInstances,
                .OpaqueMeshInstanceIndicesStorageUav = nullptr,
                .TransparentMeshInstanceIndicesStorageUav = nullptr,
                .DepthPyramidSrv = renderContext->Lookup(depthPyramidSrv),
                .DepthPyramidSampler = renderContext->Lookup(depthPyramidSampler),
                .DepthPyramidWidth = depthPyramidExtents[0].X,
                .DepthPyramidHeight = depthPyramidExtents[0].Y,
                .NumDepthPyramidMips = (U32)depthPyramidExtents.size()
            });
            meshInstancePass->Record(localFrameIdx);

            CommandList* cmdLists[]{meshInstancePassCmdList};
            asyncComputeQueue.Wait(sceneProxyRepSyncPoint);
            asyncComputeQueue.Wait(genDepthPyramidSyncPoint);
            asyncComputeQueue.ExecuteCommandLists(cmdLists);
            meshInstancePassSyncPoint = asyncComputeQueue.MakeSyncPointWithSignal();
        }
        /*********************/

        GpuBuffer* opaqueMeshInstanceDispatchStorageBuffer = renderContext->Lookup(opaqueMeshInstanceDispatchStorage->GetGpuBuffer());
        const GpuView* dsvPtr = renderContext->Lookup(depthBufferDsv);
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
                    .RenderTarget = backBufferPtr,
                    .RenderTargetView = backBufferRtvPtr,
                    .Dsv = dsvPtr,
                    .TargetViewport = mainViewport
                });

            forwardOpaqueMeshRenderPass->Record(localFrameIdx);

            CommandList* renderCmdLists[]{renderCmdList};
            mainGfxQueue.Wait(zPrePassSyncPoint);
            mainGfxQueue.Wait(lightClusteringSyncPoint);
            mainGfxQueue.ExecuteCommandLists(renderCmdLists);
        }
        GpuSyncPoint forwardOpaqueMeshRenderPassSyncPoint = mainGfxQueue.MakeSyncPointWithSignal();
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
        mainGfxQueue.Wait(forwardOpaqueMeshRenderPassSyncPoint);
        mainGfxQueue.ExecuteCommandLists(cmdLists);

        swapchain.Present();
        return mainGfxQueue.MakeSyncPointWithSignal();
        /*********************/
    }
} // namespace ig
