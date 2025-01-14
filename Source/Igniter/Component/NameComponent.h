#pragma once
#include "Igniter/Core/String.h"
#include "Igniter/Core/Meta.h"

namespace ig
{
    struct NameComponent
    {
      public:
        Json& Serialize(Json& archive) const;
        const Json& Deserialize(const Json& archive);
        static void OnInspector(Registry* registry, const Entity entity);

      public:
        String Name{"New Entity"};
    };

    IG_DECLARE_TYPE_META(NameComponent);
} // namespace ig
