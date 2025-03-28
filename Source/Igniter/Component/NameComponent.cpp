#include "Igniter/Igniter.h"
#include "Igniter/Core/Json.h"
#include "Igniter/Core/Serialization.h"
#include "Igniter/Component/NameComponent.h"

namespace ig
{
    template <>
    void DefineMeta<NameComponent>()
    {
        IG_SET_ON_INSPECTOR_META(NameComponent);
        IG_SET_META_JSON_SERIALIZABLE_COMPONENT(NameComponent);
    }
    IG_DEFINE_COMPONENT_META(NameComponent);

    template <>
    Json& Serialize<nlohmann::basic_json<>, NameComponent>(Json& archive, const NameComponent& name)
    {
        IG_SERIALIZE_TO_JSON_EXPR(NameComponent, archive, Name, name.Name);
        return archive;
    }

    template <>
    const Json& Deserialize<nlohmann::basic_json<>, NameComponent>(const Json& archive, NameComponent& name)
    {
        IG_DESERIALIZE_FROM_JSON_TEMP(NameComponent, archive, Name, name.Name);
        return archive;
    }

    template <>
    void OnInspector<NameComponent>(Registry* registry, const Entity entity)
    {
        IG_CHECK(registry != nullptr && entity != entt::null);
        NameComponent& nameComponent = registry->get<NameComponent>(entity);
        ImGui::InputText("Name", &nameComponent.Name, ImGuiInputTextFlags_EnterReturnsTrue);
    }
} // namespace ig
