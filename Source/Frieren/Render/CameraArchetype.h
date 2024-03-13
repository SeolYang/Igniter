#pragma once
#include <Render/CameraComponent.h>
#include <Render/TransformComponent.h>
#include <Gameplay/World.h>

namespace fe
{
    struct CameraArchetype
    {
	public:
        static Entity Create(World& world)
        {
			const Entity newEntity = world.Create();
			world.Attach<Camera>(newEntity);
			world.Attach<TransformComponent>(newEntity);
			return newEntity;
        }
    };
}