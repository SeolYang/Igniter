#include "Igniter/Igniter.h"
#include "Igniter/Core/Math.h"
#include "Igniter/Core/Json.h"
#include "Igniter/Core/Serialization.h"
#include "Igniter/ImGui/ImGuiExtensions.h"
#include "Igniter/Component/TransformComponent.h"

namespace ig
{
    template <>
    void DefineMeta<TransformComponent>()
    {
        IG_SET_ON_INSPECTOR_META(TransformComponent);
        IG_SET_META_JSON_SERIALIZABLE_COMPONENT(TransformComponent);
    }

    template <>
    Json& Serialize<nlohmann::basic_json<>, TransformComponent>(Json& archive, const TransformComponent& transform)
    {
        const auto& [Position, Scale, Rotation] = transform;
        
        /* Position */
        IG_SERIALIZE_TO_JSON(TransformComponent, archive, Position.x);
        IG_SERIALIZE_TO_JSON(TransformComponent, archive, Position.y);
        IG_SERIALIZE_TO_JSON(TransformComponent, archive, Position.z);

        /* Scale */
        IG_SERIALIZE_TO_JSON(TransformComponent, archive, Scale.x);
        IG_SERIALIZE_TO_JSON(TransformComponent, archive, Scale.y);
        IG_SERIALIZE_TO_JSON(TransformComponent, archive, Scale.z);

        /* Rotation */
        IG_SERIALIZE_TO_JSON(TransformComponent, archive, Rotation.w);
        IG_SERIALIZE_TO_JSON(TransformComponent, archive, Rotation.x);
        IG_SERIALIZE_TO_JSON(TransformComponent, archive, Rotation.y);
        IG_SERIALIZE_TO_JSON(TransformComponent, archive, Rotation.z);

        return archive;
    }

    template <>
    const Json& Deserialize<nlohmann::basic_json<>, TransformComponent>(const Json& archive, TransformComponent& transform)
    {
        auto& [Position, Scale, Rotation] = transform;
        
        /* Position */
        IG_DESERIALIZE_FROM_JSON_FALLBACK(TransformComponent, archive, Position.x, 0.f);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(TransformComponent, archive, Position.y, 0.f);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(TransformComponent, archive, Position.z, 0.f);

        /* Scale */
        IG_DESERIALIZE_FROM_JSON_FALLBACK(TransformComponent, archive, Scale.x, 1.f);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(TransformComponent, archive, Scale.y, 1.f);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(TransformComponent, archive, Scale.z, 1.f);

        /* Rotation */
        IG_DESERIALIZE_FROM_JSON_FALLBACK(TransformComponent, archive, Rotation.w, 1.f);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(TransformComponent, archive, Rotation.x, 0.f);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(TransformComponent, archive, Rotation.y, 0.f);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(TransformComponent, archive, Rotation.z, 0.f);
        
        return archive;
    }

    template <>
    void OnInspector<TransformComponent>(Registry* registry, const Entity entity)
    {
        IG_CHECK(registry != nullptr && entity != entt::null);
        TransformComponent& transform = registry->get<TransformComponent>(entity);
        ImGuiX::EditTransform("Transform", transform);
    }

    IG_DEFINE_COMPONENT_META(TransformComponent);
} // namespace ig
