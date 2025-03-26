#pragma once
#include "Igniter/Core/Meta.h"
#include "Igniter/Core/Serialization.h"
#include "Igniter/Render/Light.h"

namespace ig
{
    struct LightComponent
    {
        Light Property;
    };

    template <>
    Json& Serialize<Json, LightComponent>(Json& archive, const LightComponent& light);

    template<>
    const Json& Deserialize<Json, LightComponent>(const Json& archive, LightComponent& light);

    template <>
    void OnInspector<LightComponent>(Registry* registry, const Entity entity);

    IG_DECLARE_META(LightComponent);
} // namespace ig
