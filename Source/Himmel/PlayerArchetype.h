#pragma once
#include <Render/TransformComponent.h>
#include <Gameplay/Common.h>
#include <PlayerComponent.h>
#include <HealthComponent.h>

class PlayerArchetype
{
public:
    static fe::Entity Create(fe::Registry& registry)
    {
        const fe::Entity newEntity = registry.create();
        registry.emplace<Player>(newEntity);
        registry.emplace<fe::TransformComponent>(newEntity);
        registry.emplace<HealthComponent>(newEntity);
        return newEntity;
    }
};