#pragma once
#include <Render/TransformComponent.h>
#include <Gameplay/World.h>
#include <PlayerComponent.h>
#include <HealthComponent.h>

class PlayerArchetype
{
public:
	static fe::Entity Create(fe::World& world)
	{
		const fe::Entity newEntity = world.Create();
		world.Attach<Player>(newEntity);
		world.Attach<fe::TransformComponent>(newEntity);
		world.Attach<HealthComponent>(newEntity);
		return newEntity;
	}
};