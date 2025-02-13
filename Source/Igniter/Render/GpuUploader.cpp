#include "Igniter/Igniter.h"
#include "Igniter/Core/Thread.h"
#include "Igniter/D3D12/CommandQueue.h"
#include "Igniter/D3D12/CommandList.h"
#include "Igniter/D3D12/GpuBuffer.h"
#include "Igniter/D3D12/GpuDevice.h"
#include "Igniter/Render/GpuUploader.h"

namespace ig
{
    GpuUploader::GpuUploader(const String name, GpuDevice& gpuDevice, CommandQueue& copyQueue, const Size initRingBufferSize)
        : name(name)
        , gpuDevice(&gpuDevice)
        , copyQueue(&copyQueue)
    {
        IG_CHECK(copyQueue.GetType() == EQueueType::Copy);
        IG_CHECK(initRingBufferSize > 0);
        ResizeUnsafe(initRingBufferSize);
        for (Size idx = 0; idx < RequestCapacity; ++idx)
        {
            uploadRequests[idx].Reset();
            uploadRequests[idx].CmdList =
                MakePtr<CommandList>(gpuDevice.CreateCommandList(
                    std::format("{}.CmdList ({})", name, idx), EQueueType::Copy).value());
        }
    }

    GpuUploader::~GpuUploader()
    {
        if (buffer != nullptr)
        {
            buffer->Unmap();
        }
    }

    UploadContext GpuUploader::Reserve(const Size requestSize)
    {
        const static thread_local Size tuid = ThreadUIDGenerator::GetUID();
        Size expected = InvalidThreadID;
        while (!reservedThreadID.compare_exchange_weak(expected, tuid, std::memory_order::acq_rel))
        {
            expected = InvalidThreadID;
            std::this_thread::yield();
        }

        IG_CHECK(requestSize > 0);
        const Size alignedRequestSize = AlignTo(requestSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

        if (alignedRequestSize > bufferCapacity)
        {
            if (numInFlightRequests >= 1)
            {
                WaitForRequestUnsafe(numInFlightRequests);
            }
            ResizeUnsafe(alignedRequestSize);
        }

        while ((bufferUsedSizeInBytes + alignedRequestSize > bufferCapacity) || (numInFlightRequests == RequestCapacity))
        {
            WaitForRequestUnsafe(1);
        }

        details::UploadRequest* newRequest = AllocateRequestUnsafe();
        IG_CHECK(newRequest != nullptr);
        if (bufferHead + alignedRequestSize <= bufferCapacity)
        {
            newRequest->OffsetInBytes = bufferHead;
            newRequest->SizeInBytes = alignedRequestSize;
            newRequest->PaddingInBytes = 0;
            newRequest->Sync = {};

            bufferHead = (bufferHead + alignedRequestSize) % bufferCapacity;
            bufferUsedSizeInBytes += alignedRequestSize;
        }
        else
        {
            IG_CHECK(bufferHead < bufferCapacity);
            const Size padding = bufferCapacity - bufferHead;

            if ((bufferUsedSizeInBytes + padding + alignedRequestSize) <= bufferCapacity)
            {
                newRequest->OffsetInBytes = 0;
                newRequest->SizeInBytes = alignedRequestSize;
                newRequest->PaddingInBytes = padding;
                newRequest->Sync = {};

                bufferHead = alignedRequestSize;
                bufferUsedSizeInBytes += (alignedRequestSize + padding);
            }
            else
            {
                while ((bufferUsedSizeInBytes + padding + alignedRequestSize) > bufferCapacity)
                {
                    WaitForRequestUnsafe(1);
                }

                if (numInFlightRequests > 1)
                {
                    newRequest->OffsetInBytes = 0;
                    newRequest->SizeInBytes = alignedRequestSize;
                    newRequest->PaddingInBytes = padding;
                    newRequest->Sync = {};

                    bufferHead = alignedRequestSize;
                    bufferUsedSizeInBytes += (alignedRequestSize + padding);
                }
                else if (numInFlightRequests == 1)
                {
                    IG_CHECK(bufferUsedSizeInBytes == 0);
                    numInFlightRequests = 0;
                    requestHead = 0;
                    requestTail = 0;

                    newRequest = AllocateRequestUnsafe();
                    newRequest->OffsetInBytes = 0;
                    newRequest->SizeInBytes = alignedRequestSize;
                    newRequest->PaddingInBytes = 0;
                    newRequest->Sync = {};

                    bufferHead = alignedRequestSize;
                    bufferUsedSizeInBytes = alignedRequestSize;
                }
                else
                {
                    IG_CHECK_NO_ENTRY();
                }
            }
        }

        IG_CHECK(newRequest != nullptr);
        IG_CHECK(bufferUsedSizeInBytes <= bufferCapacity);
        IG_CHECK(bufferHead < bufferCapacity);
        IG_CHECK(numInFlightRequests <= RequestCapacity);
        IG_CHECK(requestHead < RequestCapacity);
        IG_CHECK(bufferCpuAddr != nullptr);

        newRequest->CmdList->Open();
        return UploadContext{buffer.get(), bufferCpuAddr, newRequest};
    }

    GpuSyncPoint GpuUploader::Submit(UploadContext& context)
    {
        const static thread_local Size tuid = ThreadUIDGenerator::GetUID();
        if (reservedThreadID.load(std::memory_order::acquire) != tuid)
        {
            IG_CHECK_NO_ENTRY();
            return {};
        }

        if (!context.IsValid())
        {
            IG_CHECK_NO_ENTRY();
            return {};
        }

        details::UploadRequest& request = context.GetRequest();
        request.CmdList->Close();

        CommandList* cmdListPtrs[] = {request.CmdList.get()};
        copyQueue->ExecuteCommandLists(cmdListPtrs);
        request.Sync = copyQueue->MakeSyncPointWithSignal();
        IG_CHECK(request.Sync.IsValid());
        context.Reset();
        reservedThreadID.store(InvalidThreadID, std::memory_order::release);
        return request.Sync;
    }

    details::UploadRequest* GpuUploader::AllocateRequestUnsafe()
    {
        IG_CHECK(numInFlightRequests <= RequestCapacity);
        if (numInFlightRequests == RequestCapacity)
        {
            return nullptr;
        }

        details::UploadRequest* newRequest = &uploadRequests[requestHead];
        requestHead = (requestHead + 1) % RequestCapacity;
        ++numInFlightRequests;
        return newRequest;
    }

    void GpuUploader::WaitForRequestUnsafe(const Size numWaitFor)
    {
        IG_CHECK(numWaitFor > 0 && numWaitFor <= numInFlightRequests);

        const Size begin = requestTail;
        for (Size counter = 0; counter < numWaitFor; ++counter)
        {
            const Size idx = (begin + counter) % RequestCapacity;
            uploadRequests[idx].Sync.WaitOnCpu();

            IG_CHECK(numInFlightRequests > 0);
            --numInFlightRequests;
            requestTail = (requestTail + 1) % RequestCapacity;
            IG_CHECK((uploadRequests[idx].SizeInBytes + uploadRequests[idx].PaddingInBytes) <= bufferUsedSizeInBytes);
            bufferUsedSizeInBytes -= (uploadRequests[idx].SizeInBytes + uploadRequests[idx].PaddingInBytes);
            uploadRequests[idx].Reset();
        }
    }

    void GpuUploader::ResizeUnsafe(const Size newSize)
    {
        const Size alignedNewSize = AlignTo(newSize + newSize / 2, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
        if (alignedNewSize > bufferCapacity)
        {
            FlushQueue();
            if (bufferCpuAddr != nullptr)
            {
                buffer->Unmap();
                bufferCpuAddr = nullptr;
            }

            GpuBufferDesc bufferDesc{};
            bufferDesc.AsUploadBuffer(static_cast<U32>(alignedNewSize));
            bufferDesc.DebugName = String(std::format("{}.Buffer", name));
            buffer = MakePtr<GpuBuffer>(gpuDevice->CreateBuffer(bufferDesc).value());

            bufferCapacity = alignedNewSize;
            bufferHead = 0;
            bufferUsedSizeInBytes = 0;
            bufferCpuAddr = buffer->Map(0);

            requestHead = 0;
            numInFlightRequests = 0;
        }
        else
        {
            IG_CHECK_NO_ENTRY();
        }
    }

    void GpuUploader::FlushQueue()
    {
        copyQueue->MakeSyncPointWithSignal().WaitOnCpu();
    }

    void UploadContext::WriteData(
        const uint8_t* srcAddr, const Size srcOffsetInBytes, const Size destOffsetInBytes, const Size writeSizeInBytes)
    {
        IG_CHECK(uploadBuffer != nullptr);
        IG_CHECK(offsettedCpuAddr != nullptr);
        IG_CHECK(request != nullptr);
        if (srcAddr != nullptr)
        {
            IG_CHECK(destOffsetInBytes + writeSizeInBytes <= request->SizeInBytes);
            std::memcpy(offsettedCpuAddr + destOffsetInBytes, srcAddr + srcOffsetInBytes, writeSizeInBytes);
        }
        else
        {
            IG_CHECK_NO_ENTRY();
        }
    }

    void UploadContext::CopyBuffer(const Size srcOffsetInBytes, const Size numBytes, GpuBuffer& dst, const Size dstOffsetInBytes /*= 0*/)
    {
        if (!IsValid())
        {
            IG_CHECK_NO_ENTRY();
            return;
        }

        IG_CHECK((request->OffsetInBytes + srcOffsetInBytes + numBytes) <= uploadBuffer->GetDesc().GetSizeAsBytes());
        IG_CHECK((dstOffsetInBytes + numBytes) <= dst.GetDesc().GetSizeAsBytes());
        IG_CHECK(request->CmdList != nullptr);
        CommandList& cmdList = *request->CmdList;
        cmdList.CopyBuffer(*uploadBuffer, request->OffsetInBytes + srcOffsetInBytes, numBytes, dst, dstOffsetInBytes);
    }

    void UploadContext::CopyTextureRegion(
        const Size srcOffsetInBytes, GpuTexture& dst, const U32 subresourceIdx, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& layout)
    {
        if (!IsValid())
        {
            IG_CHECK_NO_ENTRY();
            return;
        }

        IG_CHECK(request->CmdList != nullptr);
        CommandList& cmdList = *request->CmdList;
        cmdList.CopyTextureRegion(*uploadBuffer, request->OffsetInBytes + srcOffsetInBytes, dst, subresourceIdx, layout);
    }

    void UploadContext::CopyTextureSimple(
        GpuTexture& dst, const GpuCopyableFootprints& dstCopyableFootprints, const std::span<const D3D12_SUBRESOURCE_DATA> subresources)
    {
        /* Write subresources to upload buffer */
        for (U32 idx = 0; idx < subresources.size(); ++idx)
        {
            const D3D12_SUBRESOURCE_DATA& srcSubresource = subresources[idx];
            const D3D12_SUBRESOURCE_FOOTPRINT& dstFootprint = dstCopyableFootprints.Layouts[idx].Footprint;
            const Size rowSizesInBytes = dstCopyableFootprints.RowSizesInBytes[idx];
            for (U32 z = 0; z < dstFootprint.Depth; ++z)
            {
                const Size dstSlicePitch = static_cast<Size>(dstFootprint.RowPitch) * dstCopyableFootprints.NumRows[idx];
                const Size dstSliceOffset = dstSlicePitch * z;
                const Size srcSliceOffset = srcSubresource.SlicePitch * z;
                for (U32 y = 0; y < dstCopyableFootprints.NumRows[idx]; ++y)
                {
                    const Size dstRowOffset = static_cast<Size>(dstFootprint.RowPitch) * y;
                    const Size srcRowOffset = srcSubresource.RowPitch * y;

                    const Size dstOffset = dstCopyableFootprints.Layouts[idx].Offset + dstSliceOffset + dstRowOffset;
                    const Size srcOffset = srcSliceOffset + srcRowOffset;
                    WriteData(static_cast<const uint8_t*>(srcSubresource.pData), srcOffset, dstOffset, rowSizesInBytes);
                }
            }

            CopyTextureRegion(0, dst, idx, dstCopyableFootprints.Layouts[idx]);
        }
    }
} // namespace ig
