#include "Igniter/Igniter.h"
#include "Igniter/Core/FrameManager.h"
#include "Igniter/D3D12/GpuView.h"
#include "Igniter/D3D12/RenderDevice.h"
#include "Igniter/D3D12/GpuBuffer.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/TempConstantBufferAllocator.h"

namespace ig
{
    TempConstantBufferAllocator::TempConstantBufferAllocator(const FrameManager& frameManager, RenderContext& renderContext,
        const size_t reservedBufferSizeInBytes /*= DefaultReservedBufferSizeInBytes*/)
        : frameManager(frameManager), renderContext(renderContext), reservedSizeInBytesPerFrame(reservedBufferSizeInBytes)
    {
        IG_CHECK(reservedSizeInBytesPerFrame % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0);

        GpuBufferDesc desc{};
        desc.AsConstantBuffer(static_cast<uint32_t>(reservedSizeInBytesPerFrame));

        for (const uint8_t localFrameIdx : std::views::iota(0Ui8, NumFramesInFlight))
        {
            allocatedSizeInBytes[localFrameIdx] = 0;
            desc.DebugName = String(std::format("TempConstantBuffer.LocalFrame{}", localFrameIdx));
            buffers[localFrameIdx] = renderContext.CreateBuffer(desc);
        }
    }

    TempConstantBufferAllocator::~TempConstantBufferAllocator()
    {
        for (const uint8_t localFrameIdx : views::iota(0Ui8, NumFramesInFlight))
        {
            BeginFrame(localFrameIdx);
            renderContext.DestroyBuffer(buffers[localFrameIdx]);
        }
    }

    TempConstantBuffer TempConstantBufferAllocator::Allocate(const GpuBufferDesc& desc)
    {
        const uint8_t localFrameIdx = frameManager.GetLocalFrameIndex();
        IG_CHECK(localFrameIdx < NumFramesInFlight);

        UniqueLock lock{mutexes[localFrameIdx]};
        const size_t allocSizeInBytes = desc.GetSizeAsBytes();
        const size_t offset = allocatedSizeInBytes[localFrameIdx];
        allocatedSizeInBytes[localFrameIdx] += allocSizeInBytes;
        IG_CHECK(offset <= reservedSizeInBytesPerFrame);

        GpuBuffer* bufferPtr = renderContext.Lookup(buffers[localFrameIdx]);
        if (bufferPtr == nullptr)
        {
            return TempConstantBuffer{{}, nullptr};
        }

        allocatedViews[localFrameIdx].emplace_back(renderContext.CreateConstantBufferView(buffers[localFrameIdx], offset, allocSizeInBytes));
        if (!allocatedViews[localFrameIdx].back())
        {
            return TempConstantBuffer{{}, nullptr};
        }

        return TempConstantBuffer{allocatedViews[localFrameIdx].back(), bufferPtr->Map(offset)};
    }

    void TempConstantBufferAllocator::BeginFrame(const uint8_t localFrameIdx)
    {
        UniqueLock lock{mutexes[localFrameIdx]};
        for (const RenderResource<GpuView> gpuViewHandle : allocatedViews[localFrameIdx])
        {
            renderContext.DestroyGpuView(gpuViewHandle);
        }
        allocatedViews[localFrameIdx].clear();
        allocatedSizeInBytes[localFrameIdx] = 0;
    }

    void TempConstantBufferAllocator::InitBufferStateTransition(CommandContext& cmdCtx)
    {
        for (const RenderResource<GpuBuffer> buffer : buffers)
        {
            GpuBuffer* bufferPtr = renderContext.Lookup(buffer);
            IG_CHECK(bufferPtr != nullptr);

            /* #sy_todo 제대로 상태 전이 시킬 것 */
            cmdCtx.AddPendingBufferBarrier(*bufferPtr, D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_ALL_SHADING, D3D12_BARRIER_ACCESS_NO_ACCESS,
                D3D12_BARRIER_ACCESS_CONSTANT_BUFFER);
        }
        cmdCtx.FlushBarriers();
    }

    std::pair<size_t, size_t> TempConstantBufferAllocator::GetUsedSizeInBytes() const
    {
        UniqueLock lock{mutexes[frameManager.GetLocalFrameIndex()]};
        return std::make_pair(allocatedSizeInBytes[0], allocatedSizeInBytes[1]);
    }
}    // namespace ig