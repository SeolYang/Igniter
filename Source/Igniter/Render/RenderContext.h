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
    class UnifiedMeshStorage;

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
        [[nodiscard]] CommandQueue& GetFrameCriticalAsyncCopyQueue() { return frameCriticalAsyncCopyQueue; }
        [[nodiscard]] CommandQueue& GetNonFrameCriticalAsyncCopyQueue() { return nonFrameCriticalAsyncCopyQueue; }
        [[nodiscard]] CommandListPool& GetMainGfxCommandListPool() { return mainGfxCmdListPool; }
        [[nodiscard]] CommandListPool& GetAsyncComputeCommandListPool() { return asyncComputeCmdListPool; }
        [[nodiscard]] CommandListPool& GetAsyncCopyCommandListPool() { return asyncCopyCmdListPool; }
        [[nodiscard]] GpuUploader& GetFrameCriticalGpuUploader() { return frameCriticalGpuUploader; }
        [[nodiscard]] GpuUploader& GetNonFrameCriticalGpuUploader() { return nonFrameCriticalGpuUploader; }
        [[nodiscard]] UnifiedMeshStorage& GetUnifiedMeshStorage() { return *unifiedMeshStorage; }
        [[nodiscard]] Swapchain& GetSwapchain() { return *swapchain; }
        [[nodiscard]] const Swapchain& GetSwapchain() const { return *swapchain; }
        [[nodiscard]] auto& GetCbvSrvUavDescriptorHeap() { return gpuViewManager.GetCbvSrvUavDescHeap(); }
        [[nodiscard]] auto GetBindlessDescriptorHeaps() { return Array<DescriptorHeap*, 2>{&gpuViewManager.GetCbvSrvUavDescHeap(), &gpuViewManager.GetSamplerDescHeap()}; }

        Handle<GpuBuffer> CreateBuffer(const GpuBufferDesc& desc);
        Handle<GpuTexture> CreateTexture(const GpuTextureDesc& desc);
        Handle<GpuTexture> CreateTexture(GpuTexture gpuTexture);
        Handle<PipelineState> CreatePipelineState(const GraphicsPipelineStateDesc& desc);
        Handle<PipelineState> CreatePipelineState(const ComputePipelineStateDesc& desc);
        Handle<PipelineState> CreatePipelineState(const MeshShaderPipelineStateDesc& desc);
        Handle<GpuView> CreateConstantBufferView(const Handle<GpuBuffer> buffer);
        Handle<GpuView> CreateConstantBufferView(const Handle<GpuBuffer> buffer, const Size offset, const Size sizeInBytes);
        Handle<GpuView> CreateShaderResourceView(const Handle<GpuBuffer> buffer);
        Handle<GpuView> CreateShaderResourceView(const Handle<GpuBuffer> buffer, const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc);
        Handle<GpuView> CreateUnorderedAccessView(const Handle<GpuBuffer> buffer);
        Handle<GpuView> CreateUnorderedAccessView(const Handle<GpuBuffer> buffer, const D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc);
        Handle<GpuView> CreateShaderResourceView(Handle<GpuTexture> texture, const GpuTextureSrvVariant& srvVariant, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
        Handle<GpuView> CreateUnorderedAccessView(Handle<GpuTexture> texture, const GpuTextureUavVariant& uavVariant, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
        Handle<GpuView> CreateRenderTargetView(Handle<GpuTexture> texture, const GpuTextureRtvVariant& rtvVariant, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
        Handle<GpuView> CreateDepthStencilView(Handle<GpuTexture> texture, const GpuTextureDsvVariant& dsvVariant, const DXGI_FORMAT desireViewFormat = DXGI_FORMAT_UNKNOWN);
        Handle<GpuView> CreateSamplerView(const D3D12_SAMPLER_DESC& desc);
        Handle<GpuView> CreateGpuView(const EGpuViewType type);

        void DestroyBuffer(const Handle<GpuBuffer> buffer);
        void DestroyTexture(const Handle<GpuTexture> texture);
        void DestroyPipelineState(const Handle<PipelineState> state);
        void DestroyGpuView(const Handle<GpuView> view);

        [[nodiscard]] GpuBuffer* Lookup(const Handle<GpuBuffer> handle);
        [[nodiscard]] const GpuBuffer* Lookup(const Handle<GpuBuffer> handle) const;
        [[nodiscard]] GpuTexture* Lookup(const Handle<GpuTexture> handle);
        [[nodiscard]] const GpuTexture* Lookup(const Handle<GpuTexture> handle) const;
        [[nodiscard]] PipelineState* Lookup(const Handle<PipelineState> handle);
        [[nodiscard]] const PipelineState* Lookup(const Handle<PipelineState> handle) const;
        [[nodiscard]] GpuView* Lookup(const Handle<GpuView> handle);
        [[nodiscard]] const GpuView* Lookup(const Handle<GpuView> handle) const;

        void FlushQueues();
        void PreRender(const LocalFrameIndex localFrameIdx, GpuSyncPoint& prevFrameLastSyncPoint);
        void PostRender(const LocalFrameIndex localFrameIdx);

    private:
        Handle<GpuView> CreateShaderResourceViewUnsafe(GpuBuffer& buffer, const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc);
        Handle<GpuView> CreateUnorderedAccessViewUnsafe(GpuBuffer& buffer, const D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc);

    private:
        GpuDevice gpuDevice;
        LocalFrameIndex currentLocalFrameIdx{};

        CommandQueue mainGfxQueue;
        CommandListPool mainGfxCmdListPool;
        CommandQueue asyncComputeQueue;
        CommandListPool asyncComputeCmdListPool;
        CommandQueue frameCriticalAsyncCopyQueue;
        CommandQueue nonFrameCriticalAsyncCopyQueue;
        CommandListPool asyncCopyCmdListPool;

        DeferredResourceManagePackage<GpuBuffer> bufferPackage;
        DeferredResourceManagePackage<GpuTexture> texturePackage;
        DeferredResourceManagePackage<PipelineState> pipelineStatePackage;
        DeferredResourceManagePackage<GpuView> gpuViewPackage;

        GpuViewManager gpuViewManager;
        constexpr static Size kFrameCriticalGpuUploaderInitSize = 64Ui64 * 1024Ui64; /* ~64KB */
        GpuUploader frameCriticalGpuUploader;
        constexpr static Size kNonFrameCriticalGpuUploaderInitSize = 64Ui64 * 1024Ui64 * 1024Ui64; /* ~64MB */
        GpuUploader nonFrameCriticalGpuUploader;
        Ptr<UnifiedMeshStorage> unifiedMeshStorage;

        Ptr<Swapchain> swapchain;
    };
} // namespace ig
