#include <Frieren.h>
#include <FpsCameraController.h>

namespace fe
{
    IG_DEFINE_COMPONENT(FpsCameraController);
}

namespace ig
{
    template <>
    void OnImGui<fe::FpsCameraController>(Registry& registry, const Entity entity)
    {
        fe::FpsCameraController& controller = registry.get<fe::FpsCameraController>(entity);
        ImGui::DragFloat("Movement Power", &controller.MovementPower, 0.01f, 20.f, 90.f);
        ImGui::DragFloat("Movement Power Attenuation Time(secs)", &controller.MovementPowerAttenuationTime, 0.001f);
        ImGui::DragFloat("Mouse Yaw(Y-Axis) Sensitivity", &controller.MouseYawSentisitivity, 0.01f);
        ImGui::DragFloat("Mouse Pitch(X-Axis) Sensitivity", &controller.MousePitchSentisitivity, 0.01f);
        ImGui::DragFloat("Sprint Factor", &controller.SprintFactor, 0.1f);

        ImGui::Text("Read-only");
        ImGui::InputFloat3("Latest Impulse", &controller.LatestImpulse.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat("Elapsed Time After Latest Impulse", &controller.ElapsedTimeAfterLatestImpulse, 0.f, 0.f, "%.3f seconds", ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat("Current Yaw", &controller.CurrentYaw, 0.f, 0.f, "%.3f deg", ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat("Current Pitch", &controller.CurrentPitch, 0.f, 0.f, "%.3f deg", ImGuiInputTextFlags_ReadOnly);
    }
} // namespace ig