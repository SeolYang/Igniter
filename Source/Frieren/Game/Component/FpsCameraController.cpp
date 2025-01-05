#include "Frieren/Frieren.h"
#include "Igniter/Core/Json.h"
#include "Igniter/Core/Serialization.h"
#include "Frieren/Game/Component/FpsCameraController.h"

namespace fe
{
    template <>
    void DefineMeta<FpsCameraController>()
    {
        IG_SET_META_ON_INSPECTOR_FUNC(FpsCameraController, FpsCameraController::OnInspector);
        IG_SET_META_JSON_SERIALIZABLE_COMPONENT(FpsCameraController);
    }

    ig::Json& FpsCameraController::Serialize(ig::Json& archive) const
    {
        IG_SERIALIZE_JSON_SIMPLE(FpsCameraController, archive, MovementPower);
        IG_SERIALIZE_JSON_SIMPLE(FpsCameraController, archive, MovementPowerAttenuationTime);
        IG_SERIALIZE_JSON_SIMPLE(FpsCameraController, archive, MouseYawSentisitivity);
        IG_SERIALIZE_JSON_SIMPLE(FpsCameraController, archive, MousePitchSentisitivity);
        IG_SERIALIZE_JSON_SIMPLE(FpsCameraController, archive, SprintFactor);
        return archive;
    }

    const ig::Json& FpsCameraController::Deserialize(const ig::Json& archive)
    {
        IG_DESERIALIZE_JSON_SIMPLE_FALLBACK(FpsCameraController, archive, MovementPower, 25.f);
        IG_DESERIALIZE_JSON_SIMPLE_FALLBACK(FpsCameraController, archive, MovementPowerAttenuationTime, 0.65f);
        IG_DESERIALIZE_JSON_SIMPLE_FALLBACK(FpsCameraController, archive, MouseYawSentisitivity, 0.03f);
        IG_DESERIALIZE_JSON_SIMPLE_FALLBACK(FpsCameraController, archive, MousePitchSentisitivity, 0.03f);
        IG_DESERIALIZE_JSON_SIMPLE_FALLBACK(FpsCameraController, archive, SprintFactor, 4.f);
        return archive;
    }

    void FpsCameraController::OnInspector(ig::Registry* registry, const ig::Entity entity)
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

    IG_DEFINE_TYPE_META_AS_COMPONENT(FpsCameraController);
} // namespace fe
