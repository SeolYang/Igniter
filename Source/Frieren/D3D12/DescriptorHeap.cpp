#include <D3D12/DescriptorHeap.h>
#include <D3D12/Device.h>
#include <Core/Assert.h>
#include <Core/Container.h>
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
														   : D3D12_GPU_DESCRIPTOR_HANDLE{ std::numeric_limits<decltype(D3D12_GPU_DESCRIPTOR_HANDLE::ptr)>::max() })
	{
		check(newDescriptorHeap);
		for (uint32_t idx = 0; idx < numDescriptorsInHeap; ++idx)
		{
			this->descsriptorIdxPool.push(idx);
		}
	}

	DescriptorHeap::DescriptorHeap(DescriptorHeap&& other) noexcept
		: descriptorHeapType(other.descriptorHeapType)
		, descriptorHeap(std::move(other.descriptorHeap))
		, descriptorHandleIncrementSize(other.descriptorHandleIncrementSize)
		, numInitialDescriptors(std::exchange(other.numInitialDescriptors, 0))
		, cpuDescriptorHandleForHeapStart(std::exchange(other.cpuDescriptorHandleForHeapStart, {}))
		, gpuDescriptorHandleForHeapStart(std::exchange(other.gpuDescriptorHandleForHeapStart, {}))
		, bIsShaderVisible(other.bIsShaderVisible)
		, descsriptorIdxPool(std::move(other.descsriptorIdxPool))
	{
	}

	DescriptorHeap::~DescriptorHeap()
	{
		check(numInitialDescriptors == descsriptorIdxPool.unsafe_size());
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

	FrameResource<GPUView> DescriptorHeap::Request(FrameResourceManager& frameResourceManager, const EDescriptorType desiredType)
	{
		check(descriptorHeapType != EDescriptorHeapType::CBV_SRV_UAV ||
			  (descriptorHeapType == EDescriptorHeapType::CBV_SRV_UAV && (desiredType == EDescriptorType::ConstantBufferView ||
																		  desiredType == EDescriptorType::ShaderResourceView ||
																		  desiredType == EDescriptorType::UnorderedAccessView)));

		check(descriptorHeapType != EDescriptorHeapType::Sampler ||
			  (descriptorHeapType == EDescriptorHeapType::Sampler && (desiredType == EDescriptorType::Sampler)));

		check(descriptorHeapType != EDescriptorHeapType::RTV ||
			  (descriptorHeapType == EDescriptorHeapType::RTV && (desiredType == EDescriptorType::RenderTargetView)));

		check(descriptorHeapType != EDescriptorHeapType::DSV ||
			  (descriptorHeapType == EDescriptorHeapType::DSV && (desiredType == EDescriptorType::DepthStencilView)));

		uint32_t newDescriptorIdx = std::numeric_limits<uint32_t>::max();
		if (descsriptorIdxPool.empty() || !descsriptorIdxPool.try_pop(newDescriptorIdx))
		{
			return {};
		}

		return MakeFrameResourceCustom<GPUView>(
			frameResourceManager,
			[this](GPUView* ptr) { check(ptr && ptr->IsValid());  this->Release(ptr->Index); delete ptr; },
			GPUView{ desiredType, newDescriptorIdx, GetIndexedCPUDescriptorHandle(newDescriptorIdx), GetIndexedGPUDescriptorHandle(newDescriptorIdx) });
	}

	void DescriptorHeap::Release(const uint32_t descriptorIdx)
	{
		check(descriptorIdx < numInitialDescriptors);
		check(descriptorIdx != std::numeric_limits<decltype(GPUView::Index)>::max());
		descsriptorIdxPool.push(descriptorIdx);
		check(descsriptorIdxPool.unsafe_size() <= numInitialDescriptors);
	}
} // namespace fe::dx