#pragma once
#include "Igniter/Component/CameraComponent.h"
#include "Igniter/Component/TransformComponent.h"

namespace ig
{
    struct CameraArchetype
    {
    public:
        static Entity Create(Registry* registry)
        {
            const Entity newEntity = registry->create();
            registry->emplace<CameraComponent>(newEntity);
            registry->emplace<TransformComponent>(newEntity);
            return newEntity;
        }
    };

    IG_DECLARE_META(CameraArchetype);
} // namespace ig
