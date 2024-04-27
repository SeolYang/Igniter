#include <Igniter.h>
#include <Core/Math.h>
#include <Core/Json.h>
#include <ImGui/ImGuiExtensions.h>
#include <Component/TransformComponent.h>

namespace ig
{
    template <>
    void DefineMeta<TransformComponent>()
    {
        IG_SET_META_ON_INSPECTOR_FUNC(TransformComponent, TransformComponent::OnInspector);
        IG_SET_META_SERIALIZE_JSON(TransformComponent);
        IG_SET_META_DESERIALIZE_JSON(TransformComponent);
    }

    Json& TransformComponent::Serialize(Json& archive) const 
    {
        /* Position */
        IG_SERIALIZE_JSON_SIMPLE(TransformComponent, archive, Position.x);
        IG_SERIALIZE_JSON_SIMPLE(TransformComponent, archive, Position.y);
        IG_SERIALIZE_JSON_SIMPLE(TransformComponent, archive, Position.z);

        /* Scale */
        IG_SERIALIZE_JSON_SIMPLE(TransformComponent, archive, Scale.x);
        IG_SERIALIZE_JSON_SIMPLE(TransformComponent, archive, Scale.y);
        IG_SERIALIZE_JSON_SIMPLE(TransformComponent, archive, Scale.z);

        /* Rotation */
        IG_SERIALIZE_JSON_SIMPLE(TransformComponent, archive, Rotation.w);
        IG_SERIALIZE_JSON_SIMPLE(TransformComponent, archive, Rotation.x);
        IG_SERIALIZE_JSON_SIMPLE(TransformComponent, archive, Rotation.y);
        IG_SERIALIZE_JSON_SIMPLE(TransformComponent, archive, Rotation.z);

        return archive;
    }

    const Json& TransformComponent::Deserialize(const Json& archive)
    {
        /* Position */
        IG_DESERIALIZE_JSON_SIMPLE(TransformComponent, archive, Position.x, 0.f);
        IG_DESERIALIZE_JSON_SIMPLE(TransformComponent, archive, Position.y, 0.f);
        IG_DESERIALIZE_JSON_SIMPLE(TransformComponent, archive, Position.z, 0.f);

        /* Scale */
        IG_DESERIALIZE_JSON_SIMPLE(TransformComponent, archive, Scale.x, 1.f);
        IG_DESERIALIZE_JSON_SIMPLE(TransformComponent, archive, Scale.y, 1.f);
        IG_DESERIALIZE_JSON_SIMPLE(TransformComponent, archive, Scale.z, 1.f);

        /* Rotation */
        IG_DESERIALIZE_JSON_SIMPLE(TransformComponent, archive, Rotation.w, 1.f);
        IG_DESERIALIZE_JSON_SIMPLE(TransformComponent, archive, Rotation.x, 0.f);
        IG_DESERIALIZE_JSON_SIMPLE(TransformComponent, archive, Rotation.y, 0.f);
        IG_DESERIALIZE_JSON_SIMPLE(TransformComponent, archive, Rotation.z, 0.f);

        return archive;
    }

    void TransformComponent::OnInspector(Registry* registry, const Entity entity)
    {
        IG_CHECK(registry != nullptr && entity != entt::null);
        TransformComponent& transform = registry->get<TransformComponent>(entity);
        ImGuiX::EditTransform("Transform", transform);
    }

    IG_DEFINE_TYPE_META_AS_COMPONENT(TransformComponent);
}    // namespace ig
