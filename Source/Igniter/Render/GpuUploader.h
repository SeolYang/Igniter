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
        {}

        ~UploadContext()
        {
            IG_CHECK(!IsValid());
        }

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
        GpuUploader(const String name, GpuDevice& gpuDevice, CommandQueue& copyQueue, const Size initRingBufferSize);
        GpuUploader(const GpuUploader&) = delete;
        GpuUploader(GpuUploader&&) noexcept = delete;
        ~GpuUploader();

        GpuUploader& operator=(const GpuUploader&) = delete;
        GpuUploader& operator=(GpuUploader&&) noexcept = delete;

        /*
         * 1. 요청한 업로드 크기가 현재 링 버퍼의 크기보다 큰 경우, 현재 진행 중인 모든 업로드 요청이 완료 될 때 까지
         * 대기 한다. 
         * 2. 업로드 요청이 꽉 차있는 경우, 최소 1개의 요청 작업이 완료 될 때 까지 대기한다(GPU에서의 작업 완료).
         * 3. 요청한 업로드 크기가 현재 링 버퍼의 크기 보단 작지만, 현재 처리중인 워크 로드로 인해 처리 할 수 없는 경우
         *  요청 크기에 맞는 연속적인 메모리 공간이 확보 될 때 까지 대기한다.
         * 4. Reserve와 Submit은 스레드 안전하다(thread-safe).
         * 5. 스레드 안전성을 보장하기 위해서, 요청한 스레드의 Submit~Reserve 호출 순서는 직렬화 된다.
         *  예를 들어, A라는 스레드에서 Reserve를 호출하면 스레드 'A'가 Submit을 호출 하기 전 까지.
         *  다른 스레드는 새로운 업로드를 예약 할 수 없다. 즉 다른 스레드는 Reserve에서 대기(wait)하게 된다.
         *  이 때, 스레드 'A'가 Submit 하지 않는다면 다른 스레드는 데드락 상태에 빠질 수 있다.
         *  (UploadContext는 Debug Build에서 Assertion을 통해 해당 동작을 감지하기 위한 최소한의 시도를 보장 한다).
         * 6. UploadContext 내부의 맵핑된 버퍼 주소나 자원에 접근 하는 것은 정의되지 않은 행동(Undefined Behaviour)이다.
         */
        [[nodiscard]] UploadContext Reserve(const Size requestSize);
        GpuSyncPoint Submit(UploadContext& context);

        void PreRender(const LocalFrameIndex localFrameIdx) { this->currentLocalFrameIdx = localFrameIdx; }

    private:
        details::UploadRequest* AllocateRequestUnsafe();
        void WaitForRequestUnsafe(const Size numWaitFor);
        void ResizeUnsafe(const Size newSize);
        void FlushQueue();

    private:
        String name = "UnnamedGpuUploader"_fs;
        GpuDevice* gpuDevice = nullptr;
        CommandQueue* copyQueue = nullptr;

        LocalFrameIndex currentLocalFrameIdx;

        constexpr static uint64_t InvalidThreadID = std::numeric_limits<uint64_t>::max();
        std::atomic_uint64_t reservedThreadID = InvalidThreadID;

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
