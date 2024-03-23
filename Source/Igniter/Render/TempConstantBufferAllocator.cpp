#include <Igniter.h>
#include <Render/TempConstantBufferAllocator.h>
#include <D3D12/RenderDevice.h>
#include <D3D12/GpuBuffer.h>
#include <D3D12/GpuBufferDesc.h>
#include <Render/GpuViewManager.h>
#include <D3D12/CommandContext.h>

namespace ig
{
    TempConstantBufferAllocator::TempConstantBufferAllocator(const FrameManager& frameManager, RenderDevice& renderDevice, HandleManager& handleManager, GpuViewManager& gpuViewManager, const uint32_t reservedSizeInBytesPerFrame)
        : frameManager(frameManager),
          renderDevice(renderDevice),
          handleManager(handleManager),
          gpuViewManager(gpuViewManager),
          reservedSizeInBytesPerFrame(reservedSizeInBytesPerFrame)
    {
        IG_CHECK(reservedSizeInBytesPerFrame % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0);

        GpuBufferDesc desc{};
        desc.AsConstantBuffer(reservedSizeInBytesPerFrame);

        for (const uint8_t localFrameIdx : std::views::iota(0Ui8, NumFramesInFlight))
        {
            allocatedSizeInBytes[localFrameIdx].store(0);
            desc.DebugName = String(std::format("TempConstantBuffer.LocalFrame{}", localFrameIdx));
            buffers.emplace_back(renderDevice.CreateBuffer(desc).value());
        }
    }

    TempConstantBufferAllocator::~TempConstantBufferAllocator()
    {
    }

    TempConstantBuffer TempConstantBufferAllocator::Allocate(const GpuBufferDesc& desc)
    {
        const size_t currentLocalFrameIdx = frameManager.GetLocalFrameIndex();
        IG_CHECK(currentLocalFrameIdx < NumFramesInFlight);
        const uint64_t allocSizeInBytes = desc.GetSizeAsBytes();
        const uint64_t offset = allocatedSizeInBytes[currentLocalFrameIdx].fetch_add(allocSizeInBytes);
        IG_CHECK(offset <= reservedSizeInBytesPerFrame);
        mappedBuffers[currentLocalFrameIdx].emplace_back(buffers[currentLocalFrameIdx].MapHandle(handleManager, offset));
        allocatedViews[currentLocalFrameIdx].emplace_back(gpuViewManager.RequestConstantBufferView(buffers[currentLocalFrameIdx], offset, allocSizeInBytes));

        return TempConstantBuffer{
            .Mapping = mappedBuffers[currentLocalFrameIdx].back().MakeRef(),
            .View = allocatedViews[currentLocalFrameIdx].back().MakeRef()
        };
    }

    void TempConstantBufferAllocator::DeallocateCurrentFrame()
    {
        const size_t currentLocalFrameIdx = frameManager.GetLocalFrameIndex();
        IG_CHECK(currentLocalFrameIdx < NumFramesInFlight);
        mappedBuffers[currentLocalFrameIdx].clear();
        allocatedViews[currentLocalFrameIdx].clear();
        allocatedSizeInBytes[currentLocalFrameIdx].store(0);
    }

    void TempConstantBufferAllocator::InitBufferStateTransition(CommandContext& cmdCtx)
    {
        for (auto& buffer : buffers)
        {
            cmdCtx.AddPendingBufferBarrier(buffer,
                                           D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_ALL_SHADING,
                                           D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_CONSTANT_BUFFER);
        }

        cmdCtx.FlushBarriers();
    }

} // namespace ig