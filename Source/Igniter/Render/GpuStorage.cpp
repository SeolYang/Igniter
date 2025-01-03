#include "Igniter/Igniter.h"
#include "Igniter/D3D12/GpuSyncPoint.h"
#include "Igniter/D3D12/GpuBuffer.h"
#include "Igniter/D3D12/GpuView.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/GpuStorage.h"

namespace ig
{
    GpuStorage::GpuStorage(RenderContext& renderContext, const String debugName, const U32 elementSize, const U32 initialNumElements, const bool bIsShaderReadWritable /*= false*/) :
        renderContext(renderContext),
        debugName(debugName),
        elementSize(elementSize),
        bufferSize(initialNumElements * elementSize),
        bIsShaderReadWritable(bIsShaderReadWritable),
        fence(renderContext.GetGpuDevice().CreateFence(debugName).value())
    {
        IG_CHECK(initialNumElements > 0);
        IG_CHECK(elementSize > 0);
        GpuBufferDesc bufferDesc{};
        bufferDesc.DebugName = debugName;
        bufferDesc.AsStructuredBuffer(elementSize, initialNumElements, bIsShaderReadWritable);

        gpuBuffer = renderContext.CreateBuffer(bufferDesc);
        srv = renderContext.CreateShaderResourceView(gpuBuffer);

        if (bIsShaderReadWritable)
        {
            uav = renderContext.CreateUnorderedAccessView(gpuBuffer);
        }

        const D3D12MA::VIRTUAL_BLOCK_DESC blockDesc{.Size = bufferSize};
        D3D12MA::VirtualBlock* newVirtualBlock{nullptr};
        [[maybe_unused]] const HRESULT hr = D3D12MA::CreateVirtualBlock(&blockDesc, &newVirtualBlock);
        // 여기서 Virtual Block 할당 실패를 핸들 해야하나? 로그에 남겨야 하나?
        blocks.emplace_back(Block{.VirtualBlock = newVirtualBlock, .Offset = 0});
    }

    GpuStorage::~GpuStorage()
    {
        IG_CHECK(allocatedSize == 0);
        if (srv)
        {
            renderContext.DestroyGpuView(srv);
        }

        if (uav)
        {
            renderContext.DestroyGpuView(uav);
        }

        if (gpuBuffer)
        {
            renderContext.DestroyBuffer(gpuBuffer);
        }

        for (const Block& block : blocks)
        {
            if (block.VirtualBlock != nullptr)
            {
                block.VirtualBlock->Release();
            }
        }
    }

    GpuStorage::Allocation GpuStorage::Allocate(const Size numElements)
    {
        IG_CHECK(numElements > 0);
        IG_CHECK(blocks.size() > 0);
        if (gpuBuffer.IsNull())
        {
            return Allocation::Invalid();
        }

        const Size allocSize = numElements * elementSize;
        Allocation newAllocation{};
        for (Index blockIdx = 0; blockIdx < blocks.size(); ++blockIdx)
        {
            if (AllocateWithBlock(allocSize, blockIdx, newAllocation))
            {
                return newAllocation;
            }
        }

        // Out Of Memory!
        if (!Grow(allocSize))
        {
            return Allocation::Invalid();
        }

        const Index lastBlockIdx = blocks.size() - 1;
        if (!AllocateWithBlock(allocSize, lastBlockIdx, newAllocation))
        {
            return Allocation::Invalid();
        }

        IG_CHECK(numElements == newAllocation.NumElements);
        return newAllocation;
    }

    void GpuStorage::Deallocate(const Allocation& allocation)
    {
        IG_CHECK(allocation.IsValid());
        IG_CHECK(allocation.BlockIndex < blocks.size());
        Block& block = blocks[allocation.BlockIndex];
        IG_CHECK(block.VirtualBlock != nullptr);
        block.VirtualBlock->FreeAllocation(allocation.VirtualAllocation);
        IG_CHECK(allocatedSize >= allocation.AllocSize);
        allocatedSize -= allocation.AllocSize;
    }

    void GpuStorage::ForceReset()
    {
        allocatedSize = 0;

        for (const Block& block : blocks)
        {
            if (block.VirtualBlock != nullptr)
            {
                block.VirtualBlock->Clear();
                block.VirtualBlock->Release();
            }
        }
        blocks.clear();

        const D3D12MA::VIRTUAL_BLOCK_DESC blockDesc{.Size = bufferSize};
        D3D12MA::VirtualBlock* newVirtualBlock{nullptr};
        [[maybe_unused]] const HRESULT hr = D3D12MA::CreateVirtualBlock(&blockDesc, &newVirtualBlock);
        blocks.emplace_back(Block{.VirtualBlock = newVirtualBlock, .Offset = 0});
    }

    bool GpuStorage::AllocateWithBlock(const Size allocSize, const Index blockIdx, Allocation& allocation)
    {
        IG_CHECK(blockIdx < blocks.size());
        const D3D12MA::VIRTUAL_ALLOCATION_DESC allocDesc{.Size = allocSize};
        D3D12MA::VirtualAllocation virtualAllocation{};
        Size allocOffset = 0;

        Block& block = blocks[blockIdx];
        IG_CHECK(block.VirtualBlock != nullptr);
        if (SUCCEEDED(block.VirtualBlock->Allocate(&allocDesc, &virtualAllocation, &allocOffset)))
        {
            const Size storageOffset = block.Offset + allocOffset;
            allocatedSize += allocSize;
            allocation = Allocation{
                .BlockIndex = blockIdx,
                .VirtualAllocation = virtualAllocation,
                .Offset = storageOffset,
                .OffsetIndex = storageOffset / elementSize,
                .AllocSize = allocSize,
                .NumElements = allocSize / elementSize};

            return true;
        }

        return false;
    }

    bool GpuStorage::Grow(const Size newAllocSize)
    {
        IG_CHECK(gpuBuffer);
        IG_CHECK(bufferSize > 0);
        IG_CHECK(newAllocSize > 0);
        IG_CHECK((newAllocSize % elementSize) == 0);
        IG_CHECK((bufferSize % elementSize) == 0);

        const Size newBufferSize = std::max(bufferSize * GrowthMultiplier, bufferSize + newAllocSize);
        IG_CHECK(newBufferSize > 0);
        IG_CHECK((newBufferSize - bufferSize) > 0);

        GpuBufferDesc newGpuBufferDesc{};
        newGpuBufferDesc.DebugName = debugName;
        newGpuBufferDesc.AsStructuredBuffer(elementSize, static_cast<U32>(newBufferSize / elementSize), bIsShaderReadWritable);
        RenderHandle<GpuBuffer> newGpuBuffer = renderContext.CreateBuffer(newGpuBufferDesc);
        if (!newGpuBuffer)
        {
            return false;
        }

        GpuBuffer* gpuBufferPtr = renderContext.Lookup(gpuBuffer);
        GpuBuffer* newGpuBufferPtr = renderContext.Lookup(newGpuBuffer);
        IG_CHECK(gpuBufferPtr != nullptr);
        IG_CHECK(newGpuBufferPtr != nullptr);

        CommandQueue& asyncCopyQueue = renderContext.GetAsyncCopyQueue();
        CommandListPool& cmdListPool = renderContext.GetAsyncCopyCommandListPool();
        auto copyCmdList = cmdListPool.Request(FrameManager::GetLocalFrameIndex(), "GpuStorageGrowCopy"_fs);
        copyCmdList->Open();
        {
            copyCmdList->CopyBuffer(*gpuBufferPtr, *newGpuBufferPtr);
        }
        copyCmdList->Close();

        GpuSyncPoint newSyncPoint = fence.MakeSyncPoint();
        if (GpuSyncPoint prevSyncPoint = newSyncPoint.Prev();
            prevSyncPoint)
        {
            // 만약 버퍼에 대한 비동기 쓰기를 지원한다면 Write-After-Read Hazard 발생 가능성이 있음
            // 방지하기 위해선 GpuStorage가 별도의 Fence를 가지고 이를 사용해
            // ExecuteCommandLists 간의 명시적 동기화가 필요할것으로 보임
            asyncCopyQueue.Wait(prevSyncPoint);
        }

        ig::CommandList* copyCmdLists[] = {(ig::CommandList*)copyCmdList};
        asyncCopyQueue.ExecuteContexts(copyCmdLists);
        asyncCopyQueue.Signal(newSyncPoint);

        if (srv)
        {
            renderContext.DestroyGpuView(srv);
        }
        if (uav)
        {
            renderContext.DestroyGpuView(uav);
        }
        if (gpuBuffer)
        {
            renderContext.DestroyBuffer(gpuBuffer);
        }

        gpuBuffer = newGpuBuffer;
        srv = renderContext.CreateShaderResourceView(gpuBuffer);
        if (bIsShaderReadWritable)
        {
            uav = renderContext.CreateUnorderedAccessView(gpuBuffer);
        }

        const Size bufferSizeDiff = newBufferSize - bufferSize;
        const D3D12MA::VIRTUAL_BLOCK_DESC blockDesc{.Size = bufferSizeDiff};
        D3D12MA::VirtualBlock* newVirtualBlock{nullptr};
        [[maybe_unused]] const HRESULT hr = D3D12MA::CreateVirtualBlock(&blockDesc, &newVirtualBlock);
        // 여기서 Virtual Block 할당 실패를 핸들 해야하나? 로그에 남겨야 하나?
        blocks.emplace_back(Block{.VirtualBlock = newVirtualBlock, .Offset = bufferSize});

        bufferSize = newBufferSize;

        // Storage Fence를 통해 적절한 통제만 해준다면, 굳이 WaitOnCpu를 해주지 않아도 될 것으로 예상
        newSyncPoint.WaitOnCpu();
        return true;
    }
} // namespace ig
