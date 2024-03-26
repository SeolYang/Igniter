#pragma once
#include <D3D12/Common.h>

namespace ig
{
    class GpuView final
    {
    public:
        bool IsValid() const { return Type != EGpuViewType::Unknown && Index != InvalidIndexU32; }
        operator bool() const { return IsValid(); }

        bool HasValidCPUHandle() const { return CPUHandle.ptr != std::numeric_limits<decltype(CPUHandle.ptr)>::max(); }
        bool HasValidGPUHandle() const { return GPUHandle.ptr != std::numeric_limits<decltype(GPUHandle.ptr)>::max(); }

    public:
        const EGpuViewType Type = EGpuViewType::Unknown;  /* Type of descriptor. */
        const uint32_t Index = InvalidIndexU32;           /* Index of descriptor in Descriptor Heap. */
        const D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle = {}; /* CPU Handle of descriptor. */
        const D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle = {}; /* GPU Handle of descriptor. */
    };
} // namespace ig