#include <Igniter.h>
#include <D3D12/RenderDevice.h>
#include <Render/RenderContext.h>

namespace ig
{
    RenderContext::RenderContext(DeferredDeallocator& deferredDeallocator, RenderDevice& renderDevice,
                                 HandleManager&       handleManager) :
                                                                     mainGfxQueue(
                                                                         *renderDevice.CreateCommandQueue(
                                                                             "MainGfx", EQueueType::Direct))
                                                                     , mainGfxCmdCtxPool(
                                                                         deferredDeallocator, renderDevice,
                                                                         EQueueType::Direct)
                                                                     , asyncComputeQueue(
                                                                         *renderDevice.CreateCommandQueue(
                                                                             "AsyncCompute", EQueueType::AsyncCompute))
                                                                     , asyncCopyQueue(
                                                                         *renderDevice.CreateCommandQueue(
                                                                             "AsyncCopy", EQueueType::Copy))
                                                                     , gpuViewManager(
                                                                         handleManager, deferredDeallocator,
                                                                         renderDevice)
                                                                     , gpuUploader(renderDevice, asyncCopyQueue)
    {
    }

    void RenderContext::FlushQueues()
    {
        GpuSync mainGfxSync{mainGfxQueue.MakeSync()};
        GpuSync asyncComputeSync{asyncComputeQueue.MakeSync()};
        GpuSync asyncCopySync{asyncCopyQueue.MakeSync()};
        mainGfxSync.WaitOnCpu();
        asyncComputeSync.WaitOnCpu();
        asyncCopySync.WaitOnCpu();
    }
}
