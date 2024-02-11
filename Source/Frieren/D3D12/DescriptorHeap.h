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
	public:
		Descriptor(Descriptor&& rhs) noexcept;
		~Descriptor();

		Descriptor& operator=(Descriptor&& rhs) noexcept;

		uint32_t					GetDescriptorIndex() const { return index; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle() const { return cpuHandle; }
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle() const { return gpuHandle; }

	private:
		explicit Descriptor(DescriptorHeap& descriptorHeap);

	public:
		static constexpr uint32_t InvalidIndex = InvalidIndexU32;

	private:
		friend DescriptorHeap;
		DescriptorHeap*				ownedDescriptorHeap = nullptr;
		uint32_t					index;
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
	};

	class DescriptorHeap
	{
		friend class Device;
		friend class Descriptor;

	private:
		uint32_t AllocateIndex();
		void	 ReleaseIndex(const uint32_t index);

		D3D12_CPU_DESCRIPTOR_HANDLE GetIndexedCPUDescriptorHandle(const uint32_t index) const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetIndexedGPUDescriptorHandle(const uint32_t index) const;

	public:
		DescriptorHeap(const DescriptorHeap&) = delete;
		DescriptorHeap(DescriptorHeap&& other) noexcept;
		~DescriptorHeap();

		DescriptorHeap& operator=(const DescriptorHeap&) = delete;
		DescriptorHeap& operator=(DescriptorHeap&& other) noexcept;

		Descriptor AllocateDescriptor() { return Descriptor{ *this }; }

	private:
		DescriptorHeap(ComPtr<ID3D12DescriptorHeap> newDescriptorHeap, const bool bIsShaderVisibleHeap,
					   const uint32_t numDescriptorsInHeap, const uint32_t descriptorHandleIncSizeInHeap);

	private:
		ComPtr<ID3D12DescriptorHeap> descriptorHeap;
		uint32_t					 descriptorHandleIncrementSize = 0;
		uint32_t					 numInitialDescriptors = 0;

		bool						bIsShaderVisible = false;
		D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandleForHeapStart{};
		D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandleForHeapStart{};

		RecursiveMutex		 mutex;
		std::queue<uint32_t> indexPool;
	};
} // namespace fe::dx