#pragma once
#include <D3D12/Common.h>
#include <Core/Handle.h>
#include <Core/FrameManager.h>

namespace fe::dx
{
	class Device;
	class GPUBuffer;
	struct MappedGPUBuffer;
	class GPUBufferDesc;
	class GPUView;
	class GPUViewManager;
	class CommandContext;

	struct TempConstantBuffer
	{
	public:
		WeakHandle<MappedGPUBuffer> Mapping = {};
		WeakHandle<GPUView>			View = {};
	};

	class TempConstantBufferAllocator
	{
	public:
		TempConstantBufferAllocator(const FrameManager& frameManager, Device& renderDevice, HandleManager& handleManager, GPUViewManager& gpuViewManager, const uint32_t reservedBufferSizeInBytes = DefaultReservedBufferSizeInBytes);
		TempConstantBufferAllocator(const TempConstantBufferAllocator&) = delete;
		TempConstantBufferAllocator(TempConstantBufferAllocator&&) noexcept = delete;
		~TempConstantBufferAllocator();

		TempConstantBufferAllocator& operator=(const TempConstantBufferAllocator&) = delete;
		TempConstantBufferAllocator& operator=(TempConstantBufferAllocator&&) noexcept = delete;

		TempConstantBuffer Allocate(const GPUBufferDesc& desc);

		// 이전에, 현재 시작할 프레임(local frame)에서 할당된 모든 할당을 해제한다. 프레임 시작시 반드시 호출해야함.
		void DeallocateCurrentFrame();

		void InitBufferStateTransition(CommandContext& cmdCtx);

	public:
		// 실제 메모리 사용량을 프로파일링을 통해, 상황에 맞게 최적화된 값으로 설정하는 것이 좋다. (기본 값 == 64 MB)
		static constexpr uint32_t DefaultReservedBufferSizeInBytes = 67108864;

	private:
		const FrameManager& frameManager;
		Device&				renderDevice;
		HandleManager&		handleManager;
		GPUViewManager&		gpuViewManager;

		const uint32_t reservedSizeInBytesPerFrame;

		std::vector<dx::GPUBuffer>															  buffers;
		std::array<std::atomic_uint64_t, NumFramesInFlight>									  allocatedSizeInBytes;
		std::array<std::vector<UniqueHandle<MappedGPUBuffer, GPUBuffer*>>, NumFramesInFlight> mappedBuffers;
		std::array<std::vector<UniqueHandle<GPUView, GPUViewManager*>>, NumFramesInFlight>	  allocatedViews;
	};
} // namespace fe::dx