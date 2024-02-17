#pragma once
#include <D3D12/Common.h>
#include <Core/Misc.h>
#include <Core/Mutex.h>
#include <Core/Container.h>
#include <Core/FrameResource.h>

namespace fe::dx
{
	class Device;
	class GPUView
	{
	public:
		bool IsValid() const { return Index != std::numeric_limits<decltype(Index)>::max(); }
		operator bool() const { return IsValid(); }

		bool HasValidCPUHandle() const { return CPUHandle.ptr != std::numeric_limits<decltype(CPUHandle.ptr)>::max(); }
		bool HasValidGPUHandle() const { return GPUHandle.ptr != std::numeric_limits<decltype(GPUHandle.ptr)>::max(); }

	public:
		const EDescriptorType			  Type;		 /* Type of descriptor. */
		const uint32_t					  Index;	 /* Index of descriptor in Descriptor Heap. */
		const D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle; /* CPU Handle of descriptor. */
		const D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle; /* GPU Handle of descriptor. */

	};

	class DescriptorHeap
	{
		friend class Device;

	private:
		uint32_t AllocateIndex();
		void	 DeallocateIndex(const uint32_t index);

	public:
		DescriptorHeap(const DescriptorHeap&) = delete;
		DescriptorHeap(DescriptorHeap&& other) noexcept;
		~DescriptorHeap();

		DescriptorHeap& operator=(const DescriptorHeap&) = delete;
		DescriptorHeap& operator=(DescriptorHeap&&) noexcept = delete;

		bool IsValid() const { return descriptorHeap; }
		operator bool() const { return IsValid(); }

		ID3D12DescriptorHeap&		GetNative() { return *descriptorHeap.Get(); }
		EDescriptorHeapType			GetType() const { return descriptorHeapType; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetIndexedCPUDescriptorHandle(const uint32_t index) const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetIndexedGPUDescriptorHandle(const uint32_t index) const;

		FrameResource<GPUView> Request(FrameResourceManager& frameResourceManager, const EDescriptorType desiredType);

	private:
		DescriptorHeap(const EDescriptorHeapType newDescriptorHeapType, ComPtr<ID3D12DescriptorHeap> newDescriptorHeap,
					   const bool bIsShaderVisibleHeap, const uint32_t numDescriptorsInHeap,
					   const uint32_t descriptorHandleIncSizeInHeap);

		void Release(const uint32_t indexOfDescriptor);

	private:
		EDescriptorHeapType			 descriptorHeapType;
		ComPtr<ID3D12DescriptorHeap> descriptorHeap;
		uint32_t					 descriptorHandleIncrementSize = 0;
		uint32_t					 numInitialDescriptors = 0;

		bool						bIsShaderVisible = false;
		D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandleForHeapStart{};
		D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandleForHeapStart{};

		// #todo queue -> concurrent_queue
		RecursiveMutex		 mutex;
		std::queue<uint32_t> indexPool;
	};
} // namespace fe::dx