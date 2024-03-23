#pragma once
#include <PCH.h>
#include <D3D12/Common.h>
#include <Core/Container.h>

namespace ig
{
    class RenderDevice;
    class GpuView;
    class DescriptorHeap
    {
        friend class RenderDevice;
        friend class GpuViewManager;

    public:
        DescriptorHeap(const DescriptorHeap&) = delete;
        DescriptorHeap(DescriptorHeap&& other) noexcept;
        ~DescriptorHeap();

        DescriptorHeap& operator=(const DescriptorHeap&) = delete;
        DescriptorHeap& operator=(DescriptorHeap&&) noexcept = delete;

        bool IsValid() const { return descriptorHeap; }
        operator bool() const { return IsValid(); }

        ID3D12DescriptorHeap& GetNative() { return *descriptorHeap.Get(); }
        EDescriptorHeapType GetType() const { return descriptorHeapType; }
        D3D12_CPU_DESCRIPTOR_HANDLE GetIndexedCPUDescriptorHandle(const uint32_t index) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetIndexedGPUDescriptorHandle(const uint32_t index) const;

    private:
        DescriptorHeap(const EDescriptorHeapType newDescriptorHeapType, ComPtr<ID3D12DescriptorHeap> newDescriptorHeap,
                       const bool bIsShaderVisibleHeap, const uint32_t numDescriptorsInHeap,
                       const uint32_t descriptorHandleIncSizeInHeap);

        std::optional<GpuView> Allocate(const EGpuViewType desiredType);
        void Deallocate(const GpuView& gpuView);

    private:
        EDescriptorHeapType descriptorHeapType;
        ComPtr<ID3D12DescriptorHeap> descriptorHeap;
        uint32_t descriptorHandleIncrementSize = 0;
        uint32_t numInitialDescriptors = 0;

        bool bIsShaderVisible = false;
        D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandleForHeapStart{};
        D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandleForHeapStart{};

        std::priority_queue<uint32_t, std::vector<uint32_t>, std::greater<uint32_t>> descriptorIdxPool;
    };
} // namespace ig