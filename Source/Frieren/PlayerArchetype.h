#pragma once
#include <Component/TransformComponent.h>
#include <Gameplay/Common.h>
#include <PlayerComponent.h>
#include <HealthComponent.h>

class PlayerArchetype
{
public:
    static ig::Entity Create(ig::Registry& registry)
    {
        const ig::Entity newEntity = registry.create();
        registry.emplace<Player>(newEntity);
        registry.emplace<ig::TransformComponent>(newEntity);
        registry.emplace<HealthComponent>(newEntity);
        return newEntity;
    }
};