#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/D3D12/Common.h"

namespace ig
{
    class GpuDevice;
    class GpuView;

    class DescriptorHeap final
    {
        friend class GpuDevice;

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
        D3D12_CPU_DESCRIPTOR_HANDLE GetIndexedCPUDescriptorHandle(const U32 index) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetIndexedGPUDescriptorHandle(const U32 index) const;

        /* #sy_todo optional 대신 그냥 invalid gpu view 반환 하도록 변경 하기 */
        GpuView Allocate(const EGpuViewType desiredType);
        void Deallocate(const GpuView& gpuView);

    private:
        DescriptorHeap(const EDescriptorHeapType newDescriptorHeapType, ComPtr<ID3D12DescriptorHeap> newDescriptorHeap,
                       const bool bIsShaderVisibleHeap, const U32 numDescriptorsInHeap, const U32 descriptorHandleIncSizeInHeap);

    private:
        EDescriptorHeapType descriptorHeapType;
        ComPtr<ID3D12DescriptorHeap> descriptorHeap;
        U32 descriptorHandleIncrementSize = 0;
        U32 numInitialDescriptors = 0;

        bool bIsShaderVisible = false;
        D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandleForHeapStart{};
        D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandleForHeapStart{};

        std::priority_queue<U32, Vector<U32>, std::greater<U32>> descriptorIdxPool;
    };
} // namespace ig
