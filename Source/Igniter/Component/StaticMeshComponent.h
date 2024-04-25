#pragma once
#include <Core/Handle.h>
#include <Asset/Common.h>
#include <Asset/StaticMesh.h>
#include <Asset/Material.h>
#include <Asset/AssetCache.h>
#include <Gameplay/ComponentRegistry.h>

namespace ig
{
    class StaticMesh;
    struct StaticMeshComponent
    {
    public:
        ~StaticMeshComponent();

    public:
        Handle<StaticMesh> Mesh{};
    };

    IG_DECLARE_COMPONENT(StaticMeshComponent)
}    // namespace ig
