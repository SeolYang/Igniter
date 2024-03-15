#pragma once
#include <D3D12/Common.h>
#include <D3D12/Fence.h>

namespace fe
{
    class GpuSync
    {
    private:
        Fence* fence = nullptr;
        const size_t syncPoint = 0;
    };
} // namespace fe