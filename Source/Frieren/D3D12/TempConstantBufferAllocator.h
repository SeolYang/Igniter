#pragma once
#include <D3D12/Common.h>
#include <Core/Handle.h>
#include <Core/FrameManager.h>

namespace fe::dx
{
	class Device;
	class GPUBuffer;
	class GPUBufferDesc;
	class GPUView;
	class GPUViewManager;

	// #wip
	struct TempConstantBuffer
	{
	public:
		WeakHandle<GPUResourceMapGuard> Mapping = {};
		WeakHandle<GPUView>				View = {};
	};

	class TempConstantBufferAllocator
	{
	public:
		// FrameConstantBufferAllocator(const FrameManager& frameManager, Device& renderDevice);
		TempConstantBufferAllocator(const FrameManager& frameManager, Device& renderDevice, HandleManager& handleManager, GPUViewManager& gpuViewManager, const uint32_t reservedBufferSizeInBytes = DefaultReservedBufferSizeInBytes);
		TempConstantBufferAllocator(const TempConstantBufferAllocator&) = delete;
		TempConstantBufferAllocator(TempConstantBufferAllocator&&) noexcept = delete;
		~TempConstantBufferAllocator();

		TempConstantBufferAllocator& operator=(const TempConstantBufferAllocator&) = delete;
		TempConstantBufferAllocator& operator=(TempConstantBufferAllocator&&) noexcept = delete;

		// type == constant buffer && size%256 == 0 && size <= 1024)
		TempConstantBuffer Allocate(const GPUBufferDesc& desc);

		// 이전에, 현재 시작할 프레임(local frame)에서 할당된 모든 할당을 해제한다.
		void DeallocateCurrentFrame();

	public:
		// 실제 메모리 사용량을 프로파일링을 통해, 상황에 맞게 최적화된 값으로 설정하는 것이 좋음.
		static constexpr uint32_t DefaultReservedBufferSizeInBytes = 4096;

	private:
		const FrameManager& frameManager;
		Device&				renderDevice;
		HandleManager&		handleManager;
		GPUViewManager&		gpuViewManager;

		const uint32_t reservedSizeInBytesPerFrame;

		std::vector<dx::GPUBuffer>														   buffers;
		std::array<std::atomic_uint64_t, NumFramesInFlight>								   allocatedSizeInBytes;
		std::array<std::vector<UniqueHandle<GPUResourceMapGuard>>, NumFramesInFlight>	   mapGuards;
		std::array<std::vector<UniqueHandle<GPUView, GPUViewManager*>>, NumFramesInFlight> allocatedViews;
	};
} // namespace fe::dx