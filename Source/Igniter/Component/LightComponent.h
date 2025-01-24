#pragma once
#include "Igniter/Core/Meta.h"
#include "Igniter/Render/Light.h"

namespace ig
{
    struct LightComponent
    {
      public:
        Json& Serialize(Json& archive) const;
        const Json& Deserialize(const Json& archive);
        static void OnInspector(Registry* registry, const Entity entity);

      public:
        Light Property;
    };

    IG_DECLARE_TYPE_META(LightComponent);
} // namespace ig
