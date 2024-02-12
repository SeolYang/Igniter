#include <D3D12/DescriptorHeap.h>
#include <D3D12/Device.h>
#include <Core/Assert.h>
#include <Core/Utility.h>
#include <Engine.h>

namespace fe::dx
{
	DescriptorHeap::DescriptorHeap(const EDescriptorHeapType	newDescriptorHeapType,
								   ComPtr<ID3D12DescriptorHeap> newDescriptorHeap, const bool bIsShaderVisibleHeap,
								   const uint32_t numDescriptorsInHeap, const uint32_t descriptorHandleIncSizeInHeap)
		: descriptorHeapType(newDescriptorHeapType)
		, descriptorHeap(newDescriptorHeap)
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

	DescriptorHeap::DescriptorHeap(DescriptorHeap&& other) noexcept
		: descriptorHeapType(other.descriptorHeapType)
		, descriptorHeap(std::move(other.descriptorHeap))
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

	void DescriptorHeap::DeallocateIndex(const uint32_t index)
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

	Descriptor DescriptorHeap::Allocate(const EDescriptorType requestDescriptorType)
	{
		check(IsSupportDescriptor(descriptorHeapType, requestDescriptorType));
		const uint32_t allocatedIndex = AllocateIndex();
		return Descriptor{ *this, requestDescriptorType, allocatedIndex, GetIndexedCPUDescriptorHandle(allocatedIndex),
						   GetIndexedGPUDescriptorHandle(allocatedIndex) };
	}

	void DescriptorHeap::Deallocate(Descriptor& descriptor)
	{
		if (descriptor.ownedDescriptorHeap == this && descriptor.index != InvalidIndexU32)
		{
			DeallocateIndex(descriptor.index);
			descriptor.ownedDescriptorHeap = nullptr;
			descriptor.index = InvalidIndexU32;
			descriptor.cpuHandle = {};
			descriptor.gpuHandle = {};
		}
	}

	Descriptor::Descriptor(DescriptorHeap& owner, const EDescriptorType descriptorType, const uint32_t allocatedIndex,
						   const D3D12_CPU_DESCRIPTOR_HANDLE indexedCpuHandle,
						   const D3D12_GPU_DESCRIPTOR_HANDLE indexedGpuHandle)
		: ownedDescriptorHeap(&owner)
		, type(descriptorType)
		, index(allocatedIndex)
		, cpuHandle(indexedCpuHandle)
		, gpuHandle(indexedGpuHandle)
	{
	}

	Descriptor::Descriptor(Descriptor&& other) noexcept
		: ownedDescriptorHeap(std::exchange(other.ownedDescriptorHeap, nullptr))
		, type(other.type)
		, index(std::exchange(other.index, Descriptor::InvalidIndex))
		, cpuHandle(std::exchange(other.cpuHandle, {}))
		, gpuHandle(std::exchange(other.gpuHandle, {}))
	{
	}

	Descriptor::~Descriptor()
	{
		if (ownedDescriptorHeap != nullptr && index != InvalidIndex)
		{
			ownedDescriptorHeap->Deallocate(*this);
		}
	}

	Descriptor& Descriptor::operator=(Descriptor&& other) noexcept
	{
		this->ownedDescriptorHeap = std::exchange(other.ownedDescriptorHeap, nullptr);
		this->type = other.type;
		this->index = std::exchange(other.index, Descriptor::InvalidIndex);
		this->cpuHandle = std::exchange(other.cpuHandle, {});
		this->gpuHandle = std::exchange(other.gpuHandle, {});
		return *this;
	}

} // namespace fe::dx