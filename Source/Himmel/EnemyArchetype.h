#pragma once
#include <Render/TransformComponent.h>
#include <Gameplay/Common.h>
#include <HealthComponent.h>
#include <Enemy.h>

class EnemyArchetype
{
public:
    static fe::Entity Create(fe::Registry& registry)
    {
        const fe::Entity newEntity = registry.create();
        registry.emplace<Enemy>(newEntity);
        registry.emplace<fe::TransformComponent>(newEntity);
        registry.emplace<HealthComponent>(newEntity);
        return newEntity;
    }
};