#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/D3D12/GpuDevice.h"
#include "Igniter/D3D12/CommandQueue.h"
#include "Igniter/D3D12/GpuBuffer.h"
#include "Igniter/D3D12/GpuTexture.h"
#include "Igniter/D3D12/GpuFence.h"
#include "Igniter/D3D12/PipelineState.h"
#include "Igniter/Render/Common.h"
#include "Igniter/Render/CommandListPool.h"
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
    public:
        explicit RenderContext(const Window& window);
        ~RenderContext();

        RenderContext(const RenderContext&) = delete;
        RenderContext(RenderContext&&) noexcept = delete;

        RenderContext& operator=(const RenderContext&) = delete;
        RenderContext& operator=(RenderContext&&) noexcept = delete;

        [[nodiscard]] GpuDevice& GetGpuDevice() { return gpuDevice; }
        [[nodiscard]] CommandQueue& GetMainGfxQueue() { return mainGfxQueue; }
        [[nodiscard]] CommandQueue& GetAsyncComputeQueue() { return asyncComputeQueue; }
        [[nodiscard]] CommandQueue& GetAsyncCopyQueue() { return asyncCopyQueue; }
        [[nodiscard]] CommandListPool& GetMainGfxCommandListPool() { return mainGfxCmdListPool; }
        [[nodiscard]] CommandListPool& GetAsyncComputeCommandListPool() { return asyncComputeCmdListPool; }
        [[nodiscard]] CommandListPool& GetAsyncCopyCommandListPool() { return asyncCopyCmdListPool; }
        [[nodiscard]] GpuFence& GetMainGfxFence() { return *mainGfxFence.Resources[currentLocalFrameIdx]; }
        [[nodiscard]] GpuFence& GetAsyncComputeFence() { return *asyncComputeFence.Resources[currentLocalFrameIdx]; }
        [[nodiscard]] GpuFence& GetAsyncCopyFence() { return *asyncCopyFence.Resources[currentLocalFrameIdx]; }
        [[nodiscard]] GpuUploader& GetGpuUploader() { return gpuUploader; }
        [[nodiscard]] Swapchain& GetSwapchain() { return *swapchain; }
        [[nodiscard]] const Swapchain& GetSwapchain() const { return *swapchain; }
        [[nodiscard]] auto& GetCbvSrvUavDescriptorHeap() { return gpuViewManager.GetCbvSrvUavDescHeap(); }
        [[nodiscard]] auto GetBindlessDescriptorHeaps() { return Array<DescriptorHeap*, 2>{&gpuViewManager.GetCbvSrvUavDescHeap(), &gpuViewManager.GetSamplerDescHeap()}; }

        RenderHandle<GpuBuffer> CreateBuffer(const GpuBufferDesc& desc);
        RenderHandle<GpuTexture> CreateTexture(const GpuTextureDesc& desc);
        RenderHandle<GpuTexture> CreateTexture(GpuTexture gpuTexture);
        RenderHandle<PipelineState> CreatePipelineState(const GraphicsPipelineStateDesc& desc);
        RenderHandle<PipelineState> CreatePipelineState(const ComputePipelineStateDesc& desc);
        RenderHandle<GpuView> CreateConstantBufferView(const RenderHandle<GpuBuffer> buffer);
        RenderHandle<GpuView> CreateConstantBufferView(const RenderHandle<GpuBuffer> buffer, const size_t offset, const size_t sizeInBytes);
        RenderHandle<GpuView> CreateShaderResourceView(const RenderHandle<GpuBuffer> buffer);
        RenderHandle<GpuView> CreateUnorderedAccessView(const RenderHandle<GpuBuffer> buffer);
        RenderHandle<GpuView> CreateShaderResourceView(RenderHandle<GpuTexture> texture, const GpuTextureSrvDesc& srvDesc, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
        RenderHandle<GpuView> CreateUnorderedAccessView(RenderHandle<GpuTexture> texture, const GpuTextureUavDesc& uavDesc, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
        RenderHandle<GpuView> CreateRenderTargetView(RenderHandle<GpuTexture> texture, const GpuTextureRtvDesc& rtvDesc, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
        RenderHandle<GpuView> CreateDepthStencilView(RenderHandle<GpuTexture> texture, const GpuTextureDsvDesc& dsvDesc, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
        RenderHandle<GpuView> CreateSamplerView(const D3D12_SAMPLER_DESC& desc);
        RenderHandle<GpuView> CreateGpuView(const EGpuViewType type);

        void DestroyBuffer(const RenderHandle<GpuBuffer> buffer);
        void DestroyTexture(const RenderHandle<GpuTexture> texture);
        void DestroyPipelineState(const RenderHandle<PipelineState> state);
        void DestroyGpuView(const RenderHandle<GpuView> view);

        [[nodiscard]] GpuBuffer* Lookup(const RenderHandle<GpuBuffer> handle);
        [[nodiscard]] const GpuBuffer* Lookup(const RenderHandle<GpuBuffer> handle) const;
        [[nodiscard]] GpuTexture* Lookup(const RenderHandle<GpuTexture> handle);
        [[nodiscard]] const GpuTexture* Lookup(const RenderHandle<GpuTexture> handle) const;
        [[nodiscard]] PipelineState* Lookup(const RenderHandle<PipelineState> handle);
        [[nodiscard]] const PipelineState* Lookup(const RenderHandle<PipelineState> handle) const;
        [[nodiscard]] GpuView* Lookup(const RenderHandle<GpuView> handle);
        [[nodiscard]] const GpuView* Lookup(const RenderHandle<GpuView> handle) const;

        void FlushQueues();
        void PreRender(const LocalFrameIndex localFrameIdx);
        void PostRender(const LocalFrameIndex localFrameIdx);

    private:
        GpuDevice gpuDevice;

        LocalFrameIndex currentLocalFrameIdx{};

        CommandQueue mainGfxQueue;
        CommandListPool mainGfxCmdListPool;
        InFlightFramesResource<Ptr<GpuFence>> mainGfxFence;
        CommandQueue asyncComputeQueue;
        CommandListPool asyncComputeCmdListPool;
        InFlightFramesResource<Ptr<GpuFence>> asyncComputeFence;
        CommandQueue asyncCopyQueue;
        CommandListPool asyncCopyCmdListPool;
        InFlightFramesResource<Ptr<GpuFence>> asyncCopyFence;

        DeferredResourceManagePackage<GpuBuffer, RenderContext> bufferPackage;
        DeferredResourceManagePackage<GpuTexture, RenderContext> texturePackage;
        DeferredResourceManagePackage<PipelineState, RenderContext> pipelineStatePackage;
        DeferredResourceManagePackage<GpuView, RenderContext> gpuViewPackage;

        GpuViewManager gpuViewManager;
        GpuUploader gpuUploader;

        Ptr<Swapchain> swapchain;
    };
} // namespace ig
