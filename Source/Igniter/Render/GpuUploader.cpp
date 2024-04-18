#include <Igniter.h>
#include <Core/ThreadUIDGenerator.h>
#include <D3D12/CommandQueue.h>
#include <D3D12/CommandContext.h>
#include <D3D12/GpuBuffer.h>
#include <D3D12/GpuTexture.h>
#include <D3D12/RenderDevice.h>
#include <Render/GpuUploader.h>

namespace ig
{
    GpuUploader::GpuUploader(RenderDevice& renderDevice, CommandQueue& asyncCopyQueue) :
        renderDevice(renderDevice)
        , asyncCopyQueue(asyncCopyQueue)
    {
        ResizeUnsafe(InitialBufferCapacity);
        for (size_t idx = 0; idx < RequestCapacity; ++idx)
        {
            uploadRequests[idx].Reset();
            uploadRequests[idx].CmdCtx = MakePtr<CommandContext>(
                renderDevice.CreateCommandContext(std::format("Gpu Uploader CmdCtx{}", idx), EQueueType::Copy).value());
        }
    }

    GpuUploader::~GpuUploader()
    {
        FlushQueue();
        if (buffer != nullptr)
        {
            buffer->Unmap();
        }
    }

    UploadContext GpuUploader::Reserve(const size_t requestSize)
    {
        const static thread_local size_t tuid     = ThreadUIDGenerator::GetUID();
        size_t                           expected = InvalidThreadID;
        while (!reservedThreadID.compare_exchange_weak(expected, tuid, std::memory_order::acq_rel))
        {
            expected = InvalidThreadID;
            std::this_thread::yield();
        }

        IG_CHECK(requestSize > 0);
        const size_t alignedRequestSize = AlignTo(requestSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

        if (alignedRequestSize > bufferCapacity)
        {
            if (numInFlightRequests >= 1)
            {
                WaitForRequestUnsafe(numInFlightRequests);
            }
            ResizeUnsafe(alignedRequestSize);
        }

        while ((bufferUsedSizeInBytes + alignedRequestSize > bufferCapacity) ||
            (numInFlightRequests == RequestCapacity))
        {
            WaitForRequestUnsafe(1);
        }

        details::UploadRequest* newRequest = AllocateRequestUnsafe();
        IG_CHECK(newRequest != nullptr);
        if (bufferHead + alignedRequestSize <= bufferCapacity)
        {
            newRequest->OffsetInBytes  = bufferHead;
            newRequest->SizeInBytes    = alignedRequestSize;
            newRequest->PaddingInBytes = 0;
            newRequest->Sync           = {};

            bufferHead = (bufferHead + alignedRequestSize) % bufferCapacity;
            bufferUsedSizeInBytes += alignedRequestSize;
        }
        else
        {
            IG_CHECK(bufferHead < bufferCapacity);
            const size_t padding = bufferCapacity - bufferHead;

            if ((bufferUsedSizeInBytes + padding + alignedRequestSize) <= bufferCapacity)
            {
                newRequest->OffsetInBytes  = 0;
                newRequest->SizeInBytes    = alignedRequestSize;
                newRequest->PaddingInBytes = padding;
                newRequest->Sync           = {};

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
                    newRequest->OffsetInBytes  = 0;
                    newRequest->SizeInBytes    = alignedRequestSize;
                    newRequest->PaddingInBytes = padding;
                    newRequest->Sync           = {};

                    bufferHead = alignedRequestSize;
                    bufferUsedSizeInBytes += (alignedRequestSize + padding);
                }
                else if (numInFlightRequests == 1)
                {
                    IG_CHECK(bufferUsedSizeInBytes == 0);
                    numInFlightRequests = 0;
                    requestHead         = 0;
                    requestTail         = 0;

                    newRequest                 = AllocateRequestUnsafe();
                    newRequest->OffsetInBytes  = 0;
                    newRequest->SizeInBytes    = alignedRequestSize;
                    newRequest->PaddingInBytes = 0;
                    newRequest->Sync           = {};

                    bufferHead            = alignedRequestSize;
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

        newRequest->CmdCtx->Begin();
        return UploadContext{buffer.get(), bufferCpuAddr, newRequest};
    }

    std::optional<GpuSync> GpuUploader::Submit(UploadContext& context)
    {
        const static thread_local size_t tuid = ThreadUIDGenerator::GetUID();
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
        request.CmdCtx->End();
        std::reference_wrapper<CommandContext> cmdCtxRef[] = {std::ref(*request.CmdCtx)};

        asyncCopyQueue.ExecuteContexts(cmdCtxRef);
        request.Sync = asyncCopyQueue.MakeSync();
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
        requestHead                        = (requestHead + 1) % RequestCapacity;
        ++numInFlightRequests;
        return newRequest;
    }

    void GpuUploader::WaitForRequestUnsafe(const size_t numWaitFor)
    {
        IG_CHECK(numWaitFor > 0 && numWaitFor <= numInFlightRequests);

        const size_t begin = requestTail;
        for (size_t counter = 0; counter < numWaitFor; ++counter)
        {
            const size_t idx = (begin + counter) % RequestCapacity;
            uploadRequests[idx].Sync.WaitOnCpu();

            IG_CHECK(numInFlightRequests > 0);
            --numInFlightRequests;
            requestTail = (requestTail + 1) % RequestCapacity;
            IG_CHECK((uploadRequests[idx].SizeInBytes + uploadRequests[idx].PaddingInBytes) <= bufferUsedSizeInBytes);
            bufferUsedSizeInBytes -= (uploadRequests[idx].SizeInBytes + uploadRequests[idx].PaddingInBytes);
            uploadRequests[idx].Reset();
        }
    }

    void GpuUploader::ResizeUnsafe(const size_t newSize)
    {
        const size_t alignedNewSize = AlignTo(newSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
        if (alignedNewSize > bufferCapacity)
        {
            FlushQueue();
            if (bufferCpuAddr != nullptr)
            {
                buffer->Unmap();
                bufferCpuAddr = nullptr;
            }

            static const auto UploadBufferName = String("Async Upload Buffer");
            GpuBufferDesc     bufferDesc{};
            bufferDesc.AsUploadBuffer(static_cast<uint32_t>(alignedNewSize));
            bufferDesc.DebugName = UploadBufferName;
            buffer               = MakePtr<GpuBuffer>(renderDevice.CreateBuffer(bufferDesc).value());

            bufferCapacity        = alignedNewSize;
            bufferHead            = 0;
            bufferUsedSizeInBytes = 0;
            bufferCpuAddr         = buffer->Map(0);

            requestHead         = 0;
            numInFlightRequests = 0;
        }
        else
        {
            IG_CHECK_NO_ENTRY();
        }
    }

    void GpuUploader::FlushQueue()
    {
        GpuSync copyQueueFlushSync = asyncCopyQueue.MakeSync();
        copyQueueFlushSync.WaitOnCpu();
    }

    void UploadContext::WriteData(const uint8_t* srcAddr, const size_t srcOffsetInBytes, const size_t destOffsetInBytes,
                                  const size_t   writeSizeInBytes)
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

    void UploadContext::CopyBuffer(const size_t srcOffsetInBytes, const size_t numBytes, GpuBuffer& dst,
                                   const size_t dstOffsetInBytes /*= 0*/)
    {
        if (!IsValid())
        {
            IG_CHECK_NO_ENTRY();
            return;
        }

        IG_CHECK((request->OffsetInBytes + srcOffsetInBytes + numBytes) <= uploadBuffer->GetDesc().GetSizeAsBytes());
        IG_CHECK((dstOffsetInBytes + numBytes) <= dst.GetDesc().GetSizeAsBytes());
        IG_CHECK(request->CmdCtx != nullptr);
        CommandContext& cmdCtx = *request->CmdCtx;
        cmdCtx.CopyBuffer(*uploadBuffer, request->OffsetInBytes + srcOffsetInBytes, numBytes, dst, dstOffsetInBytes);
    }

    void UploadContext::CopyTextureRegion(const size_t srcOffsetInBytes, GpuTexture& dst, const uint32_t subresourceIdx,
                                          const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& layout)
    {
        if (!IsValid())
        {
            IG_CHECK_NO_ENTRY();
            return;
        }

        IG_CHECK(request->CmdCtx != nullptr);
        CommandContext& cmdCtx = *request->CmdCtx;
        cmdCtx.CopyTextureRegion(*uploadBuffer, request->OffsetInBytes + srcOffsetInBytes, dst, subresourceIdx, layout);
    }

    void UploadContext::CopyTextureSimple(GpuTexture& dst, const GpuCopyableFootprints& dstCopyableFootprints,
                                          const std::span<const D3D12_SUBRESOURCE_DATA> subresources)
    {
        /* Write subresources to upload buffer */
        for (uint32_t idx = 0; idx < subresources.size(); ++idx)
        {
            const D3D12_SUBRESOURCE_DATA&      srcSubresource  = subresources[idx];
            const D3D12_SUBRESOURCE_FOOTPRINT& dstFootprint    = dstCopyableFootprints.Layouts[idx].Footprint;
            const size_t                       rowSizesInBytes = dstCopyableFootprints.RowSizesInBytes[idx];
            for (uint32_t z = 0; z < dstFootprint.Depth; ++z)
            {
                const size_t dstSlicePitch = static_cast<size_t>(dstFootprint.RowPitch) * dstCopyableFootprints.NumRows[
                    idx];
                const size_t dstSliceOffset = dstSlicePitch * z;
                const size_t srcSliceOffset = srcSubresource.SlicePitch * z;
                for (uint32_t y = 0; y < dstCopyableFootprints.NumRows[idx]; ++y)
                {
                    const size_t dstRowOffset = static_cast<size_t>(dstFootprint.RowPitch) * y;
                    const size_t srcRowOffset = srcSubresource.RowPitch * y;

                    const size_t dstOffset = dstCopyableFootprints.Layouts[idx].Offset + dstSliceOffset + dstRowOffset;
                    const size_t srcOffset = srcSliceOffset + srcRowOffset;
                    WriteData(reinterpret_cast<const uint8_t*>(srcSubresource.pData),
                              srcOffset, dstOffset,
                              rowSizesInBytes);
                }
            }

            CopyTextureRegion(0, dst, idx, dstCopyableFootprints.Layouts[idx]);
        }
    }
} // namespace ig
