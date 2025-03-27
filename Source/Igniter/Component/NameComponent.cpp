#include "Igniter/Igniter.h"
#include "Igniter/Core/Json.h"
#include "Igniter/Core/Serialization.h"
#include "Igniter/Component/NameComponent.h"

namespace ig
{
    template <>
    void DefineMeta<NameComponent>()
    {
        IG_META_SET_ON_INSPECTOR(NameComponent);
        IG_META_SET_JSON_SERIALIZABLE_COMPONENT(NameComponent);
    }
    IG_META_DEFINE_AS_COMPONENT(NameComponent);

    template <>
    Json& Serialize<nlohmann::basic_json<>, NameComponent>(Json& archive, const NameComponent& name)
    {
        const String& Name = name.Name;
        IG_SERIALIZE_TO_JSON(NameComponent, archive, Name);
        return archive;
    }

    template <>
    const Json& Deserialize<nlohmann::basic_json<>, NameComponent>(const Json& archive, NameComponent& name)
    {
        String& Name = name.Name;
        IG_DESERIALIZE_FROM_JSON(NameComponent, archive, Name);
        return archive;
    }

    template <>
    void OnInspector<NameComponent>(Registry* registry, const Entity entity)
    {
        IG_CHECK(registry != nullptr && entity != entt::null);
        NameComponent& nameComponent = registry->get<NameComponent>(entity);
        std::string nameInput = nameComponent.Name.ToStandard();
        if (ImGui::InputText("Name", &nameInput, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            nameComponent.Name = String{nameInput};
        }
    }
} // namespace ig
