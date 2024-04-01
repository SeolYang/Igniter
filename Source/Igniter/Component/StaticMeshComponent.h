#pragma once
#include <Core/Handle.h>
#include <Asset/Common.h>
#include <Gameplay/ComponentRegistry.h>

namespace ig
{
    class GpuBuffer;
    class GpuView;
} // namespace ig

namespace ig
{
    struct StaticMesh;
    struct StaticMeshComponent
    {
    public:
        CachedAsset<StaticMesh> CachedStaticMesh;
        RefHandle<GpuView> DiffuseTex{};
        RefHandle<GpuView> DiffuseTexSampler{};
    };

    IG_DECLARE_COMPONENT(StaticMeshComponent);
} // namespace ig