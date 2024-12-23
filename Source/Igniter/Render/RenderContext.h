#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/Handle.h"
#include "Igniter/Core/HandleStorage.h"
#include "Igniter/D3D12/GpuDevice.h"
#include "Igniter/D3D12/CommandQueue.h"
#include "Igniter/D3D12/GpuBuffer.h"
#include "Igniter/D3D12/GpuTexture.h"
#include "Igniter/D3D12/PipelineState.h"
#include "Igniter/Render/Common.h"
#include "Igniter/Render/CommandContextPool.h"
#include "Igniter/Render/GpuViewManager.h"
#include "Igniter/Render/GpuUploader.h"
#include "Igniter/Render/Swapchain.h"

namespace ig
{
    class Window;
    class FrameManager;
    class GpuBuffer;
    class GpuBufferDesc;
    class GpuTexture;
    class GpuTextureDesc;
    class PipelineState;

    class RenderContext final
    {
    private:
        // 각 타입에 정의하고, 각 타입에 대해 Create/Destroy에서 rw lock, Lookup 에선 read only lock
        // 각 방식(GpuViewManager,..) 내부 구현이 Thread Safe를 보장하지 않아도 됨
        // 할당은 여러 스레드에서 이뤄질 수 있지만, 해제는 결국 메인 스레드에서 진행됨
        template <typename Ty>
        struct ResourceManagePackage
        {
            mutable SharedMutex                                                Mut;
            HandleStorage<Ty, RenderContext>                                   Registry;
            eastl::array<Mutex, NumFramesInFlight>                             PendingListMut;
            eastl::array<eastl::vector<RenderResource<Ty>>, NumFramesInFlight> PendingDestroyList;
        };

    public:
        explicit RenderContext(const Window& window);
        ~RenderContext();

        RenderContext(const RenderContext&)     = delete;
        RenderContext(RenderContext&&) noexcept = delete;

        RenderContext& operator=(const RenderContext&)     = delete;
        RenderContext& operator=(RenderContext&&) noexcept = delete;

        [[nodiscard]] GpuDevice&          GetGpuDevice() { return gpuDevice; }
        [[nodiscard]] CommandQueue&       GetMainGfxQueue() { return mainGfxQueue; }
        [[nodiscard]] CommandQueue&       GetAsyncComputeQueue() { return asyncComputeQueue; }
        [[nodiscard]] CommandQueue&       GetAsyncCopyQueue() { return asyncComputeQueue; }
        [[nodiscard]] CommandContextPool& GetMainGfxCommandContextPool() { return mainGfxCmdCtxPool; }
        [[nodiscard]] CommandContextPool& GetAsyncComputeCommandContextPool() { return asyncComputeCmdCtxPool; }
        [[nodiscard]] CommandContextPool& GetAsyncCopyCommandContextPool() { return asyncCopyCmdCtxPool; }
        [[nodiscard]] GpuUploader&        GetGpuUploader() { return gpuUploader; }
        [[nodiscard]] Swapchain&          GetSwapchain() { return *swapchain; }
        [[nodiscard]] const Swapchain&    GetSwapchain() const { return *swapchain; }
        [[nodiscard]] auto&               GetCbvSrvUavDescriptorHeap() { return gpuViewManager.GetCbvSrvUavDescHeap(); }
        [[nodiscard]] auto                GetBindlessDescriptorHeaps() { return eastl::array<DescriptorHeap*, 2>{&gpuViewManager.GetCbvSrvUavDescHeap(), &gpuViewManager.GetSamplerDescHeap()}; }

        RenderResource<GpuBuffer>              CreateBuffer(const GpuBufferDesc& desc);
        RenderResource<GpuTexture>             CreateTexture(const GpuTextureDesc& desc);
        RenderResource<GpuTexture>             CreateTexture(GpuTexture&& gpuTexture);
        RenderResource<PipelineState>          CreatePipelineState(const GraphicsPipelineStateDesc& desc);
        RenderResource<PipelineState>          CreatePipelineState(const ComputePipelineStateDesc& desc);
        RenderResource<GpuView>                CreateConstantBufferView(const RenderResource<GpuBuffer> buffer);
        RenderResource<GpuView>                CreateConstantBufferView(const RenderResource<GpuBuffer> buffer, const size_t offset, const size_t sizeInBytes);
        RenderResource<GpuView>                CreateShaderResourceView(const RenderResource<GpuBuffer> buffer);
        RenderResource<GpuView>                CreateUnorderedAccessView(const RenderResource<GpuBuffer> buffer);
        RenderResource<GpuView>                CreateShaderResourceView(RenderResource<GpuTexture> texture, const GpuTextureSrvDesc& srvDesc, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
        RenderResource<GpuView>                CreateUnorderedAccessView(RenderResource<GpuTexture> texture, const GpuTextureUavDesc& uavDesc, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
        RenderResource<GpuView>                CreateRenderTargetView(RenderResource<GpuTexture> texture, const GpuTextureRtvDesc& rtvDesc, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
        RenderResource<GpuView>                CreateDepthStencilView(RenderResource<GpuTexture> texture, const GpuTextureDsvDesc& dsvDesc, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
        RenderResource<GpuView>                CreateSamplerView(const D3D12_SAMPLER_DESC& desc);
        RenderResource<GpuView>                CreateGpuView(const EGpuViewType type);

        void DestroyBuffer(const RenderResource<GpuBuffer> buffer);
        void DestroyTexture(const RenderResource<GpuTexture> texture);
        void DestroyPipelineState(const RenderResource<PipelineState> state);
        void DestroyGpuView(const RenderResource<GpuView> view);

        [[nodiscard]] GpuBuffer*                    Lookup(const RenderResource<GpuBuffer> handle);
        [[nodiscard]] const GpuBuffer*              Lookup(const RenderResource<GpuBuffer> handle) const;
        [[nodiscard]] GpuTexture*                   Lookup(const RenderResource<GpuTexture> handle);
        [[nodiscard]] const GpuTexture*             Lookup(const RenderResource<GpuTexture> handle) const;
        [[nodiscard]] PipelineState*                Lookup(const RenderResource<PipelineState> handle);
        [[nodiscard]] const PipelineState*          Lookup(const RenderResource<PipelineState> handle) const;
        [[nodiscard]] GpuView*                      Lookup(const RenderResource<GpuView> handle);
        [[nodiscard]] const GpuView*                Lookup(const RenderResource<GpuView> handle) const;

        void FlushQueues();
        void PreRender(const LocalFrameIndex localFrameIdx);
        void PostRender(const LocalFrameIndex localFrameIdx);

    private:
        GpuDevice gpuDevice;

        LocalFrameIndex lastLocalFrameIdx{ };

        CommandQueue       mainGfxQueue;
        CommandContextPool mainGfxCmdCtxPool;
        CommandQueue       asyncComputeQueue;
        CommandContextPool asyncComputeCmdCtxPool;
        CommandQueue       asyncCopyQueue;
        CommandContextPool asyncCopyCmdCtxPool;

        ResourceManagePackage<GpuBuffer>     bufferPackage;
        ResourceManagePackage<GpuTexture>    texturePackage;
        ResourceManagePackage<PipelineState> pipelineStatePackage;
        ResourceManagePackage<GpuView>       gpuViewPackage;

        GpuViewManager gpuViewManager;
        GpuUploader    gpuUploader;

        Ptr<Swapchain> swapchain;
    };
} // namespace ig
