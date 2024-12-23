#include "Igniter/Igniter.h"
#include "Igniter/Core/FrameManager.h"
#include "Igniter/Core/Window.h"
#include "Igniter/Render/GpuStorage.h"
#include "Igniter/Render/Vertex.h"
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
        swapchain(MakePtr<Swapchain>(window, *this, NumFramesInFlight)) { }

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

    RenderResource<GpuBuffer> RenderContext::CreateBuffer(const GpuBufferDesc& desc)
    {
        Option<GpuBuffer> newBuffer = gpuDevice.CreateBuffer(desc);
        if (!newBuffer)
        {
            return RenderResource<GpuBuffer>{ };
        }

        ReadWriteLock bufferRegistryLock{bufferPackage.Mut};
        return bufferPackage.Registry.Create(std::move(newBuffer).value());
    }

    RenderResource<GpuTexture> RenderContext::CreateTexture(const GpuTextureDesc& desc)
    {
        Option<GpuTexture> newTexture = gpuDevice.CreateTexture(desc);
        if (!newTexture)
        {
            return RenderResource<GpuTexture>{ };
        }

        ReadWriteLock textureRegistryLock{texturePackage.Mut};
        return texturePackage.Registry.Create(std::move(newTexture).value());
    }

    RenderResource<GpuTexture> RenderContext::CreateTexture(GpuTexture&& gpuTexture)
    {
        ReadWriteLock textureRegistryLock{texturePackage.Mut};
        return texturePackage.Registry.Create(std::move(gpuTexture));
    }

    RenderResource<PipelineState> RenderContext::CreatePipelineState(const GraphicsPipelineStateDesc& desc)
    {
        Option<PipelineState> newPipelineState = gpuDevice.CreateGraphicsPipelineState(desc);
        if (!newPipelineState)
        {
            return RenderResource<PipelineState>{ };
        }

        ReadWriteLock pipelineStateRegistryLock{pipelineStatePackage.Mut};
        return pipelineStatePackage.Registry.Create(std::move(newPipelineState).value());
    }

    RenderResource<PipelineState> RenderContext::CreatePipelineState(const ComputePipelineStateDesc& desc)
    {
        Option<PipelineState> newPipelineState = gpuDevice.CreateComputePipelineState(desc);
        if (!newPipelineState)
        {
            return RenderResource<PipelineState>{ };
        }

        ReadWriteLock pipelineStateRegistryLock{pipelineStatePackage.Mut};
        return pipelineStatePackage.Registry.Create(std::move(newPipelineState).value());
    }

    RenderResource<GpuView> RenderContext::CreateConstantBufferView(const RenderResource<GpuBuffer> buffer)
    {
        ScopedLock       registryLock{bufferPackage.Mut, gpuViewPackage.Mut};
        GpuBuffer* const bufferPtr = bufferPackage.Registry.Lookup(buffer);
        if (bufferPtr == nullptr)
        {
            return RenderResource<GpuView>{ };
        }

        const GpuView newView = gpuViewManager.RequestConstantBufferView(*bufferPtr);
        if (!newView)
        {
            return RenderResource<GpuView>{ };
        }

        IG_CHECK(newView.Type == EGpuViewType::ConstantBufferView);
        return gpuViewPackage.Registry.Create(newView);
    }

    RenderResource<GpuView> RenderContext::CreateConstantBufferView(const RenderResource<GpuBuffer> buffer, const size_t offset, const size_t sizeInBytes)
    {
        ScopedLock       registryLock{bufferPackage.Mut, gpuViewPackage.Mut};
        GpuBuffer* const bufferPtr = bufferPackage.Registry.Lookup(buffer);
        if (bufferPtr == nullptr)
        {
            return RenderResource<GpuView>{ };
        }

        const GpuView newView = gpuViewManager.RequestConstantBufferView(*bufferPtr, offset, sizeInBytes);
        if (!newView)
        {
            return RenderResource<GpuView>{ };
        }

        IG_CHECK(newView.Type == EGpuViewType::ConstantBufferView);
        return gpuViewPackage.Registry.Create(newView);
    }

    RenderResource<GpuView> RenderContext::CreateShaderResourceView(const RenderResource<GpuBuffer> buffer)
    {
        ScopedLock       registryLock{bufferPackage.Mut, gpuViewPackage.Mut};
        GpuBuffer* const bufferPtr = bufferPackage.Registry.Lookup(buffer);
        if (bufferPtr == nullptr)
        {
            return RenderResource<GpuView>{ };
        }

        const GpuView newView = gpuViewManager.RequestShaderResourceView(*bufferPtr);
        if (!newView)
        {
            return RenderResource<GpuView>{ };
        }

        IG_CHECK(newView.Type == EGpuViewType::ShaderResourceView);
        return gpuViewPackage.Registry.Create(newView);
    }

    RenderResource<GpuView> RenderContext::CreateUnorderedAccessView(const RenderResource<GpuBuffer> buffer)
    {
        ScopedLock       registryLock{bufferPackage.Mut, gpuViewPackage.Mut};
        GpuBuffer* const bufferPtr = bufferPackage.Registry.Lookup(buffer);
        if (bufferPtr == nullptr)
        {
            return RenderResource<GpuView>{ };
        }

        const GpuView newView = gpuViewManager.RequestUnorderedAccessView(*bufferPtr);
        if (!newView)
        {
            return RenderResource<GpuView>{ };
        }

        IG_CHECK(newView.Type == EGpuViewType::UnorderedAccessView);
        return gpuViewPackage.Registry.Create(newView);
    }

    RenderResource<GpuView> RenderContext::CreateShaderResourceView(RenderResource<GpuTexture> texture, const GpuTextureSrvDesc& srvDesc, const DXGI_FORMAT desireViewFormat /*= DXGI_FORMAT_UNKNOWN*/)
    {
        ScopedLock        registryLock{texturePackage.Mut, gpuViewPackage.Mut};
        GpuTexture* const texturePtr = texturePackage.Registry.Lookup(texture);
        if (texturePtr == nullptr)
        {
            return RenderResource<GpuView>{ };
        }

        const GpuView newView = gpuViewManager.RequestShaderResourceView(*texturePtr, srvDesc, desireViewFormat);
        if (!newView)
        {
            return RenderResource<GpuView>{ };
        }

        IG_CHECK(newView.Type == EGpuViewType::ShaderResourceView);
        return gpuViewPackage.Registry.Create(newView);
    }

    RenderResource<GpuView> RenderContext::CreateUnorderedAccessView(RenderResource<GpuTexture> texture, const GpuTextureUavDesc& uavDesc, const DXGI_FORMAT desireViewFormat /*= DXGI_FORMAT_UNKNOWN*/)
    {
        ScopedLock        registryLock{texturePackage.Mut, gpuViewPackage.Mut};
        GpuTexture* const texturePtr = texturePackage.Registry.Lookup(texture);
        if (texturePtr == nullptr)
        {
            return RenderResource<GpuView>{ };
        }

        const GpuView newView = gpuViewManager.RequestUnorderedAccessView(*texturePtr, uavDesc, desireViewFormat);
        if (!newView)
        {
            return RenderResource<GpuView>{ };
        }

        IG_CHECK(newView.Type == EGpuViewType::UnorderedAccessView);
        return gpuViewPackage.Registry.Create(newView);
    }

    RenderResource<GpuView> RenderContext::CreateRenderTargetView(RenderResource<GpuTexture> texture, const GpuTextureRtvDesc& rtvDesc, const DXGI_FORMAT desireViewFormat /*= DXGI_FORMAT_UNKNOWN*/)
    {
        ScopedLock        registryLock{texturePackage.Mut, gpuViewPackage.Mut};
        GpuTexture* const texturePtr = texturePackage.Registry.Lookup(texture);
        if (texturePtr == nullptr)
        {
            return RenderResource<GpuView>{ };
        }

        const GpuView newView = gpuViewManager.RequestRenderTargetView(*texturePtr, rtvDesc, desireViewFormat);
        if (!newView)
        {
            return RenderResource<GpuView>{ };
        }

        IG_CHECK(newView.Type == EGpuViewType::RenderTargetView);
        return gpuViewPackage.Registry.Create(newView);
    }

    RenderResource<GpuView> RenderContext::CreateDepthStencilView(RenderResource<GpuTexture> texture, const GpuTextureDsvDesc& dsvDesc, const DXGI_FORMAT desireViewFormat /*= DXGI_FORMAT_UNKNOWN*/)
    {
        ScopedLock        registryLock{texturePackage.Mut, gpuViewPackage.Mut};
        GpuTexture* const texturePtr = texturePackage.Registry.Lookup(texture);
        if (texturePtr == nullptr)
        {
            return RenderResource<GpuView>{ };
        }

        const GpuView newView = gpuViewManager.RequestDepthStencilView(*texturePtr, dsvDesc, desireViewFormat);
        if (!newView)
        {
            return RenderResource<GpuView>{ };
        }

        IG_CHECK(newView.Type == EGpuViewType::DepthStencilView);
        return gpuViewPackage.Registry.Create(newView);
    }

    RenderResource<GpuView> RenderContext::CreateSamplerView(const D3D12_SAMPLER_DESC& desc)
    {
        ReadWriteLock gpuViewRegistryLock{gpuViewPackage.Mut};
        const GpuView newView = gpuViewManager.RequestSampler(desc);
        if (!newView)
        {
            return RenderResource<GpuView>{ };
        }

        IG_CHECK(newView.Type == EGpuViewType::Sampler);
        return gpuViewPackage.Registry.Create(newView);
    }

    RenderResource<GpuView> RenderContext::CreateGpuView(const EGpuViewType type)
    {
        IG_CHECK(type != EGpuViewType::Unknown);
        ReadWriteLock gpuViewRegistryLock{gpuViewPackage.Mut};
        const GpuView newView = gpuViewManager.Allocate(type);
        if (!newView)
        {
            return RenderResource<GpuView>{ };
        }

        IG_CHECK(newView.Type != EGpuViewType::Unknown);
        return gpuViewPackage.Registry.Create(newView);
    }

    void RenderContext::DestroyBuffer(const RenderResource<GpuBuffer> buffer)
    {
        if (buffer)
        {
            ScopedLock packageLock{bufferPackage.Mut, bufferPackage.PendingListMut[lastLocalFrameIdx]};
            bufferPackage.PendingDestroyList[lastLocalFrameIdx].emplace_back(buffer);
            bufferPackage.Registry.Destroy(buffer);
        }
    }

    void RenderContext::DestroyTexture(const RenderResource<GpuTexture> texture)
    {
        if (texture)
        {
            ScopedLock packageLock{texturePackage.Mut, texturePackage.PendingListMut[lastLocalFrameIdx]};
            texturePackage.PendingDestroyList[lastLocalFrameIdx].emplace_back(texture);
            texturePackage.Registry.Destroy(texture);
        }
    }

    void RenderContext::DestroyPipelineState(const RenderResource<PipelineState> state)
    {
        if (state)
        {
            ScopedLock packageLock{pipelineStatePackage.Mut, pipelineStatePackage.PendingListMut[lastLocalFrameIdx]};
            pipelineStatePackage.PendingDestroyList[lastLocalFrameIdx].emplace_back(state);
            pipelineStatePackage.Registry.Destroy(state);
        }
    }

    void RenderContext::DestroyGpuView(const RenderResource<GpuView> view)
    {
        if (view)
        {
            ScopedLock packageLock{gpuViewPackage.Mut, gpuViewPackage.PendingListMut[lastLocalFrameIdx]};
            gpuViewPackage.PendingDestroyList[lastLocalFrameIdx].emplace_back(view);
            gpuViewPackage.Registry.Destroy(view);
        }
    }

    GpuBuffer* RenderContext::Lookup(const RenderResource<GpuBuffer> handle)
    {
        ReadOnlyLock bufferRegistryLock{bufferPackage.Mut};
        return bufferPackage.Registry.Lookup(handle);
    }

    const GpuBuffer* RenderContext::Lookup(const RenderResource<GpuBuffer> handle) const
    {
        ReadOnlyLock bufferRegistryLock{bufferPackage.Mut};
        return bufferPackage.Registry.Lookup(handle);
    }

    GpuTexture* RenderContext::Lookup(const RenderResource<GpuTexture> handle)
    {
        ReadOnlyLock textureRegistryLock{texturePackage.Mut};
        return texturePackage.Registry.Lookup(handle);
    }

    const GpuTexture* RenderContext::Lookup(const RenderResource<GpuTexture> handle) const
    {
        ReadOnlyLock textureRegistryLock{texturePackage.Mut};
        return texturePackage.Registry.Lookup(handle);
    }

    PipelineState* RenderContext::Lookup(const RenderResource<PipelineState> handle)
    {
        ReadOnlyLock pipelineStateRegistryLock{pipelineStatePackage.Mut};
        return pipelineStatePackage.Registry.Lookup(handle);
    }

    const PipelineState* RenderContext::Lookup(const RenderResource<PipelineState> handle) const
    {
        ReadOnlyLock pipelineStateRegistryLock{pipelineStatePackage.Mut};
        return pipelineStatePackage.Registry.Lookup(handle);
    }

    GpuView* RenderContext::Lookup(const RenderResource<GpuView> handle)
    {
        ReadOnlyLock gpuViewRegistryLock{gpuViewPackage.Mut};
        return gpuViewPackage.Registry.Lookup(handle);
    }

    const GpuView* RenderContext::Lookup(const RenderResource<GpuView> handle) const
    {
        ReadOnlyLock gpuViewRegistryLock{gpuViewPackage.Mut};
        return gpuViewPackage.Registry.Lookup(handle);
    }

    void RenderContext::FlushQueues()
    {
        mainGfxQueue.MakeSyncPointWithSignal().WaitOnCpu();
        asyncComputeQueue.MakeSyncPointWithSignal().WaitOnCpu();
        asyncCopyQueue.MakeSyncPointWithSignal().WaitOnCpu();
    }

    void RenderContext::PreRender(const LocalFrameIndex localFrameIdx)
    {
        this->lastLocalFrameIdx = localFrameIdx;
        mainGfxCmdCtxPool.PreRender(localFrameIdx);
        asyncComputeCmdCtxPool.PreRender(localFrameIdx);

        /* Flush Buffer Package [local frame] pending destroy */
        {
            ScopedLock bufferPackageLock{bufferPackage.Mut, bufferPackage.PendingListMut[localFrameIdx]};
            for (const auto handle : bufferPackage.PendingDestroyList[localFrameIdx])
            {
                bufferPackage.Registry.DestroyImmediate(handle);
            }
            bufferPackage.PendingDestroyList[localFrameIdx].clear();
        }

        /* Flush Texture Package [local frame] pending destroy */
        {
            ScopedLock texturePackageLock{texturePackage.Mut, texturePackage.PendingListMut[localFrameIdx]};
            for (const auto handle : texturePackage.PendingDestroyList[localFrameIdx])
            {
                texturePackage.Registry.DestroyImmediate(handle);
            }
            texturePackage.PendingDestroyList[localFrameIdx].clear();
        }

        /* Flush Pipeline State Package [local frame] pending destroy */
        {
            ScopedLock pipelineStatePackageLock{pipelineStatePackage.Mut, pipelineStatePackage.PendingListMut[localFrameIdx]};
            for (const auto handle : pipelineStatePackage.PendingDestroyList[localFrameIdx])
            {
                pipelineStatePackage.Registry.DestroyImmediate(handle);
            }
            pipelineStatePackage.PendingDestroyList[localFrameIdx].clear();
        }

        /* Flush Gpu View Package [local frame] pending destroy */
        {
            ScopedLock gpuViewPackageLock{gpuViewPackage.Mut, gpuViewPackage.PendingListMut[localFrameIdx]};
            for (const auto handle : gpuViewPackage.PendingDestroyList[localFrameIdx])
            {
                gpuViewManager.Deallocate(*gpuViewPackage.Registry.LookupUnsafe(handle));
                gpuViewPackage.Registry.DestroyImmediate(handle);
            }
            gpuViewPackage.PendingDestroyList[localFrameIdx].clear();
        }
    }

    void RenderContext::PostRender([[maybe_unused]] const LocalFrameIndex localFrameIdx)
    {
        swapchain->Present();
    }
} // namespace ig
