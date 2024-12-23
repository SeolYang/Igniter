#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/D3D12/GpuSyncPoint.h"
#include "Igniter/D3D12/GpuBuffer.h"
#include "Igniter/D3D12/GpuFence.h"
#include "Igniter/Render/RenderContext.h"

namespace ig
{
    // 주의: 렌더링 로직 내에서 Allocation/Deallocation을 지양할 것!
    // 해당 저장 공간(버퍼) 대한 작업 시, 반드시 Storage Fence를 사용해서 올바르게
    // 버퍼 접근에 대한 동기화가 올바르게 일어 날 수 있도록 하여야한다.
    // 
    // 개선 희망 사항: 만약 할당 정보를 항상 핸들을 사용해서 접근 할 수 있게 만들면 내부적으로 Defragmentation
    // 알고리즘을 구현 할 수도 있을 것 같음.
    template <typename Ty>
    class GpuStorage
    {
    private:
        struct Block
        {
            D3D12MA::VirtualBlock* VirtualBlock = nullptr;
            Size Offset = 0;
        };

    public:
        struct Allocation
        {
        public:
            [[nodiscard]] bool IsValid() const noexcept { return VirtualAllocation.AllocHandle != 0 && AllocSize > 0 && BlockIndex != InvalidIndex; }
            static Allocation Invalid() { return Allocation{}; }

        public:
            Size Offset = 0;
            Size AllocSize = 0;
            Index BlockIndex = InvalidIndex;
            D3D12MA::VirtualAllocation VirtualAllocation{};
        };

    public:
        GpuStorage(RenderContext& renderContext, const String debugName, const Size initialNumElements, const bool bIsShaderReadWritable = false) :
            renderContext(renderContext),
            debugName(debugName),
            fence(renderContext.GetGpuDevice().CreateFence(debugName).value()),
            bIsShaderReadWritable(bIsShaderReadWritable),
            bufferSize(initialNumElements * sizeof(Ty))
        {
            IG_CHECK(initialNumElements > 0);
            GpuBufferDesc bufferDesc{};
            bufferDesc.DebugName = debugName;
            bufferDesc.AsStructuredBuffer<Ty>(initialNumElements, bIsShaderReadWritable);

            gpuBuffer = renderContext.CreateBuffer(bufferDesc);
            srv = renderContext.CreateShaderResourceView(gpuBuffer);

            if (bIsShaderReadWritable)
            {
                uav = renderContext.CreateUnorderedAccessView(gpuBuffer);
            }

            const D3D12MA::VIRTUAL_BLOCK_DESC blockDesc{ .Size = bufferSize };
            D3D12MA::VirtualBlock* newVirtualBlock{ nullptr };
            [[maybe_unused]] const HRESULT hr = D3D12MA::CreateVirtualBlock(&blockDesc, &newVirtualBlock);
            // 여기서 Virtual Block 할당 실패를 핸들 해야하나? 로그에 남겨야 하나?
            blocks.emplace_back(Block{ .VirtualBlock = newVirtualBlock, .Offset = 0 });
        }

        GpuStorage(const GpuStorage&) = delete;
        GpuStorage(GpuStorage&&) noexcept = delete;
        ~GpuStorage()
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

        GpuStorage& operator=(const GpuStorage&) = delete;
        GpuStorage& operator=(GpuStorage&&) noexcept = delete;

        Allocation Allocate(Size numElements)
        {
            IG_CHECK(numElements > 0);
            IG_CHECK(blocks.size() > 0);
            if (!gpuBuffer.IsNull())
            {
                return Allocation::Invalid();
            }

            const Size allocSize = numElements * sizeof(Ty);
            Allocation newAllocation{};
            for (Index blockIdx = 0; blockIdx < blocks.size(); ++blockIdx)
            {
                if (AllocateWithBlock(allocSize, blockIdx, newAllocation))
                {
                    return newAllocation;
                }
            }

            // Out Of Memory!
            IG_CHECK((allocSize + allocatedSize) > bufferSize);
            if (!Grow(allocSize))
            {
                return Allocation::Invalid();
            }

            const Index lastBlockIdx = blocks.size() - 1;
            if (!AllocateWithBlock(allocSize, lastBlockIdx, newAllocation))
            {
                return Allocation::Invalid();
            }

            return newAllocation;
        }

        void Deallocate(const Allocation& allocation)
        {
            IG_CHECK(allocation.IsValid());
            IG_CHECK(allocation.BlockIndex < blocks.size());
            Block& block = blocks[allocation.BlockIndex];
            IG_CHECK(block.VirtualBlock != nullptr);
            block.VirtualBlock->FreeAllocation(allocation.VirtualAllocation);
            IG_CHECK(allocatedSize >= allocation.AllocSize);
            allocatedSize -= allocation.AllocSize;
        }

        [[nodiscard]] Size GetAllocatedSize() const noexcept { return allocatedSize; }
        [[nodiscard]] Size GetBufferSize() const noexcept { return bufferSize; }
        [[nodiscard]] GpuFence& GetStorageFence() noexcept { return fence; }

        [[nodiscard]] RenderResource<GpuBuffer> GetGpuBuffer() const noexcept { return gpuBuffer; }
        [[nodiscard]] RenderResource<GpuView> GetShaderResourceView() const noexcept { return srv; }
        [[nodiscard]] RenderResource<GpuView> GetUnorderedResourceView() const noexcept { return uav; }

    private:
        bool AllocateWithBlock(const Size allocSize, const Index blockIdx, Allocation& allocation)
        {
            IG_CHECK(blockIdx < blocks.size());
            const D3D12MA::VIRTUAL_ALLOCATION_DESC allocDesc{ .Size = allocSize };
            D3D12MA::VirtualAllocation virtualAllocation{};
            Size allocOffset = 0;

            Block& block = blocks[blockIdx];
            IG_CHECK(block.VirtualBlock != nullptr);
            if (SUCCEEDED(block.VirtualBlock->Allocate(&allocDesc, &virtualAllocation, &allocOffset)))
            {
                allocatedSize += allocSize;
                allocation = Allocation
                {
                    .Offset = allocOffset,
                    .AllocSize = allocSize,
                    .BlockIndex = blockIdx,
                    .VirtualAllocation = virtualAllocation
                };

                return true;
            }

            return false;
        }

        bool Grow(const Size newAllocSize)
        {
            IG_CHECK(gpuBuffer);
            IG_CHECK(bufferSize > 0);
            IG_CHECK(newAllocSize > 0);
            IG_CHECK((newAllocSize % sizeof(Ty)) == 0);
            IG_CHECK((bufferSize % sizeof(Ty)) == 0);

            const Size newBufferSize = std::max(bufferSize * GrowthMultiplier, bufferSize + newAllocSize);
            const Size bufferSizeDiff = newBufferSize - bufferSize;
            IG_CHECK(newBufferSize > 0);

            GpuBufferDesc newGpuBufferDesc{};
            newGpuBufferDesc.DebugName = debugName;
            newGpuBufferDesc.AsStructuredBuffer<Ty>(newBufferSize / sizeof(Ty), bIsShaderReadWritable);
            RenderResource<GpuBuffer> newGpuBuffer = renderContext.CreateBuffer(newGpuBufferDesc);
            if (!newGpuBuffer)
            {
                return false;
            }

            GpuBuffer* gpuBufferPtr = renderContext.Lookup(gpuBuffer);
            GpuBuffer* newGpuBufferPtr = renderContext.Lookup(newGpuBuffer);
            IG_CHECK(gpuBufferPtr != nullptr);
            IG_CHECK(newGpuBufferPtr != nullptr);

            CommandQueue& asyncCopyQueue = renderContext.GetAsyncCopyQueue();
            CommandContextPool& contextPool = renderContext.GetAsyncCopyCommandContextPool();
            auto copyCmdCtx = contextPool.Request(FrameManager::GetLocalFrameIndex(), "GpuStorageGrowCopy"_fs);
            copyCmdCtx->Begin();
            {
                copyCmdCtx->CopyBuffer(*gpuBufferPtr, *newGpuBufferPtr);
            }
            copyCmdCtx->End();

            GpuSyncPoint newSyncPoint = fence.MakeSyncPoint();
            GpuSyncPoint prevSyncPoint = newSyncPoint.Prev();
            if (prevSyncPoint)
            {
                // 만약 버퍼에 대한 비동기 쓰기를 지원한다면 Write-After-Read Hazard 발생 가능성이 있음
                // 방지하기 위해선 GpuStorage가 별도의 Fence를 가지고 이를 사용해
                // ExecuteCommandLists 간의 명시적 동기화가 필요할것으로 보임
                asyncCopyQueue.SyncWith(prevSyncPoint);
            }

            ig::CommandContext* copyCmdCtxs[] = { (ig::CommandContext*)copyCmdCtx };
            asyncCopyQueue.ExecuteContexts(copyCmdCtxs);
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
            bufferSize = newBufferSize;

            // Storage Fence를 통해 적절한 통제만 해준다면, 굳이 WaitOnCpu를 해주지 않아도 될 것으로 예상
            newSyncPoint.WaitOnCpu();
            return true;
        }

    private:
        // 새롭게 할당 될 버퍼의 크기는 최소 (최소 버퍼 크기 + 할당 요청 크기)
        // NextBufferSize = max(previousBufferSize + NewAllocSize, previousBufferSize * GrowthMultiplier);
        // 내부적으로 fragmentation이 발생 할 가능성이 있음.
        // ex. 버퍼의 크기를 키운다고 해도, 할당 알고리즘 상 이전 버퍼 범위와 이후 추가된 버퍼 범위가
        // 물리적으로 연속적일지 언정, 논리적으로 불연속적인 별도의 공간으로 취급되기 때문이다.
        constexpr static Size GrowthMultiplier = 2;

        RenderContext& renderContext;

        String debugName;

        GpuFence fence;

        bool bIsShaderReadWritable = false;

        RenderResource<GpuBuffer> gpuBuffer;
        eastl::vector<Block> blocks;

        RenderResource<GpuView> srv;
        RenderResource<GpuView> uav;

        Size allocatedSize = 0;
        Size bufferSize = 0;

    };
}