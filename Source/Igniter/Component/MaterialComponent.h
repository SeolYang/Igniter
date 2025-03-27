#pragma once
#include "Igniter/Core/Handle.h"
#include "Igniter/Core/Meta.h"

namespace ig
{
    class Material;

    struct MaterialComponent
    {
        Handle<Material> Instance{};
    };

    template <>
    Json& Serialize(Json& archive, const MaterialComponent& materialComponent);

    template <>
    const Json& Deserialize(const Json& archive, MaterialComponent& materialComponent);

    template <>
    void OnInspector<MaterialComponent>(Registry* registry, const Entity entity);

    IG_META_DECLARE(MaterialComponent);
} // namespace ig
