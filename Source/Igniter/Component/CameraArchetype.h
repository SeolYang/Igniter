#pragma once
#include <Component/CameraComponent.h>
#include <Component/TransformComponent.h>

namespace ig
{
    struct CameraArchetype
    {
    public:
        static Entity Create(Registry& registry)
        {
            const Entity newEntity = registry.create();
            registry.emplace<CameraComponent>(newEntity);
            registry.emplace<TransformComponent>(newEntity);
            return newEntity;
        }
    };
} // namespace ig
