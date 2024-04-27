#include <Igniter.h>
#include <Core/Json.h>
#include <Component/NameComponent.h>

namespace ig
{
    template<>
    void DefineMeta<NameComponent>()
    {
        IG_SET_META_ON_INSPECTOR_FUNC(NameComponent, NameComponent::OnInspector);
        IG_SET_META_SERIALIZE_JSON(NameComponent);
        IG_SET_META_DESERIALIZE_JSON(NameComponent);
    }

    Json& NameComponent::Serialize(Json& archive) const
    {
        IG_SERIALIZE_JSON_SIMPLE(NameComponent, archive, Name);
        return archive;
    }

    const ig::Json& NameComponent::Deserialize(const Json& archive)
    {
        IG_DESERIALIZE_JSON_SIMPLE(NameComponent, archive, Name, String{});
        return archive;
    }

    void NameComponent::OnInspector(Registry* registry, const Entity entity)
    {
        IG_CHECK(registry != nullptr && entity != entt::null);
        NameComponent& nameComponent = registry->get<NameComponent>(entity);
        ImGui::Text(nameComponent.Name.ToCString());
    }

    IG_DEFINE_TYPE_META_AS_COMPONENT(NameComponent);
}    // namespace ig
