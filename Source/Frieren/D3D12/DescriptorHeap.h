#pragma once
#include <D3D12/Common.h>
#include <Core/Misc.h>
#include <Core/Mutex.h>
#include <Core/Container.h>

namespace fe::dx
{
	class Device;
	class GPUView;
	class DescriptorHeap
	{
		friend class Device;
		friend class GPUViewManager;

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

	private:
		DescriptorHeap(const EDescriptorHeapType newDescriptorHeapType, ComPtr<ID3D12DescriptorHeap> newDescriptorHeap,
					   const bool bIsShaderVisibleHeap, const uint32_t numDescriptorsInHeap,
					   const uint32_t descriptorHandleIncSizeInHeap);

		std::optional<GPUView> Allocate(const EGPUViewType desiredType);
		void				   Deallocate(const GPUView& gpuView);

	private:
		EDescriptorHeapType			 descriptorHeapType;
		ComPtr<ID3D12DescriptorHeap> descriptorHeap;
		uint32_t					 descriptorHandleIncrementSize = 0;
		uint32_t					 numInitialDescriptors = 0;

		bool						bIsShaderVisible = false;
		D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandleForHeapStart{};
		D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandleForHeapStart{};

		concurrency::concurrent_queue<uint32_t> descsriptorIdxPool;
	};
} // namespace fe::dx