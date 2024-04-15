#pragma once
#include <Igniter.h>
#include <D3D12/CommandQueue.h>
#include <D3D12/CommandContextPool.h>
#include <Render/GpuUploader.h>
#include <Render/GpuViewManager.h>

namespace ig
{
    class RenderDevice;

    class RenderContext final
    {
    public:
        RenderContext(DeferredDeallocator& deferredDeallocator, RenderDevice& renderDevice,
                      HandleManager&       handleManager);
        ~RenderContext() = default;

        RenderContext(const RenderContext&)     = delete;
        RenderContext(RenderContext&&) noexcept = delete;

        RenderContext& operator=(const RenderContext&)     = delete;
        RenderContext& operator=(RenderContext&&) noexcept = delete;

        CommandQueue&       GetMainGfxQueue() { return mainGfxQueue; }
        CommandContextPool& GetMainGfxCommandContextPool() { return mainGfxCmdCtxPool; }
        CommandQueue&       GetAsyncComputeQueue() { return asyncComputeQueue; }
        CommandQueue&       GetAsyncCopyQueue() { return asyncComputeQueue; }
        GpuViewManager&     GetGpuViewManager() { return gpuViewManager; }
        GpuUploader&        GetGpuUploader() { return gpuUploader; }

        void FlushQueues();

    private:
        CommandQueue       mainGfxQueue;
        CommandContextPool mainGfxCmdCtxPool;
        CommandQueue       asyncComputeQueue;
        CommandQueue       asyncCopyQueue;

        GpuViewManager gpuViewManager;
        GpuUploader    gpuUploader;
    };
}
