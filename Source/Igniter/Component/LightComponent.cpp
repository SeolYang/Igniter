#include "Igniter/Igniter.h"
#include "Igniter/Core/Json.h"
#include "Igniter/Core/Serialization.h"
#include "Igniter/ImGui/ImGuiExtensions.h"
#include "Igniter/Component/LightComponent.h"

namespace ig
{
    template <>
    void DefineMeta<LightComponent>()
    {
        IG_SET_META_ON_INSPECTOR_FUNC(LightComponent, LightComponent::OnInspector);
        IG_SET_META_JSON_SERIALIZABLE_COMPONENT(LightComponent);
    }

    Json& LightComponent::Serialize(Json& archive) const
    {
        IG_SERIALIZE_TO_JSON(LightComponent, archive, Property.Radius);
        IG_SERIALIZE_TO_JSON(LightComponent, archive, Property.Type);
        return archive;
    }

    const Json& LightComponent::Deserialize(const Json& archive)
    {
        IG_DESERIALIZE_FROM_JSON_FALLBACK(LightComponent, archive, Property.Radius, 1.f);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(LightComponent, archive, Property.Type, ELightType::Point);
        return archive;
    }

    void LightComponent::OnInspector(Registry* registry, const Entity entity)
    {
        IG_CHECK(registry != nullptr && entity != NullEntity);
        LightComponent& light = registry->get<LightComponent>(entity);
        ImGui::DragFloat("Radius", &light.Property.Radius, 0.1f, 0.01f, 100000000.f);
        if (ImGuiX::BeginEnumCombo("Type", light.Property.Type))
        {
            ImGuiX::EndEnumCombo();
        }
    }

    IG_DEFINE_TYPE_META_AS_COMPONENT(LightComponent);
}
