#include <D3D12/DescriptorHeap.h>
#include <D3D12/Device.h>
#include <Core/Assert.h>
#include <Core/Utility.h>
#include <Engine.h>

namespace fe
{
	DescriptorHeap::DescriptorHeap(const Device& device, const D3D12_DESCRIPTOR_HEAP_TYPE type, const uint32_t numDescriptors, const std::string_view debugName)
		: bIsShaderVisible(type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER), descriptorHandleIncrementSize(device.GetDescriptorHandleIncrementSize(type)), numInitialDescriptors(numDescriptors), indexPool(CreateIndexQueue(numDescriptors))
	{
		FE_ASSERT(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES != type);

		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		desc.NodeMask = 0;
		desc.NumDescriptors = numDescriptors;
		desc.Type = type;
		desc.Flags = bIsShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		ID3D12Device10& nativeDevice = device.GetNative();
		const bool		bSucceeded = IsDXCallSucceeded(nativeDevice.CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));
		FE_CONDITIONAL_LOG(D3D12Fatal, bSucceeded, "Failed to create descriptor heap {}", debugName);

		if (bSucceeded)
		{
			cpuDescriptorHandleForHeapStart = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
			gpuDescriptorHandleForHeapStart = bIsShaderVisible ? descriptorHeap->GetGPUDescriptorHandleForHeapStart() : CD3DX12_GPU_DESCRIPTOR_HANDLE{};
		}
	}

	DescriptorHeap::~DescriptorHeap()
	{
		FE_CONDITIONAL_LOG(D3D12Warn, numInitialDescriptors == indexPool.size(), "Some Descriptorsd doesn't released yet. {}/{}", indexPool.size(), numInitialDescriptors);
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
		FE_ASSERT(index < numInitialDescriptors);
		FE_ASSERT(index != Descriptor::InvalidIndex);

		RecursiveLock lock{ mutex };
		FE_ASSERT(indexPool.size() <= numInitialDescriptors);
		if (index != Descriptor::InvalidIndex)
		{
			indexPool.push(index);
		}
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetIndexedCPUDescriptorHandle(const uint32_t index)
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE{ cpuDescriptorHandleForHeapStart, static_cast<INT>(index), descriptorHandleIncrementSize };
	}

	D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetIndexedGPUDescriptorHandle(const uint32_t index)
	{
		return bIsShaderVisible ? CD3DX12_GPU_DESCRIPTOR_HANDLE{ gpuDescriptorHandleForHeapStart, static_cast<INT>(index), descriptorHandleIncrementSize } : gpuDescriptorHandleForHeapStart;
	}

	Descriptor::Descriptor(DescriptorHeap& descriptorHeap)
		: ownedDescriptorHeap(&descriptorHeap), index(descriptorHeap.AllocateIndex()), cpuHandle(descriptorHeap.GetIndexedCPUDescriptorHandle(index)), gpuHandle(descriptorHeap.GetIndexedGPUDescriptorHandle(index))
	{
	}

	Descriptor::Descriptor(Descriptor&& rhs) noexcept
		: ownedDescriptorHeap(std::exchange(rhs.ownedDescriptorHeap, nullptr)), index(std::exchange(rhs.index, Descriptor::InvalidIndex)), cpuHandle(std::exchange(rhs.cpuHandle, {})), gpuHandle(std::exchange(rhs.gpuHandle, {}))
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

} // namespace fe