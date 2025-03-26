#include "Frieren/Frieren.h"
#include "Igniter/Core/Json.h"
#include "Igniter/Core/Serialization.h"
#include "Frieren/Game/Component/FpsCameraController.h"

namespace fe
{
    template <>
    void DefineMeta<FpsCameraController>()
    {
        IG_SET_ON_INSPECTOR_META(FpsCameraController);
        IG_SET_META_JSON_SERIALIZABLE_COMPONENT(FpsCameraController);
    }
    IG_DEFINE_COMPONENT_META(FpsCameraController);
} // namespace fe

namespace ig
{
    template <>
    Json& ig::Serialize<Json, fe::FpsCameraController>(Json& archive, const fe::FpsCameraController& controller)
    {
        IG_SERIALIZE_TO_JSON(FpsCameraController, archive, controller.MovementPower);
        IG_SERIALIZE_TO_JSON(FpsCameraController, archive, controller.MovementPowerAttenuationTime);
        IG_SERIALIZE_TO_JSON(FpsCameraController, archive, controller.MouseYawSentisitivity);
        IG_SERIALIZE_TO_JSON(FpsCameraController, archive, controller.MousePitchSentisitivity);
        IG_SERIALIZE_TO_JSON(FpsCameraController, archive, controller.SprintFactor);
        return archive;
    }

    template <>
    const Json& ig::Deserialize<Json, fe::FpsCameraController>(const Json& archive, fe::FpsCameraController& controller)
    {
        IG_DESERIALIZE_FROM_JSON_FALLBACK(FpsCameraController, archive, controller.MovementPower, 25.f);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(FpsCameraController, archive, controller.MovementPowerAttenuationTime, 0.65f);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(FpsCameraController, archive, controller.MouseYawSentisitivity, 0.03f);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(FpsCameraController, archive, controller.MousePitchSentisitivity, 0.03f);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(FpsCameraController, archive, controller.SprintFactor, 4.f);
        return archive;
    }

    template <>
    void OnInspector<fe::FpsCameraController>(Registry* registry, Entity entity)
    {
        IG_CHECK(registry != nullptr && entity != entt::null);
        fe::FpsCameraController& controller = registry->get<fe::FpsCameraController>(entity);
        ImGui::DragFloat("Movement Power", &controller.MovementPower, 0.01f, 20.f, 90.f);
        ImGui::DragFloat("Movement Power Attenuation Time(secs)", &controller.MovementPowerAttenuationTime, 0.001f);
        ImGui::DragFloat("Mouse Yaw(Y-Axis) Sensitivity", &controller.MouseYawSentisitivity, 0.01f);
        ImGui::DragFloat("Mouse Pitch(X-Axis) Sensitivity", &controller.MousePitchSentisitivity, 0.01f);
        ImGui::DragFloat("Sprint Factor", &controller.SprintFactor, 0.1f);

        ImGui::Text("Read-only");
        ImGui::InputFloat3("Latest Impulse", &controller.LatestImpulse.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat(
            "Elapsed Time After Latest Impulse", &controller.ElapsedTimeAfterLatestImpulse, 0.f, 0.f, "%.3f seconds", ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat("Current Yaw", &controller.CurrentYaw, 0.f, 0.f, "%.3f deg", ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat("Current Pitch", &controller.CurrentPitch, 0.f, 0.f, "%.3f deg", ImGuiInputTextFlags_ReadOnly);
    }
}
