#pragma once
#include <D3D12/GpuBuffer.h>
#include <D3D12/GpuBufferDesc.h>
#include <D3D12/CommandQueue.h>
#include <D3D12/CommandContext.h>
#include <D3D12/RenderDevice.h>
#include <Core/Mutex.h>

namespace fe
{
	namespace details
	{
		struct UploadRequest
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
			std::unique_ptr<CommandContext> CmdCtx;
			size_t OffsetInBytes = 0;
			size_t SizeInBytes = 0;
			size_t PaddingInBytes = 0;
			GpuSync Sync{};
		};
	} // namespace details

	class UploadContext final
	{
		friend class GpuUploader;

	public:
		UploadContext(const UploadContext&) = delete;
		UploadContext(UploadContext&& other) noexcept
			: uploadBuffer(std::exchange(other.uploadBuffer, nullptr)),
			  offsettedCpuAddr(std::exchange(other.offsettedCpuAddr, nullptr)),
			  request(std::exchange(other.request, nullptr))
		{
		}
		~UploadContext() = default;

		UploadContext& operator=(const UploadContext&) = delete;
		UploadContext& operator=(UploadContext&& other) noexcept = delete;

		bool IsValid() const { return (uploadBuffer != nullptr) && (offsettedCpuAddr != nullptr) && (request != nullptr); }

		size_t GetSize() const { return request != nullptr ? request->SizeInBytes : 0; }
		size_t GetOffset() const { return request != nullptr ? request->OffsetInBytes : 0; }
		GpuBuffer& GetBuffer() { return *uploadBuffer; }
		void WriteData(const uint8_t* srcAddr, const size_t srcOffsetInBytes, const size_t destOffsetInBytes, const size_t writeSizeInBytes);

		CommandContext* operator->()
		{
			check(request != nullptr && request->CmdCtx != nullptr);
			return request->CmdCtx.get();
		}

		CommandContext& operator*()
		{
			check(request != nullptr && request->CmdCtx != nullptr);
			return *request->CmdCtx;
		}

	private:
		UploadContext();
		UploadContext(GpuBuffer* uploadBuffer, uint8_t* uploadBufferCpuAddr, details::UploadRequest* request)
			: uploadBuffer(uploadBuffer),
			  offsettedCpuAddr(uploadBufferCpuAddr + request->OffsetInBytes),
			  request(request)
		{
			check(uploadBuffer != nullptr);
			check(uploadBufferCpuAddr != nullptr);
			check(request != nullptr);
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
	class GpuUploader
	{
	public:
		GpuUploader(RenderDevice& renderDevice);
		GpuUploader(const GpuUploader&) = delete;
		GpuUploader(GpuUploader&&) noexcept = delete;
		~GpuUploader();

		GpuUploader& operator=(const GpuUploader&) = delete;
		GpuUploader& operator=(GpuUploader&&) noexcept = delete;

		[[nodiscard]] UploadContext Reserve(const size_t requestSize);

		std::optional<GpuSync> Submit(UploadContext& context);

	private:
		details::UploadRequest* AllocateRequestUnsafe();

		void WaitForRequestUnsafe(const size_t numWaitFor);

		void ResizeUnsafe(const size_t newSize);

		void FlushQueue();

	private:
		RenderDevice& renderDevice;

		constexpr static uint64_t InvalidThreadID = std::numeric_limits<uint64_t>::max();
		std::atomic_uint64_t reservedThreadID = InvalidThreadID;

		CommandQueue copyQueue;

		constexpr static size_t InitialBufferCapacity = 64 * 1024 * 1024; /* Initial Size = 64 MB */
		size_t bufferCapacity = 0;
		std::unique_ptr<GpuBuffer> buffer;
		uint8_t* bufferCpuAddr = nullptr;
		size_t bufferHead = 0;
		size_t bufferUsedSizeInBytes = 0;

		constexpr static size_t RequestCapacity = 2;
		details::UploadRequest uploadRequests[RequestCapacity];
		size_t requestHead = 0;
		size_t numInFlightRequests = 0;
	};

} // namespace fe