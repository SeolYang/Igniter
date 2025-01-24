#pragma once
#include "Igniter/Core/Meta.h"
#include "Igniter/Component/TransformComponent.h"
#include "Igniter/Component/LightComponent.h"

namespace ig
{
    struct PointLightArchetype
    {
        static Entity Create(Registry* registry)
        {
            IG_CHECK(registry != nullptr);
            ig::Entity entity = registry->create();
            registry->emplace<TransformComponent>(entity);
            LightComponent& light = registry->emplace<LightComponent>(entity);
            light.Property.Type = ELightType::Point;
            light.Property.Radius = 1.f;
            return entity;
        }
    };

    IG_DECLARE_META(PointLightArchetype);
}
