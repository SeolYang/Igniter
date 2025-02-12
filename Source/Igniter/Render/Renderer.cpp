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
        dsv = renderContext.CreateDepthStencilView(depthStencil,
            D3D12_TEX2D_DSV{.MipSlice = 0});

        GpuBufferDesc zeroFilledBufferDesc{};
        zeroFilledBufferDesc.AsUploadBuffer(kZeroFilledBufferSize);
        zeroFilledBufferDesc.DebugName = "ZeroFilledBuffer"_fs;
        zeroFilledBuffer = MakePtr<GpuBuffer>(gpuDevice.CreateBuffer(zeroFilledBufferDesc).value());
        U8* const mappedZeroFilledBuffer = zeroFilledBuffer->Map();
        ZeroMemory(mappedZeroFilledBuffer, kZeroFilledBufferSize);
        zeroFilledBuffer->Unmap();

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

    GpuSyncPoint Renderer::Render(const LocalFrameIndex localFrameIdx, [[maybe_unused]] const World& world, [[maybe_unused]] GpuSyncPoint sceneProxyRepSyncPoint)
    {
        IG_CHECK(renderContext != nullptr);
        IG_CHECK(sceneProxy != nullptr);
        ZoneScoped;

        Swapchain& swapchain = renderContext->GetSwapchain();
        GpuTexture* backBuffer = renderContext->Lookup(swapchain.GetBackBuffer());
        const GpuView* backBufferRtv = renderContext->Lookup(swapchain.GetBackBufferRtv());

        CommandQueue& mainGfxQueue{renderContext->GetMainGfxQueue()};
        GpuFence& mainGfxFence = renderContext->GetMainGfxFence();
        CommandListPool& mainGfxCmdListPool = renderContext->GetMainGfxCommandListPool();
        auto renderCmdList = mainGfxCmdListPool.Request(localFrameIdx, "MainGfxRender"_fs);
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

        renderCmdList->Close();
        CommandList* renderCmdLists[]{renderCmdList};
        mainGfxQueue.ExecuteCommandLists(renderCmdLists);

        IG_CHECK(imguiRenderPass != nullptr);
        auto imguiRenderCmdList =
            mainGfxCmdListPool.Request(localFrameIdx, "ImGuiRenderCmdList"_fs);
        {
            imguiRenderPass->SetParams({
                .CmdList = imguiRenderCmdList,
                .BackBuffer = swapchain.GetBackBuffer(),
                .BackBufferRtv = swapchain.GetBackBufferRtv(),
                .MainViewport = mainViewport
            });
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
