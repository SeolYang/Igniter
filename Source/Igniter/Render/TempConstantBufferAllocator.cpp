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
        RenderContext& renderContext, const Size reservedBufferSizeInBytes /*= DefaultReservedBufferSizeInBytes*/)
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
        const Size allocSizeInBytes = desc.GetSizeAsBytes();
        const Size offset = allocatedSizeInBytes[localFrameIdx];
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
        for (const Handle<GpuView> gpuViewHandle : allocatedViews[localFrameIdx])
        {
            renderContext->DestroyGpuView(gpuViewHandle);
        }
        allocatedViews[localFrameIdx].clear();
        allocatedSizeInBytes[localFrameIdx] = 0;
    }
} // namespace ig
