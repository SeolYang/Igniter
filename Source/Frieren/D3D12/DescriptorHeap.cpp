#include <D3D12/DescriptorHeap.h>
#include <D3D12/Device.h>
#include <Core/Assert.h>
#include <Core/Utility.h>
#include <Engine.h>

namespace fe::dx
{
	DescriptorHeap::DescriptorHeap(DescriptorHeap&& other) noexcept
		: descriptorHeap(std::move(other.descriptorHeap))
		, descriptorHandleIncrementSize(other.descriptorHandleIncrementSize)
		, numInitialDescriptors(std::exchange(other.numInitialDescriptors, 0))
		, cpuDescriptorHandleForHeapStart(std::exchange(other.cpuDescriptorHandleForHeapStart, {}))
		, gpuDescriptorHandleForHeapStart(std::exchange(other.gpuDescriptorHandleForHeapStart, {}))
		, bIsShaderVisible(other.bIsShaderVisible)
		, indexPool(std::move(other.indexPool))
	{
	}

	DescriptorHeap::~DescriptorHeap()
	{
		check(numInitialDescriptors == indexPool.size());
	}

	DescriptorHeap::DescriptorHeap(ComPtr<ID3D12DescriptorHeap> newDescriptorHeap, const bool bIsShaderVisibleHeap,
								   const uint32_t numDescriptorsInHeap, const uint32_t descriptorHandleIncSizeInHeap)
		: descriptorHeap(newDescriptorHeap)
		, descriptorHandleIncrementSize(descriptorHandleIncSizeInHeap)
		, numInitialDescriptors(numDescriptorsInHeap)
		, bIsShaderVisible(bIsShaderVisibleHeap)
		, cpuDescriptorHandleForHeapStart(descriptorHeap->GetCPUDescriptorHandleForHeapStart())
		, gpuDescriptorHandleForHeapStart(bIsShaderVisible ? descriptorHeap->GetGPUDescriptorHandleForHeapStart()
														   : D3D12_GPU_DESCRIPTOR_HANDLE{})
		, indexPool(CreateIndexQueue(numDescriptorsInHeap))
	{
		check(newDescriptorHeap);
	}

	uint32_t DescriptorHeap::AllocateIndex()
	{
		RecursiveLock lock{ mutex };
		if (indexPool.empty())
		{
			FE_LOG(D3D12Warn, "There are no more remaining descriptors.");
			return Descriptor::InvalidIndex;
		}

		const uint32_t index = indexPool.front();
		indexPool.pop();
		return index;
	}

	void DescriptorHeap::ReleaseIndex(const uint32_t index)
	{
		verify(index < numInitialDescriptors);
		verify(index != Descriptor::InvalidIndex);

		RecursiveLock lock{ mutex };
		verify(indexPool.size() <= numInitialDescriptors);
		if (index != Descriptor::InvalidIndex)
		{
			indexPool.push(index);
		}
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetIndexedCPUDescriptorHandle(const uint32_t index) const
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE{ cpuDescriptorHandleForHeapStart, static_cast<INT>(index),
											  descriptorHandleIncrementSize };
	}

	D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetIndexedGPUDescriptorHandle(const uint32_t index) const
	{
		return bIsShaderVisible
				   ? CD3DX12_GPU_DESCRIPTOR_HANDLE{ gpuDescriptorHandleForHeapStart, static_cast<INT>(index),
													descriptorHandleIncrementSize }
				   : gpuDescriptorHandleForHeapStart;
	}

	DescriptorHeap& DescriptorHeap::operator=(DescriptorHeap&& other) noexcept 
	{
		descriptorHeap = std::move(other.descriptorHeap);
		descriptorHandleIncrementSize = other.descriptorHandleIncrementSize;
		numInitialDescriptors = other.numInitialDescriptors;
		bIsShaderVisible = other.bIsShaderVisible;
		cpuDescriptorHandleForHeapStart = other.cpuDescriptorHandleForHeapStart;
		gpuDescriptorHandleForHeapStart = other.gpuDescriptorHandleForHeapStart;
		indexPool = std::move(other.indexPool);
		return *this;
	}

	Descriptor::Descriptor(DescriptorHeap& descriptorHeap)
		: ownedDescriptorHeap(&descriptorHeap)
		, index(descriptorHeap.AllocateIndex())
		, cpuHandle(descriptorHeap.GetIndexedCPUDescriptorHandle(index))
		, gpuHandle(descriptorHeap.GetIndexedGPUDescriptorHandle(index))
	{
	}

	Descriptor::Descriptor(Descriptor&& rhs) noexcept
		: ownedDescriptorHeap(std::exchange(rhs.ownedDescriptorHeap, nullptr))
		, index(std::exchange(rhs.index, Descriptor::InvalidIndex))
		, cpuHandle(std::exchange(rhs.cpuHandle, {}))
		, gpuHandle(std::exchange(rhs.gpuHandle, {}))
	{
	}

	Descriptor::~Descriptor()
	{
		if (ownedDescriptorHeap != nullptr && index != InvalidIndex)
		{
			ownedDescriptorHeap->ReleaseIndex(index);
		}
	}

	Descriptor& Descriptor::operator=(Descriptor&& rhs) noexcept
	{
		this->ownedDescriptorHeap = std::exchange(rhs.ownedDescriptorHeap, nullptr);
		this->index = std::exchange(rhs.index, Descriptor::InvalidIndex);
		this->cpuHandle = std::exchange(rhs.cpuHandle, {});
		this->gpuHandle = std::exchange(rhs.gpuHandle, {});
		return *this;
	}

} // namespace fe::dx