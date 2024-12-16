#include "Igniter/Igniter.h"
#include "Igniter/Core/Json.h"
#include "Igniter/Core/Serialization.h"
#include "Igniter/Component/NameComponent.h"

namespace ig
{
    template<>
    void DefineMeta<NameComponent>()
    {
        IG_SET_META_ON_INSPECTOR_FUNC(NameComponent, NameComponent::OnInspector);
        IG_SET_META_JSON_SERIALIZABLE_COMPONENT(NameComponent);
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
        std::string nameInput = nameComponent.Name.ToStandard();
        if (ImGui::InputText("Name", &nameInput, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            nameComponent.Name = String{nameInput};
        }
    }

    IG_DEFINE_TYPE_META_AS_COMPONENT(NameComponent);
}    // namespace ig
