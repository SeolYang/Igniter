#pragma once
#include <Render/CameraComponent.h>
#include <Render/TransformComponent.h>
#include <Gameplay/Common.h>

namespace fe
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
} // namespace fe