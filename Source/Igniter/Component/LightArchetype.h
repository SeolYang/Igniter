#pragma once
#include "Igniter/Core/Meta.h"
#include "Igniter/Component/TransformComponent.h"
#include "Igniter/Component/LightComponent.h"
#include "Igniter/Component/Archetype.h"

namespace ig
{
    using PointLightArchetype = Archetype<TransformComponent, LightComponent>;
    template <>
    inline Entity PointLightArchetype::Create(Registry* registry)
    {
        if (registry == nullptr)
        {
            return NullEntity;
        }

        const Entity newEntity = registry->create();
        auto [transform, light] = CreateInternal(registry, newEntity);
        light.Property.Type = ELightType::Point;
        light.Property.FalloffRadius = 1.f;
        return newEntity;
    }
    IG_DECLARE_META(PointLightArchetype);
}
