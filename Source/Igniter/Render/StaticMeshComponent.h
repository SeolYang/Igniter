#pragma once
#include <Core/Handle.h>

namespace ig
{
    class GpuBuffer;
    class GpuView;
} // namespace ig

namespace ig
{
    struct StaticMeshComponent
    {
    public:
        RefHandle<GpuView> VerticesBufferSRV = {};
        RefHandle<GpuBuffer> IndexBufferHandle = {};
        uint32_t NumIndices = 0;
        RefHandle<GpuView> DiffuseTex{};
        RefHandle<GpuView> DiffuseTexSampler{};
    };
} // namespace ig