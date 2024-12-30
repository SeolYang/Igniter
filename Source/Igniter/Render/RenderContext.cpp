#include "Igniter/Igniter.h"
#include "Igniter/Core/Window.h"
#include "Igniter/Render/RenderContext.h"

namespace ig
{
    RenderContext::RenderContext(const Window& window) :
        mainGfxQueue(*gpuDevice.CreateCommandQueue("MainGfx", EQueueType::Graphics)),
        mainGfxCmdCtxPool(gpuDevice, EQueueType::Graphics),
        asyncComputeQueue(*gpuDevice.CreateCommandQueue("AsyncCompute", EQueueType::Compute)),
        asyncComputeCmdCtxPool(gpuDevice, EQueueType::Compute),
        asyncCopyQueue(*gpuDevice.CreateCommandQueue("AsyncCopy", EQueueType::Copy)),
        asyncCopyCmdCtxPool(gpuDevice, EQueueType::Copy),
        gpuViewManager(gpuDevice),
        gpuUploader(gpuDevice),
        swapchain(MakePtr<Swapchain>(window, *this, NumFramesInFlight))
    {
        for (LocalFrameIndex localFrameIdx = 0; localFrameIdx < NumFramesInFlight; ++localFrameIdx)
        {
            mainGfxFence.Resources[localFrameIdx]      = MakePtr<GpuFence>(*gpuDevice.CreateFence(std::format("MainGfxFence{}", localFrameIdx)));
            asyncComputeFence.Resources[localFrameIdx] = MakePtr<GpuFence>(*gpuDevice.CreateFence(std::format("AsyncComputeFence{}", localFrameIdx)));
            asyncCopyFence.Resources[localFrameIdx]    = MakePtr<GpuFence>(*gpuDevice.CreateFence(std::format("AsyncCopyFence{}", localFrameIdx)));
        }
    }

    RenderContext::~RenderContext()
    {
        FlushQueues();

        swapchain.reset();
        /* Flush pending destroy list! */
        for (const LocalFrameIndex localFrameIdx : views::iota(0Ui8, NumFramesInFlight))
        {
            PreRender(localFrameIdx);
        }
    }

    RenderHandle<GpuBuffer> RenderContext::CreateBuffer(const GpuBufferDesc& desc)
    {
        Option<GpuBuffer> newBuffer = gpuDevice.CreateBuffer(desc);
        if (!newBuffer)
        {
            return RenderHandle<GpuBuffer>{};
        }

        ReadWriteLock bufferStorageLock{bufferPackage.StorageMutex};
        return bufferPackage.Storage.Create(std::move(newBuffer).value());
    }

    RenderHandle<GpuTexture> RenderContext::CreateTexture(const GpuTextureDesc& desc)
    {
        Option<GpuTexture> newTexture = gpuDevice.CreateTexture(desc);
        if (!newTexture)
        {
            return RenderHandle<GpuTexture>{};
        }

        ReadWriteLock textureStorageLock{texturePackage.StorageMutex};
        return texturePackage.Storage.Create(std::move(newTexture).value());
    }

    RenderHandle<GpuTexture> RenderContext::CreateTexture(GpuTexture gpuTexture)
    {
        ReadWriteLock textureStorageLock{texturePackage.StorageMutex};
        return texturePackage.Storage.Create(std::move(gpuTexture));
    }

    RenderHandle<PipelineState> RenderContext::CreatePipelineState(const GraphicsPipelineStateDesc& desc)
    {
        Option<PipelineState> newPipelineState = gpuDevice.CreateGraphicsPipelineState(desc);
        if (!newPipelineState)
        {
            return RenderHandle<PipelineState>{};
        }

        ReadWriteLock pipelineStateStorageLock{pipelineStatePackage.StorageMutex};
        return pipelineStatePackage.Storage.Create(std::move(newPipelineState).value());
    }

    RenderHandle<PipelineState> RenderContext::CreatePipelineState(const ComputePipelineStateDesc& desc)
    {
        Option<PipelineState> newPipelineState = gpuDevice.CreateComputePipelineState(desc);
        if (!newPipelineState)
        {
            return RenderHandle<PipelineState>{};
        }

        ReadWriteLock pipelineStateStorageLock{pipelineStatePackage.StorageMutex};
        return pipelineStatePackage.Storage.Create(std::move(newPipelineState).value());
    }

    RenderHandle<GpuView> RenderContext::CreateConstantBufferView(const RenderHandle<GpuBuffer> buffer)
    {
        ScopedLock       StorageLock{bufferPackage.StorageMutex, gpuViewPackage.StorageMutex};
        GpuBuffer* const bufferPtr = bufferPackage.Storage.Lookup(buffer);
        if (bufferPtr == nullptr)
        {
            return RenderHandle<GpuView>{};
        }

        const GpuView newView = gpuViewManager.RequestConstantBufferView(*bufferPtr);
        if (!newView)
        {
            return RenderHandle<GpuView>{};
        }

        IG_CHECK(newView.Type == EGpuViewType::ConstantBufferView);
        return gpuViewPackage.Storage.Create(newView);
    }

    RenderHandle<GpuView> RenderContext::CreateConstantBufferView(const RenderHandle<GpuBuffer> buffer, const size_t offset, const size_t sizeInBytes)
    {
        ScopedLock       StorageLock{bufferPackage.StorageMutex, gpuViewPackage.StorageMutex};
        GpuBuffer* const bufferPtr = bufferPackage.Storage.Lookup(buffer);
        if (bufferPtr == nullptr)
        {
            return RenderHandle<GpuView>{};
        }

        const GpuView newView = gpuViewManager.RequestConstantBufferView(*bufferPtr, offset, sizeInBytes);
        if (!newView)
        {
            return RenderHandle<GpuView>{};
        }

        IG_CHECK(newView.Type == EGpuViewType::ConstantBufferView);
        return gpuViewPackage.Storage.Create(newView);
    }

    RenderHandle<GpuView> RenderContext::CreateShaderResourceView(const RenderHandle<GpuBuffer> buffer)
    {
        ScopedLock       StorageLock{bufferPackage.StorageMutex, gpuViewPackage.StorageMutex};
        GpuBuffer* const bufferPtr = bufferPackage.Storage.Lookup(buffer);
        if (bufferPtr == nullptr)
        {
            return RenderHandle<GpuView>{};
        }

        const GpuView newView = gpuViewManager.RequestShaderResourceView(*bufferPtr);
        if (!newView)
        {
            return RenderHandle<GpuView>{};
        }

        IG_CHECK(newView.Type == EGpuViewType::ShaderResourceView);
        return gpuViewPackage.Storage.Create(newView);
    }

    RenderHandle<GpuView> RenderContext::CreateUnorderedAccessView(const RenderHandle<GpuBuffer> buffer)
    {
        ScopedLock       StorageLock{bufferPackage.StorageMutex, gpuViewPackage.StorageMutex};
        GpuBuffer* const bufferPtr = bufferPackage.Storage.Lookup(buffer);
        if (bufferPtr == nullptr)
        {
            return RenderHandle<GpuView>{};
        }

        const GpuView newView = gpuViewManager.RequestUnorderedAccessView(*bufferPtr);
        if (!newView)
        {
            return RenderHandle<GpuView>{};
        }

        IG_CHECK(newView.Type == EGpuViewType::UnorderedAccessView);
        return gpuViewPackage.Storage.Create(newView);
    }

    RenderHandle<GpuView> RenderContext::CreateShaderResourceView(RenderHandle<GpuTexture> texture, const GpuTextureSrvDesc& srvDesc, const DXGI_FORMAT desireViewFormat /*= DXGI_FORMAT_UNKNOWN*/)
    {
        ScopedLock        StorageLock{texturePackage.StorageMutex, gpuViewPackage.StorageMutex};
        GpuTexture* const texturePtr = texturePackage.Storage.Lookup(texture);
        if (texturePtr == nullptr)
        {
            return RenderHandle<GpuView>{};
        }

        const GpuView newView = gpuViewManager.RequestShaderResourceView(*texturePtr, srvDesc, desireViewFormat);
        if (!newView)
        {
            return RenderHandle<GpuView>{};
        }

        IG_CHECK(newView.Type == EGpuViewType::ShaderResourceView);
        return gpuViewPackage.Storage.Create(newView);
    }

    RenderHandle<GpuView> RenderContext::CreateUnorderedAccessView(RenderHandle<GpuTexture> texture, const GpuTextureUavDesc& uavDesc, const DXGI_FORMAT desireViewFormat /*= DXGI_FORMAT_UNKNOWN*/)
    {
        ScopedLock        StorageLock{texturePackage.StorageMutex, gpuViewPackage.StorageMutex};
        GpuTexture* const texturePtr = texturePackage.Storage.Lookup(texture);
        if (texturePtr == nullptr)
        {
            return RenderHandle<GpuView>{};
        }

        const GpuView newView = gpuViewManager.RequestUnorderedAccessView(*texturePtr, uavDesc, desireViewFormat);
        if (!newView)
        {
            return RenderHandle<GpuView>{};
        }

        IG_CHECK(newView.Type == EGpuViewType::UnorderedAccessView);
        return gpuViewPackage.Storage.Create(newView);
    }

    RenderHandle<GpuView> RenderContext::CreateRenderTargetView(RenderHandle<GpuTexture> texture, const GpuTextureRtvDesc& rtvDesc, const DXGI_FORMAT desireViewFormat /*= DXGI_FORMAT_UNKNOWN*/)
    {
        ScopedLock        StorageLock{texturePackage.StorageMutex, gpuViewPackage.StorageMutex};
        GpuTexture* const texturePtr = texturePackage.Storage.Lookup(texture);
        if (texturePtr == nullptr)
        {
            return RenderHandle<GpuView>{};
        }

        const GpuView newView = gpuViewManager.RequestRenderTargetView(*texturePtr, rtvDesc, desireViewFormat);
        if (!newView)
        {
            return RenderHandle<GpuView>{};
        }

        IG_CHECK(newView.Type == EGpuViewType::RenderTargetView);
        return gpuViewPackage.Storage.Create(newView);
    }

    RenderHandle<GpuView> RenderContext::CreateDepthStencilView(RenderHandle<GpuTexture> texture, const GpuTextureDsvDesc& dsvDesc, const DXGI_FORMAT desireViewFormat /*= DXGI_FORMAT_UNKNOWN*/)
    {
        ScopedLock        StorageLock{texturePackage.StorageMutex, gpuViewPackage.StorageMutex};
        GpuTexture* const texturePtr = texturePackage.Storage.Lookup(texture);
        if (texturePtr == nullptr)
        {
            return RenderHandle<GpuView>{};
        }

        const GpuView newView = gpuViewManager.RequestDepthStencilView(*texturePtr, dsvDesc, desireViewFormat);
        if (!newView)
        {
            return RenderHandle<GpuView>{};
        }

        IG_CHECK(newView.Type == EGpuViewType::DepthStencilView);
        return gpuViewPackage.Storage.Create(newView);
    }

    RenderHandle<GpuView> RenderContext::CreateSamplerView(const D3D12_SAMPLER_DESC& desc)
    {
        ReadWriteLock gpuViewStorageLock{gpuViewPackage.StorageMutex};
        const GpuView newView = gpuViewManager.RequestSampler(desc);
        if (!newView)
        {
            return RenderHandle<GpuView>{};
        }

        IG_CHECK(newView.Type == EGpuViewType::Sampler);
        return gpuViewPackage.Storage.Create(newView);
    }

    RenderHandle<GpuView> RenderContext::CreateGpuView(const EGpuViewType type)
    {
        IG_CHECK(type != EGpuViewType::Unknown);
        ReadWriteLock gpuViewStorageLock{gpuViewPackage.StorageMutex};
        const GpuView newView = gpuViewManager.Allocate(type);
        if (!newView)
        {
            return RenderHandle<GpuView>{};
        }

        IG_CHECK(newView.Type != EGpuViewType::Unknown);
        return gpuViewPackage.Storage.Create(newView);
    }

    void RenderContext::DestroyBuffer(const RenderHandle<GpuBuffer> buffer)
    {
        if (buffer)
        {
            ScopedLock packageLock{bufferPackage.StorageMutex, bufferPackage.DeferredDestroyPendingListMutex.Resources[currentLocalFrameIdx]};
            bufferPackage.DeferredDestroyPendingList.Resources[currentLocalFrameIdx].emplace_back(buffer);
            bufferPackage.Storage.MarkAsDestroy(buffer);
        }
    }

    void RenderContext::DestroyTexture(const RenderHandle<GpuTexture> texture)
    {
        if (texture)
        {
            ScopedLock packageLock{texturePackage.StorageMutex, texturePackage.DeferredDestroyPendingListMutex.Resources[currentLocalFrameIdx]};
            texturePackage.DeferredDestroyPendingList.Resources[currentLocalFrameIdx].emplace_back(texture);
            texturePackage.Storage.MarkAsDestroy(texture);
        }
    }

    void RenderContext::DestroyPipelineState(const RenderHandle<PipelineState> state)
    {
        if (state)
        {
            ScopedLock packageLock{pipelineStatePackage.StorageMutex, pipelineStatePackage.DeferredDestroyPendingListMutex.Resources[currentLocalFrameIdx]};
            pipelineStatePackage.DeferredDestroyPendingList.Resources[currentLocalFrameIdx].emplace_back(state);
            pipelineStatePackage.Storage.MarkAsDestroy(state);
        }
    }

    void RenderContext::DestroyGpuView(const RenderHandle<GpuView> view)
    {
        if (view)
        {
            ScopedLock packageLock{gpuViewPackage.StorageMutex, gpuViewPackage.DeferredDestroyPendingListMutex.Resources[currentLocalFrameIdx]};
            gpuViewPackage.DeferredDestroyPendingList.Resources[currentLocalFrameIdx].emplace_back(view);
            gpuViewPackage.Storage.MarkAsDestroy(view);
        }
    }

    GpuBuffer* RenderContext::Lookup(const RenderHandle<GpuBuffer> handle)
    {
        ReadOnlyLock bufferStorageLock{bufferPackage.StorageMutex};
        return bufferPackage.Storage.Lookup(handle);
    }

    const GpuBuffer* RenderContext::Lookup(const RenderHandle<GpuBuffer> handle) const
    {
        ReadOnlyLock bufferStorageLock{bufferPackage.StorageMutex};
        return bufferPackage.Storage.Lookup(handle);
    }

    GpuTexture* RenderContext::Lookup(const RenderHandle<GpuTexture> handle)
    {
        ReadOnlyLock textureStorageLock{texturePackage.StorageMutex};
        return texturePackage.Storage.Lookup(handle);
    }

    const GpuTexture* RenderContext::Lookup(const RenderHandle<GpuTexture> handle) const
    {
        ReadOnlyLock textureStorageLock{texturePackage.StorageMutex};
        return texturePackage.Storage.Lookup(handle);
    }

    PipelineState* RenderContext::Lookup(const RenderHandle<PipelineState> handle)
    {
        ReadOnlyLock pipelineStateStorageLock{pipelineStatePackage.StorageMutex};
        return pipelineStatePackage.Storage.Lookup(handle);
    }

    const PipelineState* RenderContext::Lookup(const RenderHandle<PipelineState> handle) const
    {
        ReadOnlyLock pipelineStateStorageLock{pipelineStatePackage.StorageMutex};
        return pipelineStatePackage.Storage.Lookup(handle);
    }

    GpuView* RenderContext::Lookup(const RenderHandle<GpuView> handle)
    {
        ReadOnlyLock gpuViewStorageLock{gpuViewPackage.StorageMutex};
        return gpuViewPackage.Storage.Lookup(handle);
    }

    const GpuView* RenderContext::Lookup(const RenderHandle<GpuView> handle) const
    {
        ReadOnlyLock gpuViewStorageLock{gpuViewPackage.StorageMutex};
        return gpuViewPackage.Storage.Lookup(handle);
    }

    void RenderContext::FlushQueues()
    {
        for (LocalFrameIndex localFrameIdx = 0; localFrameIdx < NumFramesInFlight; ++localFrameIdx)
        {
            mainGfxQueue.MakeSyncPointWithSignal(*mainGfxFence.Resources[localFrameIdx]).WaitOnCpu();
            asyncComputeQueue.MakeSyncPointWithSignal(*asyncComputeFence.Resources[localFrameIdx]).WaitOnCpu();
            asyncCopyQueue.MakeSyncPointWithSignal(*asyncCopyFence.Resources[localFrameIdx]).WaitOnCpu();
        }
    }

    void RenderContext::PreRender(const LocalFrameIndex localFrameIdx)
    {
        this->currentLocalFrameIdx = localFrameIdx;
        mainGfxCmdCtxPool.PreRender(localFrameIdx);
        asyncComputeCmdCtxPool.PreRender(localFrameIdx);
        asyncCopyCmdCtxPool.PreRender(localFrameIdx);
        gpuUploader.PreRender(localFrameIdx);

        /* Flush Buffer Package [local frame] pending destroy */
        {
            ScopedLock bufferPackageLock{bufferPackage.StorageMutex, bufferPackage.DeferredDestroyPendingListMutex.Resources[localFrameIdx]};
            for (const auto handle : bufferPackage.DeferredDestroyPendingList.Resources[localFrameIdx])
            {
                bufferPackage.Storage.Destroy(handle);
            }
            bufferPackage.DeferredDestroyPendingList.Resources[localFrameIdx].clear();
        }

        /* Flush Texture Package [local frame] pending destroy */
        {
            ScopedLock texturePackageLock{texturePackage.StorageMutex, texturePackage.DeferredDestroyPendingListMutex.Resources[localFrameIdx]};
            for (const auto handle : texturePackage.DeferredDestroyPendingList.Resources[localFrameIdx])
            {
                texturePackage.Storage.Destroy(handle);
            }
            texturePackage.DeferredDestroyPendingList.Resources[localFrameIdx].clear();
        }

        /* Flush Pipeline State Package [local frame] pending destroy */
        {
            ScopedLock pipelineStatePackageLock{pipelineStatePackage.StorageMutex, pipelineStatePackage.DeferredDestroyPendingListMutex.Resources[localFrameIdx]};
            for (const auto handle : pipelineStatePackage.DeferredDestroyPendingList.Resources[localFrameIdx])
            {
                pipelineStatePackage.Storage.Destroy(handle);
            }
            pipelineStatePackage.DeferredDestroyPendingList.Resources[localFrameIdx].clear();
        }

        /* Flush Gpu View Package [local frame] pending destroy */
        {
            ScopedLock gpuViewPackageLock{gpuViewPackage.StorageMutex, gpuViewPackage.DeferredDestroyPendingListMutex.Resources[localFrameIdx]};
            for (const auto handle : gpuViewPackage.DeferredDestroyPendingList.Resources[localFrameIdx])
            {
                gpuViewManager.Deallocate(*gpuViewPackage.Storage.LookupUnsafe(handle));
                gpuViewPackage.Storage.Destroy(handle);
            }
            gpuViewPackage.DeferredDestroyPendingList.Resources[localFrameIdx].clear();
        }
    }

    void RenderContext::PostRender([[maybe_unused]] const LocalFrameIndex localFrameIdx)
    {
        swapchain->Present();
    }
} // namespace ig
