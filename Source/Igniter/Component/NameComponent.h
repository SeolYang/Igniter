#pragma once
#include "Igniter/Core/String.h"
#include "Igniter/Core/Serialization.h"
#include "Igniter/Core/Meta.h"

namespace ig
{
    struct NameComponent
    {
        String Name{"New Entity"};
    };

    template <>
    Json& Serialize<Json, NameComponent>(Json& archive, const NameComponent& name);

    template <>
    const Json& Deserialize<Json, NameComponent>(const Json& archive, NameComponent& name);

    template <>
    void OnInspector<NameComponent>(Registry* registry, const Entity entity);

    IG_DECLARE_META(NameComponent);
} // namespace ig
