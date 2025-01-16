#include "Igniter/Igniter.h"
#include "Igniter/Core/FrameManager.h"
#include "Igniter/D3D12/GpuView.h"
#include "Igniter/D3D12/GpuDevice.h"
#include "Igniter/D3D12/GpuBuffer.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/TempConstantBufferAllocator.h"

namespace ig
{
    TempConstantBufferAllocator::TempConstantBufferAllocator(
        RenderContext& renderContext, const size_t reservedBufferSizeInBytes /*= DefaultReservedBufferSizeInBytes*/)
        : renderContext(&renderContext)
        , reservedSizeInBytesPerFrame(reservedBufferSizeInBytes)
    {
        IG_CHECK(reservedSizeInBytesPerFrame % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0);

        GpuBufferDesc desc{};
        desc.AsConstantBuffer(static_cast<U32>(reservedSizeInBytesPerFrame));

        for (const LocalFrameIndex localFrameIdx : std::views::iota(0Ui8, NumFramesInFlight))
        {
            allocatedSizeInBytes[localFrameIdx] = 0;
            desc.DebugName = String(std::format("TempConstantBuffer.LocalFrame{}", localFrameIdx));
            buffers[localFrameIdx] = renderContext.CreateBuffer(desc);
        }
    }

    TempConstantBufferAllocator::~TempConstantBufferAllocator()
    {
        for (const LocalFrameIndex localFrameIdx : views::iota(0Ui8, NumFramesInFlight))
        {
            Reset(localFrameIdx);
            renderContext->DestroyBuffer(buffers[localFrameIdx]);
        }
    }

    TempConstantBuffer TempConstantBufferAllocator::Allocate(const LocalFrameIndex localFrameIdx, const GpuBufferDesc& desc)
    {
        IG_CHECK(localFrameIdx < NumFramesInFlight);
        UniqueLock lock{mutexes[localFrameIdx]};
        const size_t allocSizeInBytes = desc.GetSizeAsBytes();
        const size_t offset = allocatedSizeInBytes[localFrameIdx];
        allocatedSizeInBytes[localFrameIdx] += allocSizeInBytes;
        IG_CHECK(offset <= reservedSizeInBytesPerFrame);

        GpuBuffer* bufferPtr = renderContext->Lookup(buffers[localFrameIdx]);
        if (bufferPtr == nullptr)
        {
            return TempConstantBuffer{{}, nullptr};
        }

        allocatedViews[localFrameIdx].emplace_back(renderContext->CreateConstantBufferView(buffers[localFrameIdx], offset, allocSizeInBytes));
        if (!allocatedViews[localFrameIdx].back())
        {
            return TempConstantBuffer{{}, nullptr};
        }

        return TempConstantBuffer{allocatedViews[localFrameIdx].back(), bufferPtr->Map(offset)};
    }

    void TempConstantBufferAllocator::Reset(const LocalFrameIndex localFrameIdx)
    {
        UniqueLock lock{mutexes[localFrameIdx]};
        for (const RenderHandle<GpuView> gpuViewHandle : allocatedViews[localFrameIdx])
        {
            renderContext->DestroyGpuView(gpuViewHandle);
        }
        allocatedViews[localFrameIdx].clear();
        allocatedSizeInBytes[localFrameIdx] = 0;
    }

    void TempConstantBufferAllocator::InitBufferStateTransition(CommandList& cmdList)
    {
        for (const RenderHandle<GpuBuffer> buffer : buffers)
        {
            GpuBuffer* bufferPtr = renderContext->Lookup(buffer);
            IG_CHECK(bufferPtr != nullptr);

            /* #sy_todo 제대로 상태 전이 시킬 것 */
            cmdList.AddPendingBufferBarrier(*bufferPtr,
                                            D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_ALL_SHADING,
                                            D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_CONSTANT_BUFFER);
        }
        cmdList.FlushBarriers();
    }
} // namespace ig
