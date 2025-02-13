#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/D3D12/GpuSyncPoint.h"
#include "Igniter/Render/Common.h"

namespace ig
{
    class CommandQueue;
    class CommandList;
    class GpuTexture;
    class GpuBuffer;
    class GpuFence;

    namespace details
    {
        struct UploadRequest final
        {
          public:
            void Reset()
            {
                OffsetInBytes = 0;
                SizeInBytes = 0;
                PaddingInBytes = 0;
                Sync = {};
            }

          public:
            std::unique_ptr<CommandList> CmdList;
            Size OffsetInBytes = 0;
            Size SizeInBytes = 0;
            Size PaddingInBytes = 0;
            GpuSyncPoint Sync{};
        };
    } // namespace details

    class UploadContext final
    {
        friend class GpuUploader;

      public:
        UploadContext(const UploadContext&) = delete;

        UploadContext(UploadContext&& other) noexcept
            : uploadBuffer(std::exchange(other.uploadBuffer, nullptr))
            , offsettedCpuAddr(std::exchange(other.offsettedCpuAddr, nullptr))
            , request(std::exchange(other.request, nullptr))
        {
        }

        ~UploadContext() = default;

        UploadContext& operator=(const UploadContext&) = delete;
        UploadContext& operator=(UploadContext&& other) noexcept = delete;

        bool IsValid() const { return (uploadBuffer != nullptr) && (offsettedCpuAddr != nullptr) && (request != nullptr); }

        Size GetSize() const { return request != nullptr ? request->SizeInBytes : 0; }
        Size GetOffset() const { return request != nullptr ? request->OffsetInBytes : 0; }

        uint8_t* GetOffsettedCpuAddress()
        {
            IG_CHECK(offsettedCpuAddr != nullptr);
            return offsettedCpuAddr;
        }

        void WriteData(const uint8_t* srcAddr, const Size srcOffsetInBytes, const Size destOffsetInBytes, const Size writeSizeInBytes);

        void CopyBuffer(const Size srcOffsetInBytes, const Size numBytes, GpuBuffer& dst, const Size dstOffsetInBytes = 0);
        void CopyTextureRegion(
            const Size srcOffsetInBytes, GpuTexture& dst, const U32 subresourceIdx, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& layout);
        void CopyTextureSimple(
            GpuTexture& dst, const GpuCopyableFootprints& dstCopyableFootprints, const std::span<const D3D12_SUBRESOURCE_DATA> subresources);

      private:
        UploadContext();

        UploadContext(GpuBuffer* uploadBuffer, uint8_t* uploadBufferCpuAddr, details::UploadRequest* request)
            : uploadBuffer(uploadBuffer)
            , offsettedCpuAddr(uploadBufferCpuAddr + request->OffsetInBytes)
            , request(request)
        {
            IG_CHECK(uploadBuffer != nullptr);
            IG_CHECK(uploadBufferCpuAddr != nullptr);
            IG_CHECK(request != nullptr);
        }

        details::UploadRequest& GetRequest() { return *request; }

        void Reset()
        {
            uploadBuffer = nullptr;
            offsettedCpuAddr = nullptr;
            request = nullptr;
        }

      private:
        GpuBuffer* uploadBuffer = nullptr;
        uint8_t* offsettedCpuAddr = nullptr;
        details::UploadRequest* request = nullptr;
    };

    // Reserve Upload -> setup data & fill up copy cmd ctx -> Submit Upload -> Submit to Async Copy Queue
    class CommandQueue;
    class GpuUploader final
    {
      public:
        GpuUploader(GpuDevice& gpuDevice);
        GpuUploader(const GpuUploader&) = delete;
        GpuUploader(GpuUploader&&) noexcept = delete;
        ~GpuUploader();

        GpuUploader& operator=(const GpuUploader&) = delete;
        GpuUploader& operator=(GpuUploader&&) noexcept = delete;

        [[nodiscard]] UploadContext Reserve(const Size requestSize);
        std::optional<GpuSyncPoint> Submit(UploadContext& context);

        void PreRender(const LocalFrameIndex localFrameIdx) { this->currentLocalFrameIdx = localFrameIdx; }

      private:
        details::UploadRequest* AllocateRequestUnsafe();
        void WaitForRequestUnsafe(const Size numWaitFor);
        void ResizeUnsafe(const Size newSize);
        void FlushQueue();

      private:
        GpuDevice& gpuDevice;
        Ptr<CommandQueue> copyQueue{};

        LocalFrameIndex currentLocalFrameIdx;

        constexpr static uint64_t InvalidThreadID = std::numeric_limits<uint64_t>::max();
        std::atomic_uint64_t reservedThreadID = InvalidThreadID;

        constexpr static Size InitialBufferCapacity = 64 * 1024 * 1024; /* Initial Size = 64 MB */
        Size bufferCapacity = 0;
        std::unique_ptr<GpuBuffer> buffer;
        uint8_t* bufferCpuAddr = nullptr;
        Size bufferHead = 0;
        Size bufferUsedSizeInBytes = 0;

        constexpr static Size RequestCapacity = 16;
        details::UploadRequest uploadRequests[RequestCapacity];
        Size requestHead = 0;
        Size requestTail = 0;
        Size numInFlightRequests = 0;
    };
} // namespace ig
