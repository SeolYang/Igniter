#include "Igniter/Igniter.h"
#include "Igniter/Core/ContainerUtils.h"
#include "Igniter/D3D12/DescriptorHeap.h"
#include "Igniter/D3D12/GpuDevice.h"
#include "Igniter/D3D12/GPUView.h"

namespace ig
{
    DescriptorHeap::DescriptorHeap(const EDescriptorHeapType newDescriptorHeapType, ComPtr<ID3D12DescriptorHeap> newDescriptorHeap,
                                   const bool bIsShaderVisibleHeap, const U32 numDescriptorsInHeap, const U32 descriptorHandleIncSizeInHeap)
        : descriptorHeapType(newDescriptorHeapType)
        , descriptorHeap(newDescriptorHeap)
        , descriptorHandleIncrementSize(descriptorHandleIncSizeInHeap)
        , numInitialDescriptors(numDescriptorsInHeap)
        , bIsShaderVisible(bIsShaderVisibleHeap)
        , cpuDescriptorHandleForHeapStart(descriptorHeap->GetCPUDescriptorHandleForHeapStart())
        , gpuDescriptorHandleForHeapStart(bIsShaderVisible ? descriptorHeap->GetGPUDescriptorHandleForHeapStart() : D3D12_GPU_DESCRIPTOR_HANDLE{std::numeric_limits<decltype(D3D12_GPU_DESCRIPTOR_HANDLE::ptr)>::max()})
    {
        IG_CHECK(newDescriptorHeap);
        for (U32 idx = 0; idx < numDescriptorsInHeap; ++idx)
        {
            this->descriptorIdxPool.push(idx);
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
        , descriptorIdxPool(std::move(other.descriptorIdxPool))
    {}

    DescriptorHeap::~DescriptorHeap()
    {
        IG_CHECK(numInitialDescriptors == descriptorIdxPool.size());
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetIndexedCPUDescriptorHandle(const U32 index) const
    {
        return CD3DX12_CPU_DESCRIPTOR_HANDLE{cpuDescriptorHandleForHeapStart, static_cast<INT>(index), descriptorHandleIncrementSize};
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetIndexedGPUDescriptorHandle(const U32 index) const
    {
        return bIsShaderVisible ? CD3DX12_GPU_DESCRIPTOR_HANDLE{gpuDescriptorHandleForHeapStart, static_cast<INT>(index), descriptorHandleIncrementSize} : gpuDescriptorHandleForHeapStart;
    }

    GpuView DescriptorHeap::Allocate(const EGpuViewType desiredType)
    {
        IG_CHECK(descriptorHeapType != EDescriptorHeapType::CBV_SRV_UAV ||
            (descriptorHeapType == EDescriptorHeapType::CBV_SRV_UAV &&
                (desiredType == EGpuViewType::ConstantBufferView || desiredType == EGpuViewType::ShaderResourceView ||
                    desiredType == EGpuViewType::UnorderedAccessView)));

        IG_CHECK(descriptorHeapType != EDescriptorHeapType::Sampler ||
            (descriptorHeapType == EDescriptorHeapType::Sampler && (desiredType == EGpuViewType::Sampler)));

        IG_CHECK(descriptorHeapType != EDescriptorHeapType::RTV ||
            (descriptorHeapType == EDescriptorHeapType::RTV && (desiredType == EGpuViewType::RenderTargetView)));

        IG_CHECK(descriptorHeapType != EDescriptorHeapType::DSV ||
            (descriptorHeapType == EDescriptorHeapType::DSV && (desiredType == EGpuViewType::DepthStencilView)));

        if (descriptorIdxPool.empty())
        {
            IG_CHECK_NO_ENTRY();
            return GpuView{};
        }

        const U32 newDescriptorIdx = descriptorIdxPool.top();
        descriptorIdxPool.pop();
        return GpuView{
            .Type = desiredType,
            .Index = newDescriptorIdx,
            .CpuHandle = GetIndexedCPUDescriptorHandle(newDescriptorIdx),
            .GpuHandle = GetIndexedGPUDescriptorHandle(newDescriptorIdx)
        };
    }

    void DescriptorHeap::Deallocate(const GpuView& gpuView)
    {
        IG_CHECK(descriptorHeapType != EDescriptorHeapType::CBV_SRV_UAV ||
            (descriptorHeapType == EDescriptorHeapType::CBV_SRV_UAV &&
                (gpuView.Type == EGpuViewType::ConstantBufferView || gpuView.Type == EGpuViewType::ShaderResourceView ||
                    gpuView.Type == EGpuViewType::UnorderedAccessView)));

        IG_CHECK(descriptorHeapType != EDescriptorHeapType::Sampler ||
            (descriptorHeapType == EDescriptorHeapType::Sampler && (gpuView.Type == EGpuViewType::Sampler)));

        IG_CHECK(descriptorHeapType != EDescriptorHeapType::RTV ||
            (descriptorHeapType == EDescriptorHeapType::RTV && (gpuView.Type == EGpuViewType::RenderTargetView)));

        IG_CHECK(descriptorHeapType != EDescriptorHeapType::DSV ||
            (descriptorHeapType == EDescriptorHeapType::DSV && (gpuView.Type == EGpuViewType::DepthStencilView)));

        IG_CHECK(gpuView.Index < numInitialDescriptors);
        IG_CHECK(gpuView.Index != std::numeric_limits<decltype(GpuView::Index)>::max());
        descriptorIdxPool.push(gpuView.Index);
        IG_CHECK(descriptorIdxPool.size() <= numInitialDescriptors);
    }
} // namespace ig
