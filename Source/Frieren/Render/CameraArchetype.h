#pragma once
#include <Render/CameraComponent.h>
#include <Render/TransformComponent.h>
#include <Gameplay/World.h>
#include <CameraController.h>

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
			world.Attach<CameraController>(newEntity);
			return newEntity;
        }
    };
}