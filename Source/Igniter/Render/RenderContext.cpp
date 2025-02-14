#include "Igniter/Igniter.h"
#include "Igniter/Core/Window.h"
#include "Igniter/Render/UnifiedMeshStorage.h"
#include "Igniter/Render/RenderContext.h"

namespace ig
{
    RenderContext::RenderContext(const Window& window)
        : mainGfxQueue(*gpuDevice.CreateCommandQueue("MainGfx", EQueueType::Graphics))
        , mainGfxCmdListPool(gpuDevice, EQueueType::Graphics)
        , asyncComputeQueue(*gpuDevice.CreateCommandQueue("AsyncCompute", EQueueType::Compute))
        , asyncComputeCmdListPool(gpuDevice, EQueueType::Compute)
        , frameCriticalAsyncCopyQueue(*gpuDevice.CreateCommandQueue("FrameCriticalAsyncCopyQueue", EQueueType::Copy))
        , nonFrameCriticalAsyncCopyQueue(*gpuDevice.CreateCommandQueue("NonFrameCriticalAsyncCopyQueue", EQueueType::Copy))
        , asyncCopyCmdListPool(gpuDevice, EQueueType::Copy)
        , gpuViewManager(gpuDevice)
        , frameCriticalGpuUploader("FrameCriticalUploader"_fs, gpuDevice, frameCriticalAsyncCopyQueue, kFrameCriticalGpuUploaderInitSize)
        , nonFrameCriticalGpuUploader("NonFrameCriticalUploader"_fs, gpuDevice, nonFrameCriticalAsyncCopyQueue, kNonFrameCriticalGpuUploaderInitSize)
        , unifiedMeshStorage(MakePtr<UnifiedMeshStorage>(*this))
        , swapchain(MakePtr<Swapchain>(window, *this))
    {}

    RenderContext::~RenderContext()
    {
        FlushQueues();

        for (const LocalFrameIndex localFrameIdx : views::iota(0Ui8, NumFramesInFlight))
        {
            unifiedMeshStorage->PreRender(localFrameIdx);
        }
        unifiedMeshStorage.reset();
        swapchain.reset();

        /* Flush pending destroy list! */
        GpuSyncPoint invalidSyncPoint{};
        for (const LocalFrameIndex localFrameIdx : views::iota(0Ui8, NumFramesInFlight))
        {
            PreRender(localFrameIdx, invalidSyncPoint);
        }
    }

    Handle<GpuBuffer> RenderContext::CreateBuffer(const GpuBufferDesc& desc)
    {
        Option<GpuBuffer> newBuffer = gpuDevice.CreateBuffer(desc);
        if (!newBuffer)
        {
            return Handle<GpuBuffer>{};
        }

        ReadWriteLock bufferStorageLock{bufferPackage.StorageMutex};
        return bufferPackage.Storage.Create(std::move(newBuffer).value());
    }

    Handle<GpuTexture> RenderContext::CreateTexture(const GpuTextureDesc& desc)
    {
        Option<GpuTexture> newTexture = gpuDevice.CreateTexture(desc);
        if (!newTexture)
        {
            return Handle<GpuTexture>{};
        }

        ReadWriteLock textureStorageLock{texturePackage.StorageMutex};
        return texturePackage.Storage.Create(std::move(newTexture).value());
    }

    Handle<GpuTexture> RenderContext::CreateTexture(GpuTexture gpuTexture)
    {
        ReadWriteLock textureStorageLock{texturePackage.StorageMutex};
        return texturePackage.Storage.Create(std::move(gpuTexture));
    }

    Handle<PipelineState> RenderContext::CreatePipelineState(const GraphicsPipelineStateDesc& desc)
    {
        Option<PipelineState> newPipelineState = gpuDevice.CreateGraphicsPipelineState(desc);
        if (!newPipelineState)
        {
            return Handle<PipelineState>{};
        }

        ReadWriteLock pipelineStateStorageLock{pipelineStatePackage.StorageMutex};
        return pipelineStatePackage.Storage.Create(std::move(newPipelineState).value());
    }

    Handle<PipelineState> RenderContext::CreatePipelineState(const ComputePipelineStateDesc& desc)
    {
        Option<PipelineState> newPipelineState = gpuDevice.CreateComputePipelineState(desc);
        if (!newPipelineState)
        {
            return Handle<PipelineState>{};
        }

        ReadWriteLock pipelineStateStorageLock{pipelineStatePackage.StorageMutex};
        return pipelineStatePackage.Storage.Create(std::move(newPipelineState).value());
    }

    Handle<PipelineState> RenderContext::CreatePipelineState(const MeshShaderPipelineStateDesc& desc)
    {
        Option<PipelineState> newPipelineState = gpuDevice.CreateMeshShaderPipelineState(desc);
        if (!newPipelineState)
        {
            return Handle<PipelineState>{};
        }

        ReadWriteLock pipelineStateStorageLock{pipelineStatePackage.StorageMutex};
        return pipelineStatePackage.Storage.Create(std::move(newPipelineState).value());
    }

    Handle<GpuView> RenderContext::CreateConstantBufferView(const Handle<GpuBuffer> buffer)
    {
        ScopedLock StorageLock{bufferPackage.StorageMutex, gpuViewPackage.StorageMutex};
        GpuBuffer* const bufferPtr = bufferPackage.Storage.Lookup(buffer);
        if (bufferPtr == nullptr)
        {
            return Handle<GpuView>{};
        }

        const GpuView newView = gpuViewManager.RequestConstantBufferView(*bufferPtr);
        if (!newView)
        {
            return Handle<GpuView>{};
        }

        IG_CHECK(newView.Type == EGpuViewType::ConstantBufferView);
        return gpuViewPackage.Storage.Create(newView);
    }

    Handle<GpuView> RenderContext::CreateConstantBufferView(const Handle<GpuBuffer> buffer, const Size offset, const Size sizeInBytes)
    {
        ScopedLock StorageLock{bufferPackage.StorageMutex, gpuViewPackage.StorageMutex};
        GpuBuffer* const bufferPtr = bufferPackage.Storage.Lookup(buffer);
        if (bufferPtr == nullptr)
        {
            return Handle<GpuView>{};
        }

        const GpuView newView = gpuViewManager.RequestConstantBufferView(*bufferPtr, offset, sizeInBytes);
        if (!newView)
        {
            return Handle<GpuView>{};
        }

        IG_CHECK(newView.Type == EGpuViewType::ConstantBufferView);
        return gpuViewPackage.Storage.Create(newView);
    }

    Handle<GpuView> RenderContext::CreateShaderResourceView(const Handle<GpuBuffer> buffer)
    {
        ScopedLock StorageLock{bufferPackage.StorageMutex, gpuViewPackage.StorageMutex};
        GpuBuffer* const bufferPtr = bufferPackage.Storage.Lookup(buffer);
        if (bufferPtr == nullptr)
        {
            return Handle<GpuView>{};
        }

        const GpuView newView = gpuViewManager.RequestShaderResourceView(*bufferPtr);
        if (!newView)
        {
            return Handle<GpuView>{};
        }

        IG_CHECK(newView.Type == EGpuViewType::ShaderResourceView);
        return gpuViewPackage.Storage.Create(newView);
    }

    Handle<GpuView> RenderContext::CreateUnorderedAccessView(const Handle<GpuBuffer> buffer)
    {
        ScopedLock StorageLock{bufferPackage.StorageMutex, gpuViewPackage.StorageMutex};
        GpuBuffer* const bufferPtr = bufferPackage.Storage.Lookup(buffer);
        if (bufferPtr == nullptr)
        {
            return Handle<GpuView>{};
        }

        const GpuView newView = gpuViewManager.RequestUnorderedAccessView(*bufferPtr);
        if (!newView)
        {
            return Handle<GpuView>{};
        }

        IG_CHECK(newView.Type == EGpuViewType::UnorderedAccessView);
        return gpuViewPackage.Storage.Create(newView);
    }

    Handle<GpuView> RenderContext::CreateShaderResourceView(Handle<GpuTexture> texture, const GpuTextureSrvDesc& srvDesc, const DXGI_FORMAT desireViewFormat /*= DXGI_FORMAT_UNKNOWN*/)
    {
        ScopedLock StorageLock{texturePackage.StorageMutex, gpuViewPackage.StorageMutex};
        GpuTexture* const texturePtr = texturePackage.Storage.Lookup(texture);
        if (texturePtr == nullptr)
        {
            return Handle<GpuView>{};
        }

        const GpuView newView = gpuViewManager.RequestShaderResourceView(*texturePtr, srvDesc, desireViewFormat);
        if (!newView)
        {
            return Handle<GpuView>{};
        }

        IG_CHECK(newView.Type == EGpuViewType::ShaderResourceView);
        return gpuViewPackage.Storage.Create(newView);
    }

    Handle<GpuView> RenderContext::CreateUnorderedAccessView(Handle<GpuTexture> texture, const GpuTextureUavDesc& uavDesc, const DXGI_FORMAT desireViewFormat /*= DXGI_FORMAT_UNKNOWN*/)
    {
        ScopedLock StorageLock{texturePackage.StorageMutex, gpuViewPackage.StorageMutex};
        GpuTexture* const texturePtr = texturePackage.Storage.Lookup(texture);
        if (texturePtr == nullptr)
        {
            return Handle<GpuView>{};
        }

        const GpuView newView = gpuViewManager.RequestUnorderedAccessView(*texturePtr, uavDesc, desireViewFormat);
        if (!newView)
        {
            return Handle<GpuView>{};
        }

        IG_CHECK(newView.Type == EGpuViewType::UnorderedAccessView);
        return gpuViewPackage.Storage.Create(newView);
    }

    Handle<GpuView> RenderContext::CreateRenderTargetView(Handle<GpuTexture> texture, const GpuTextureRtvDesc& rtvDesc, const DXGI_FORMAT desireViewFormat /*= DXGI_FORMAT_UNKNOWN*/)
    {
        ScopedLock StorageLock{texturePackage.StorageMutex, gpuViewPackage.StorageMutex};
        GpuTexture* const texturePtr = texturePackage.Storage.Lookup(texture);
        if (texturePtr == nullptr)
        {
            return Handle<GpuView>{};
        }

        const GpuView newView = gpuViewManager.RequestRenderTargetView(*texturePtr, rtvDesc, desireViewFormat);
        if (!newView)
        {
            return Handle<GpuView>{};
        }

        IG_CHECK(newView.Type == EGpuViewType::RenderTargetView);
        return gpuViewPackage.Storage.Create(newView);
    }

    Handle<GpuView> RenderContext::CreateDepthStencilView(Handle<GpuTexture> texture, const GpuTextureDsvDesc& dsvDesc, const DXGI_FORMAT desireViewFormat /*= DXGI_FORMAT_UNKNOWN*/)
    {
        ScopedLock StorageLock{texturePackage.StorageMutex, gpuViewPackage.StorageMutex};
        GpuTexture* const texturePtr = texturePackage.Storage.Lookup(texture);
        if (texturePtr == nullptr)
        {
            return Handle<GpuView>{};
        }

        const GpuView newView = gpuViewManager.RequestDepthStencilView(*texturePtr, dsvDesc, desireViewFormat);
        if (!newView)
        {
            return Handle<GpuView>{};
        }

        IG_CHECK(newView.Type == EGpuViewType::DepthStencilView);
        return gpuViewPackage.Storage.Create(newView);
    }

    Handle<GpuView> RenderContext::CreateSamplerView(const D3D12_SAMPLER_DESC& desc)
    {
        ReadWriteLock gpuViewStorageLock{gpuViewPackage.StorageMutex};
        const GpuView newView = gpuViewManager.RequestSampler(desc);
        if (!newView)
        {
            return Handle<GpuView>{};
        }

        IG_CHECK(newView.Type == EGpuViewType::Sampler);
        return gpuViewPackage.Storage.Create(newView);
    }

    Handle<GpuView> RenderContext::CreateGpuView(const EGpuViewType type)
    {
        IG_CHECK(type != EGpuViewType::Unknown);
        ReadWriteLock gpuViewStorageLock{gpuViewPackage.StorageMutex};
        const GpuView newView = gpuViewManager.Allocate(type);
        if (!newView)
        {
            return Handle<GpuView>{};
        }

        IG_CHECK(newView.Type != EGpuViewType::Unknown);
        return gpuViewPackage.Storage.Create(newView);
    }

    void RenderContext::DestroyBuffer(const Handle<GpuBuffer> buffer)
    {
        if (buffer)
        {
            ScopedLock packageLock{bufferPackage.StorageMutex, bufferPackage.DeferredDestroyPendingListMutex.Resources[currentLocalFrameIdx]};
            bufferPackage.DeferredDestroyPendingList.Resources[currentLocalFrameIdx].emplace_back(buffer);
            bufferPackage.Storage.MarkAsDestroy(buffer);
        }
    }

    void RenderContext::DestroyTexture(const Handle<GpuTexture> texture)
    {
        if (texture)
        {
            ScopedLock packageLock{texturePackage.StorageMutex, texturePackage.DeferredDestroyPendingListMutex.Resources[currentLocalFrameIdx]};
            texturePackage.DeferredDestroyPendingList.Resources[currentLocalFrameIdx].emplace_back(texture);
            texturePackage.Storage.MarkAsDestroy(texture);
        }
    }

    void RenderContext::DestroyPipelineState(const Handle<PipelineState> state)
    {
        if (state)
        {
            ScopedLock packageLock{pipelineStatePackage.StorageMutex, pipelineStatePackage.DeferredDestroyPendingListMutex.Resources[currentLocalFrameIdx]};
            pipelineStatePackage.DeferredDestroyPendingList.Resources[currentLocalFrameIdx].emplace_back(state);
            pipelineStatePackage.Storage.MarkAsDestroy(state);
        }
    }

    void RenderContext::DestroyGpuView(const Handle<GpuView> view)
    {
        if (view)
        {
            ScopedLock packageLock{gpuViewPackage.StorageMutex, gpuViewPackage.DeferredDestroyPendingListMutex.Resources[currentLocalFrameIdx]};
            gpuViewPackage.DeferredDestroyPendingList.Resources[currentLocalFrameIdx].emplace_back(view);
            gpuViewPackage.Storage.MarkAsDestroy(view);
        }
    }

    GpuBuffer* RenderContext::Lookup(const Handle<GpuBuffer> handle)
    {
        ReadOnlyLock bufferStorageLock{bufferPackage.StorageMutex};
        return bufferPackage.Storage.Lookup(handle);
    }

    const GpuBuffer* RenderContext::Lookup(const Handle<GpuBuffer> handle) const
    {
        ReadOnlyLock bufferStorageLock{bufferPackage.StorageMutex};
        return bufferPackage.Storage.Lookup(handle);
    }

    GpuTexture* RenderContext::Lookup(const Handle<GpuTexture> handle)
    {
        ReadOnlyLock textureStorageLock{texturePackage.StorageMutex};
        return texturePackage.Storage.Lookup(handle);
    }

    const GpuTexture* RenderContext::Lookup(const Handle<GpuTexture> handle) const
    {
        ReadOnlyLock textureStorageLock{texturePackage.StorageMutex};
        return texturePackage.Storage.Lookup(handle);
    }

    PipelineState* RenderContext::Lookup(const Handle<PipelineState> handle)
    {
        ReadOnlyLock pipelineStateStorageLock{pipelineStatePackage.StorageMutex};
        return pipelineStatePackage.Storage.Lookup(handle);
    }

    const PipelineState* RenderContext::Lookup(const Handle<PipelineState> handle) const
    {
        ReadOnlyLock pipelineStateStorageLock{pipelineStatePackage.StorageMutex};
        return pipelineStatePackage.Storage.Lookup(handle);
    }

    GpuView* RenderContext::Lookup(const Handle<GpuView> handle)
    {
        ReadOnlyLock gpuViewStorageLock{gpuViewPackage.StorageMutex};
        return gpuViewPackage.Storage.Lookup(handle);
    }

    const GpuView* RenderContext::Lookup(const Handle<GpuView> handle) const
    {
        ReadOnlyLock gpuViewStorageLock{gpuViewPackage.StorageMutex};
        return gpuViewPackage.Storage.Lookup(handle);
    }

    void RenderContext::FlushQueues()
    {
        mainGfxQueue.MakeSyncPointWithSignal().WaitOnCpu();
        asyncComputeQueue.MakeSyncPointWithSignal().WaitOnCpu();
        frameCriticalAsyncCopyQueue.MakeSyncPointWithSignal().WaitOnCpu();
        nonFrameCriticalAsyncCopyQueue.MakeSyncPointWithSignal().WaitOnCpu();
    }

    void RenderContext::PreRender(const LocalFrameIndex localFrameIdx, GpuSyncPoint& prevFrameLastSyncPoint)
    {
        this->currentLocalFrameIdx = localFrameIdx;
        mainGfxCmdListPool.PreRender(localFrameIdx);
        asyncComputeCmdListPool.PreRender(localFrameIdx);
        asyncCopyCmdListPool.PreRender(localFrameIdx);
        frameCriticalGpuUploader.PreRender(localFrameIdx);
        nonFrameCriticalGpuUploader.PreRender(localFrameIdx);
        if (unifiedMeshStorage != nullptr)
        {
            unifiedMeshStorage->PreRender(localFrameIdx);
        }

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
                const GpuView& gpuView = *gpuViewPackage.Storage.LookupUnsafe(handle);
                gpuViewManager.Deallocate(gpuView);
                gpuViewPackage.Storage.Destroy(handle);
            }
            gpuViewPackage.DeferredDestroyPendingList.Resources[localFrameIdx].clear();
        }

        mainGfxQueue.Wait(prevFrameLastSyncPoint);
        asyncComputeQueue.Wait(prevFrameLastSyncPoint);
        frameCriticalAsyncCopyQueue.Wait(prevFrameLastSyncPoint);
    }

    void RenderContext::PostRender([[maybe_unused]] const LocalFrameIndex localFrameIdx)
    {}
} // namespace ig
