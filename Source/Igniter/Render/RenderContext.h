#pragma once
#include <Igniter.h>
#include <Core/Handle.h>
#include <Core/HandleRegistry.h>
#include <D3D12/RenderDevice.h>
#include <D3D12/CommandQueue.h>
#include <D3D12/GpuBuffer.h>
#include <D3D12/GpuBufferDesc.h>
#include <D3D12/GpuTexture.h>
#include <D3D12/GpuTextureDesc.h>
#include <D3D12/PipelineState.h>
#include <D3D12/PipelineStateDesc.h>
#include <Render/GpuViewManager.h>
#include <Render/CommandContextPool.h>
#include <Render/GpuUploader.h>

namespace ig
{
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
            mutable SharedMutex Mut;
            HandleRegistry<Ty> Registry;
            eastl::array<Mutex, NumFramesInFlight> PendingListMut;
            eastl::array<eastl::vector<Handle<Ty>>, NumFramesInFlight> PendingDestroyList;
        };

    public:
        RenderContext(const FrameManager& frameManager);
        ~RenderContext();

        RenderContext(const RenderContext&) = delete;
        RenderContext(RenderContext&&) noexcept = delete;

        RenderContext& operator=(const RenderContext&) = delete;
        RenderContext& operator=(RenderContext&&) noexcept = delete;

        RenderDevice& GetRenderDevice() { return renderDevice; }
        CommandQueue& GetMainGfxQueue() { return mainGfxQueue; }
        CommandQueue& GetAsyncComputeQueue() { return asyncComputeQueue; }
        CommandQueue& GetAsyncCopyQueue() { return asyncComputeQueue; }
        CommandContextPool& GetMainGfxCommandContextPool() { return mainGfxCmdCtxPool; }
        CommandContextPool& GetAsyncComputeCommandContextPool() { return mainGfxCmdCtxPool; }
        GpuUploader& GetGpuUploader() { return gpuUploader; }
        auto GetBindlessDescriptorHeaps()
        {
            return eastl::array<DescriptorHeap*, 2>{&gpuViewManager.GetCbvSrvUavDescHeap(), &gpuViewManager.GetSamplerDescHeap()};
        }

        Handle<GpuBuffer> CreateBuffer(const GpuBufferDesc& desc);
        Handle<GpuTexture> CreateTexture(const GpuTextureDesc& desc);
        Handle<GpuTexture> CreateTexture(GpuTexture&& gpuTexture);

        Handle<PipelineState> CreatePipelineState(const GraphicsPipelineStateDesc& desc);
        Handle<PipelineState> CreatePipelineState(const ComputePipelineStateDesc& desc);

        Handle<GpuView> CreateConstantBufferView(const Handle<GpuBuffer> buffer);
        Handle<GpuView> CreateConstantBufferView(const Handle<GpuBuffer> buffer, const size_t offset, const size_t sizeInBytes);
        Handle<GpuView> CreateShaderResourceView(const Handle<GpuBuffer> buffer);
        Handle<GpuView> CreateUnorderedAccessView(const Handle<GpuBuffer> buffer);
        Handle<GpuView> CreateShaderResourceView(
            Handle<GpuTexture> texture, const GpuTextureSrvDesc& srvDesc, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
        Handle<GpuView> CreateUnorderedAccessView(
            Handle<GpuTexture> texture, const GpuTextureUavDesc& uavDesc, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
        Handle<GpuView> CreateRenderTargetView(
            Handle<GpuTexture> texture, const GpuTextureRtvDesc& rtvDesc, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
        Handle<GpuView> CreateDepthStencilView(
            Handle<GpuTexture> texture, const GpuTextureDsvDesc& dsvDesc, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
        Handle<GpuView> CreateSamplerView(const D3D12_SAMPLER_DESC& desc);

        void DestroyBuffer(const Handle<GpuBuffer> buffer);
        void DestroyTexture(const Handle<GpuTexture> texture);
        void DestroyPipelineState(const Handle<PipelineState> state);
        void DestroyGpuView(const Handle<GpuView> view);

        GpuBuffer* Lookup(const Handle<GpuBuffer> handle);
        const GpuBuffer* Lookup(const Handle<GpuBuffer> handle) const;
        GpuTexture* Lookup(const Handle<GpuTexture> handle);
        const GpuTexture* Lookup(const Handle<GpuTexture> handle) const;
        PipelineState* Lookup(const Handle<PipelineState> handle);
        const PipelineState* Lookup(const Handle<PipelineState> handle) const;
        GpuView* Lookup(const Handle<GpuView> handle);
        const GpuView* Lookup(const Handle<GpuView> handle) const;

        void FlushQueues();
        void BeginFrame(const uint8_t localFrameIdx);

    private:
        const FrameManager& frameManager;

        RenderDevice renderDevice;

        CommandQueue mainGfxQueue;
        CommandContextPool mainGfxCmdCtxPool;
        CommandQueue asyncComputeQueue;
        CommandContextPool asyncComputeCmdCtxPool;
        CommandQueue asyncCopyQueue;

        ResourceManagePackage<GpuBuffer> bufferPackage;
        ResourceManagePackage<GpuTexture> texturePackage;
        ResourceManagePackage<PipelineState> pipelineStatePackage;
        ResourceManagePackage<GpuView> gpuViewPackage;

        GpuViewManager gpuViewManager;
        GpuUploader gpuUploader;
    };
}    // namespace ig
