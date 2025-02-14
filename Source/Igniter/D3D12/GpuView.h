#pragma once
#include "Igniter/D3D12/Common.h"

namespace ig
{
    class GpuView final
    {
    public:
        [[nodiscard]] bool IsValid() const noexcept { return Type != EGpuViewType::Unknown && Index != InvalidIndexU32; }
        [[nodiscard]] operator bool() const noexcept { return IsValid(); }

        [[nodiscard]] bool HasValidCpuHandle() const noexcept { return CpuHandle.ptr != std::numeric_limits<decltype(CpuHandle.ptr)>::max(); }
        [[nodiscard]] bool HasValidGpuHandle() const noexcept { return GpuHandle.ptr != std::numeric_limits<decltype(GpuHandle.ptr)>::max(); }

    public:
        EGpuViewType Type = EGpuViewType::Unknown;  /* Type of descriptor. */
        U32 Index = InvalidIndexU32;                /* Index of descriptor in Descriptor Heap. */
        D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle = {}; /* CPU Handle of descriptor. */
        D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle = {}; /* GPU Handle of descriptor. */
    };
} // namespace ig
