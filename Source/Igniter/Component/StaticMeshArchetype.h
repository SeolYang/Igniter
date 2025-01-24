#pragma once
#include "Igniter/Component/TransformComponent.h"
#include "Igniter/Component/StaticMeshComponent.h"
#include "Igniter/Component/MaterialComponent.h"

namespace ig
{
    struct StaticMeshArchetype
    {
        static Entity Create(Registry* registry)
        {
            const Entity newEntity = registry->create();
            registry->emplace<TransformComponent>(newEntity);
            registry->emplace<StaticMeshComponent>(newEntity);
            registry->emplace<MaterialComponent>(newEntity);
            return newEntity;
        }
    };

    IG_DECLARE_META(StaticMeshArchetype);
} // namespace ig
