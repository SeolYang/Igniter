#pragma once
#include <Render/TransformComponent.h>
#include <HealthComponent.h>
#include <Enemy.h>
#include <Gameplay/World.h>

class EnemyArchetype
{
public:
    static fe::Entity Create(fe::World& world)
    {
        const fe::Entity newEntity = world.Create();
        world.Attach<Enemy>(newEntity);
        world.Attach<fe::TransformComponent>(newEntity);
        world.Attach<HealthComponent>(newEntity);
        return newEntity;
    }
};