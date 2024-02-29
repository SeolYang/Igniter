#include <D3D12/DescriptorHeap.h>
#include <D3D12/RenderDevice.h>
#include <D3D12/GPUView.h>
#include <Core/Assert.h>
#include <Core/Container.h>
#include <Engine.h>

namespace fe
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

	std::optional<GpuView> DescriptorHeap::Allocate(const EGpuViewType desiredType)
	{
		check(descriptorHeapType != EDescriptorHeapType::CBV_SRV_UAV ||
			  (descriptorHeapType == EDescriptorHeapType::CBV_SRV_UAV && (desiredType == EGpuViewType::ConstantBufferView ||
																		  desiredType == EGpuViewType::ShaderResourceView ||
																		  desiredType == EGpuViewType::UnorderedAccessView)));

		check(descriptorHeapType != EDescriptorHeapType::Sampler ||
			  (descriptorHeapType == EDescriptorHeapType::Sampler && (desiredType == EGpuViewType::Sampler)));

		check(descriptorHeapType != EDescriptorHeapType::RTV ||
			  (descriptorHeapType == EDescriptorHeapType::RTV && (desiredType == EGpuViewType::RenderTargetView)));

		check(descriptorHeapType != EDescriptorHeapType::DSV ||
			  (descriptorHeapType == EDescriptorHeapType::DSV && (desiredType == EGpuViewType::DepthStencilView)));

		uint32_t newDescriptorIdx = std::numeric_limits<uint32_t>::max();
		if (descsriptorIdxPool.empty() || !descsriptorIdxPool.try_pop(newDescriptorIdx))
		{
			return GpuView{};
		}

		return GpuView{
			.Type = desiredType,
			.Index = newDescriptorIdx,
			.CPUHandle = GetIndexedCPUDescriptorHandle(newDescriptorIdx),
			.GPUHandle = GetIndexedGPUDescriptorHandle(newDescriptorIdx)
		};
	}

	void DescriptorHeap::Deallocate(const GpuView& gpuView)
	{
		check(descriptorHeapType != EDescriptorHeapType::CBV_SRV_UAV ||
			  (descriptorHeapType == EDescriptorHeapType::CBV_SRV_UAV && (gpuView.Type == EGpuViewType::ConstantBufferView ||
																		  gpuView.Type == EGpuViewType::ShaderResourceView ||
																		  gpuView.Type == EGpuViewType::UnorderedAccessView)));

		check(descriptorHeapType != EDescriptorHeapType::Sampler ||
			  (descriptorHeapType == EDescriptorHeapType::Sampler && (gpuView.Type == EGpuViewType::Sampler)));

		check(descriptorHeapType != EDescriptorHeapType::RTV ||
			  (descriptorHeapType == EDescriptorHeapType::RTV && (gpuView.Type == EGpuViewType::RenderTargetView)));

		check(descriptorHeapType != EDescriptorHeapType::DSV ||
			  (descriptorHeapType == EDescriptorHeapType::DSV && (gpuView.Type == EGpuViewType::DepthStencilView)));

		check(gpuView.Index < numInitialDescriptors);
		check(gpuView.Index != std::numeric_limits<decltype(GpuView::Index)>::max());
		descsriptorIdxPool.push(gpuView.Index);
		check(descsriptorIdxPool.unsafe_size() <= numInitialDescriptors);
	}

} // namespace fe