#pragma once
#include <D3D12/GpuBuffer.h>
#include <D3D12/CommandQueue.h>
#include <D3D12/CommandContext.h>

// 독립 Copy Queue 를 이용한 링 버퍼 기반 CPU->GPU 업로더

namespace fe
{
	class UploadRequest;
	class CircularUploader
	{
	public:
		UploadRequest RequestUpload(const size_t requestSize);

	private:
		void Resize(const size_t newSize); /* only newSize > currentSize */

	private:
		CommandQueue copyQueue;

		GpuBuffer buffer;
		size_t currentSize = 64 * 1024 * 1024; /* Initial Size = 64 MB */
		uint8_t* bufferPtr = nullptr;

		size_t currentHead = 0;
		size_t currentTail = 0;
	};

	class UploadRequest
	{
	public:
		// Copy operations ...
		// State Transitions ...

		// src != nullptr, srcOffset >= 0, sizeInBytes > 0
		bool WriteData(const uint8_t* src, const size_t srcOffset, const size_t sizeInBytes);
		size_t Submit();

	private:
		UploadRequest();

	private:
		CircularUploader* uploader = nullptr;
		CommandQueue* copyQueue = nullptr;
		uint8_t* uploadBufferPtr = nullptr; // CircularUploader::bufferPtr + begin
	};

} // namespace fe