#pragma once
#include "Igniter/Core/Handle.h"
#include "Igniter/Core/Serialization.h"
#include "Igniter/Core/Meta.h"

namespace ig
{
    class StaticMesh;

    /* Static Mesh의 수명은 외부에서 관리되어야 함. */
    struct StaticMeshComponent
    {
        Handle<StaticMesh> Mesh{};
    };

    template <>
    Json& Serialize<Json, StaticMeshComponent>(Json& archive, const StaticMeshComponent& staticMesh);
    template <>
    const Json& Deserialize(const Json& archive, StaticMeshComponent& staticMesh);

    template <>
    void OnInspector<StaticMeshComponent>(Registry* registry, const Entity entity);
    
    IG_META_DECLARE(StaticMeshComponent);
} // namespace ig
