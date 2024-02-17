#pragma once
#include <D3D12/Common.h>
#include <Core/Misc.h>
#include <Core/Mutex.h>
#include <Core/Container.h>

namespace fe::dx
{
	class Device;
	class DescriptorHeap;
	class Descriptor
	{
		friend class DescriptorHeap;

	public:
		Descriptor(Descriptor&& other) noexcept;
		~Descriptor();

		Descriptor& operator=(Descriptor&& other) noexcept;

		bool IsValid() const { return ownedDescriptorHeap != nullptr && index != InvalidIndexU32; }
		operator bool() const { return IsValid(); }

		uint32_t					GetIndex() const { return index; }
		EDescriptorType				GetType() const { return type; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle() const { return cpuHandle; }
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle() const { return gpuHandle; }

	private:
		Descriptor(DescriptorHeap& owner, const EDescriptorType descriptorType, const uint32_t allocatedIndex,
				   const D3D12_CPU_DESCRIPTOR_HANDLE indexedCpuHandle,
				   const D3D12_GPU_DESCRIPTOR_HANDLE indexedGpuHandle);

	public:
		static constexpr uint32_t InvalidIndex = InvalidIndexU32;

	private:
		DescriptorHeap*				ownedDescriptorHeap = nullptr;
		EDescriptorType				type;
		uint32_t					index;
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
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
		
		// #todo Descriptor의 FrameResource 화?
		// ex using ResourceView = FrameResource<Descriptor>?
		// MakeFrameResource<Descriptor>(new Descriptor(...), [](...){...{ descriptorHeap.Deallocate(idx); delete ptr; }}
		Descriptor Allocate(const EDescriptorType requestDescriptorType);
		void	   Deallocate(Descriptor& descriptor);

	private:
		DescriptorHeap(const EDescriptorHeapType newDescriptorHeapType, ComPtr<ID3D12DescriptorHeap> newDescriptorHeap,
					   const bool bIsShaderVisibleHeap, const uint32_t numDescriptorsInHeap,
					   const uint32_t descriptorHandleIncSizeInHeap);

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