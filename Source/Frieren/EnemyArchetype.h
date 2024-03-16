#pragma once
#include <Gameplay/Common.h>
#include <Component/TransformComponent.h>
#include <HealthComponent.h>
#include <Enemy.h>

class EnemyArchetype
{
public:
    static ig::Entity Create(ig::Registry& registry)
    {
        const ig::Entity newEntity = registry.create();
        registry.emplace<Enemy>(newEntity);
        registry.emplace<ig::TransformComponent>(newEntity);
        registry.emplace<HealthComponent>(newEntity);
        return newEntity;
    }
};