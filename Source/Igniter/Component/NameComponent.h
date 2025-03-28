#pragma once
#include "Igniter/Core/String.h"
#include "Igniter/Core/Serialization.h"
#include "Igniter/Core/Meta.h"

namespace ig
{
    struct NameComponent
    {
        std::string Name{"New Entity"};
    };

    template <>
    Json& Serialize<Json, NameComponent>(Json& archive, const NameComponent& name);

    template <>
    const Json& Deserialize<Json, NameComponent>(const Json& archive, NameComponent& name);

    template <>
    void OnInspector<NameComponent>(Registry* registry, const Entity entity);

    IG_META_DECLARE(NameComponent);
} // namespace ig
