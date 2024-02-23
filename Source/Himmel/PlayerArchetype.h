#pragma once
#include <Gameplay/World.h>

#include <PlayerComponent.h>
#include <ControllableTag.h>
#include <HealthComponent.h>
#include <Gameplay/PositionComponent.h>

class PlayerArchetype
{
public:
	static fe::Entity Create(fe::World& world)
	{
		const fe::Entity newEntity = world.Create();
		world.Attach<Player>(newEntity);
		world.Attach<fe::PositionComponent>(newEntity);
		world.Attach<HealthComponent>(newEntity);
		world.Attach<Controllable>(newEntity);
		return newEntity;
	}
};