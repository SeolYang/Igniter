#include <Render/GpuUploader.h>
#include <Core/ThreadUIDGenerator.h>

namespace fe
{
	GpuUploader::GpuUploader(RenderDevice& renderDevice)
		: renderDevice(renderDevice),
		  copyQueue(renderDevice.CreateCommandQueue("Gpu Uploader Copy Queue", EQueueType::Copy).value())
	{
		ResizeUnsafe(InitialBufferCapacity);
		for (size_t idx = 0; idx < RequestCapacity; ++idx)
		{
			uploadRequests[idx].Reset();
			uploadRequests[idx].CmdCtx = std::make_unique<CommandContext>(
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
		const static thread_local size_t tuid = ThreadUIDGenerator::GetUID();
		size_t expected = InvalidThreadID;
		while (!reservedThreadID.compare_exchange_weak(expected, tuid, std::memory_order::acq_rel))
		{
			expected = InvalidThreadID;
			std::this_thread::yield();
		}

		check(requestSize > 0);
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
		check(newRequest != nullptr);
		if (bufferHead + alignedRequestSize <= bufferCapacity)
		{
			newRequest->OffsetInBytes = bufferHead;
			newRequest->SizeInBytes = alignedRequestSize;
			newRequest->PaddingInBytes = 0;
			newRequest->Sync = {};

			bufferHead = (bufferUsedSizeInBytes + alignedRequestSize) % bufferCapacity;
			bufferUsedSizeInBytes += alignedRequestSize;
		}
		else
		{
			check(bufferHead < bufferCapacity);
			const size_t padding = bufferCapacity - bufferHead;

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
					check(bufferUsedSizeInBytes == 0);
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
					checkNoEntry();
				}
			}
		}

		check(newRequest != nullptr);
		check(bufferUsedSizeInBytes <= bufferCapacity);
		check(bufferHead < bufferCapacity);
		check(numInFlightRequests <= RequestCapacity);
		check(requestHead < RequestCapacity);
		check(bufferCpuAddr != nullptr);

		newRequest->CmdCtx->Begin();
		return UploadContext{ buffer.get(), bufferCpuAddr, newRequest };
	}

	std::optional<GpuSync> GpuUploader::Submit(UploadContext& context)
	{
		const static thread_local size_t tuid = ThreadUIDGenerator::GetUID();
		if (reservedThreadID.load(std::memory_order::acquire) != tuid)
		{
			checkNoEntry();
			return {};
		}

		if (!context.IsValid())
		{
			checkNoEntry();
			return {};
		}

		details::UploadRequest& request = context.GetRequest();
		request.CmdCtx->End();
		copyQueue.AddPendingContext(*request.CmdCtx);
		request.Sync = copyQueue.Submit();
		check(request.Sync.IsValid());
		context.Reset();
		reservedThreadID.store(InvalidThreadID, std::memory_order::release);
		return request.Sync;
	}

	details::UploadRequest* GpuUploader::AllocateRequestUnsafe()
	{
		check(numInFlightRequests <= RequestCapacity);
		if (numInFlightRequests == RequestCapacity)
		{
			return nullptr;
		}

		details::UploadRequest* newRequest = &uploadRequests[requestHead];
		requestHead = (requestHead + 1) % RequestCapacity;
		++numInFlightRequests;
		return newRequest;
	}

	void GpuUploader::WaitForRequestUnsafe(const size_t numWaitFor)
	{
		check(numWaitFor > 0 && numWaitFor <= numInFlightRequests);

		const size_t begin = requestTail;
		for (size_t counter = 0; counter < numWaitFor; ++counter)
		{
			const size_t idx = (begin + counter) % RequestCapacity;
			uploadRequests[idx].Sync.WaitOnCpu();

			check(numInFlightRequests > 0);
			--numInFlightRequests;
			requestTail = (requestTail + 1) % RequestCapacity;
			check((uploadRequests[idx].SizeInBytes + uploadRequests[idx].PaddingInBytes) <= bufferUsedSizeInBytes);
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
			GpuBufferDesc bufferDesc{};
			bufferDesc.AsUploadBuffer(static_cast<uint32_t>(alignedNewSize));
			bufferDesc.DebugName = UploadBufferName;
			buffer = std::make_unique<GpuBuffer>(renderDevice.CreateBuffer(bufferDesc).value());

			bufferCapacity = alignedNewSize;
			bufferHead = 0;
			bufferUsedSizeInBytes = 0;
			bufferCpuAddr = buffer->Map(0);

			requestHead = 0;
			numInFlightRequests = 0;
		}
		else
		{
			checkNoEntry();
		}
	}

	void GpuUploader::FlushQueue()
	{
		GpuSync copyQueueFlushSync = copyQueue.Flush();
		copyQueueFlushSync.WaitOnCpu();
	}

	void UploadContext::WriteData(const uint8_t* srcAddr, const size_t srcOffsetInBytes, const size_t destOffsetInBytes, const size_t writeSizeInBytes)
	{
		check(uploadBuffer != nullptr);
		check(offsettedCpuAddr != nullptr);
		check(request != nullptr);
		if (srcAddr != nullptr)
		{
			check(destOffsetInBytes + writeSizeInBytes <= request->SizeInBytes);
			std::memcpy(offsettedCpuAddr + destOffsetInBytes, srcAddr + srcOffsetInBytes, writeSizeInBytes);
		}
		else
		{
			checkNoEntry();
		}
	}

} // namespace fe