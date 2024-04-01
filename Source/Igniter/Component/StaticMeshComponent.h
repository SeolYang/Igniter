#pragma once
#include <Core/Handle.h>
#include <Asset/Common.h>
#include <Asset/Texture.h>
#include <Asset/StaticMesh.h>
#include <Asset/AssetCache.h>
#include <Gameplay/ComponentRegistry.h>

namespace ig
{
    struct StaticMesh;
    struct Texture;
    struct StaticMeshComponent
    {
    public:
        CachedAsset<StaticMesh> Mesh{};
        CachedAsset<Texture> DiffuseTex{};
        /* 2024/04/02 여기서 부터! Material를 가지는 StaticMeshComponent! */
    };

    IG_DECLARE_COMPONENT(StaticMeshComponent)
} // namespace ig