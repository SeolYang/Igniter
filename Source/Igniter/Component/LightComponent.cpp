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
        IG_SET_ON_INSPECTOR_META(LightComponent);
        IG_SET_META_JSON_SERIALIZABLE_COMPONENT(LightComponent);
    }

    template <>
    Json& Serialize<Json, LightComponent>(Json& archive, const LightComponent& light)
    {
        const auto& [Property] = light;
        IG_SERIALIZE_TO_JSON(LightComponent, archive, Property.FalloffRadius);
        IG_SERIALIZE_TO_JSON(LightComponent, archive, Property.Type);
        return archive;
    }

    template <>
    const Json& Deserialize<Json, LightComponent>(const Json& archive, LightComponent& light)
    {
        auto& [Property] = light;
        IG_DESERIALIZE_FROM_JSON_FALLBACK(LightComponent, archive, Property.FalloffRadius, 1.f);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(LightComponent, archive, Property.Type, ELightType::Point);
        return archive;
    }

    template <>
    void OnInspector<LightComponent>(Registry* registry, const Entity entity)
    {
        IG_CHECK(registry != nullptr && entity != NullEntity);
        LightComponent& light = registry->get<LightComponent>(entity);
        ImGui::DragFloat("FalloffRadius", &light.Property.FalloffRadius, 0.1f, 0.01f, FLT_MAX);
        if (ImGuiX::BeginEnumCombo("Type", light.Property.Type))
        {
            ImGuiX::EndEnumCombo();
        }

        ImGui::DragFloat(std::format("Intensity({})", ToLightIntensityUnit(light.Property.Type)).c_str(), &light.Property.Intensity);

        bool bLightColorEdited = ImGuiX::EditColor3("Color", light.Property.Color);
        ImGui::SameLine();
        bLightColorEdited |= ImGuiX::EditVector3("Color", light.Property.Color, 0.001f, "%.03f");
        if (bLightColorEdited)
        {
            light.Property.Color.x = std::clamp<float>(light.Property.Color.x, 0.f, 1.f);
            light.Property.Color.y = std::clamp<float>(light.Property.Color.y, 0.f, 1.f);
            light.Property.Color.z = std::clamp<float>(light.Property.Color.z, 0.f, 1.f);
        }
    }

    IG_DEFINE_COMPONENT_META(LightComponent);
} // namespace ig
