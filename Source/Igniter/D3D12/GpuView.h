#pragma once
#include "Igniter/D3D12/Common.h"

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
        EGpuViewType                Type      = EGpuViewType::Unknown; /* Type of descriptor. */
        uint32_t                    Index     = InvalidIndexU32;       /* Index of descriptor in Descriptor Heap. */
        D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle = { };                   /* CPU Handle of descriptor. */
        D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle = { };                   /* GPU Handle of descriptor. */
    };
} // namespace ig
