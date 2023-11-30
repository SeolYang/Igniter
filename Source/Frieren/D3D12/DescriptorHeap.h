#pragma once
#include <D3D12/Commons.h>
#include <queue>

namespace fe
{
	class Device;
	class DescriptorHeap
	{
	public:
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
			friend DescriptorHeap;
			explicit Descriptor(DescriptorHeap& descriptorHeap);

		public:
			static constexpr uint32_t InvalidIndex = 0xffffffff;

		private:
			DescriptorHeap*				ownedDescriptorHeap = nullptr;
			uint32_t					index;
			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
		};

	private:
		uint32_t AllocateIndex();
		void	 ReleaseIndex(const uint32_t index);

		D3D12_CPU_DESCRIPTOR_HANDLE GetIndexedCPUDescriptorHandle(const uint32_t index);
		D3D12_GPU_DESCRIPTOR_HANDLE GetIndexedGPUDescriptorHandle(const uint32_t index);

	public:
		DescriptorHeap(Device& device, const D3D12_DESCRIPTOR_HEAP_TYPE type, const uint32_t numDescriptors, const std::string_view debugName);
		~DescriptorHeap();

		DescriptorHeap(const DescriptorHeap&) = delete;
		DescriptorHeap(DescriptorHeap&&) noexcept = delete;

		DescriptorHeap& operator=(const DescriptorHeap&) = delete;
		DescriptorHeap& operator=(DescriptorHeap&&) noexcept = delete;

		Descriptor AllocateDescriptor()
		{
			return Descriptor{ *this };
		}

	private:
		wrl::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
		const bool						  bIsShaderVisible = false;
		const uint32_t					  descriptorHandleIncrementSize = 0;

		D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandleForHeapStart{};
		D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandleForHeapStart{};

		const uint32_t numInitialDescriptors = 0;
		// @todo impl thread-safe alloc/release
		std::queue<uint32_t> indexPool;
	};

	using Descriptor = DescriptorHeap::Descriptor;
} // namespace fe