#include <Igniter.h>
#include <Component/CameraComponent.h>

namespace ig
{
    IG_DEFINE_COMPONENT(CameraComponent);
    IG_DEFINE_COMPONENT(MainCameraTag);

    template <>
    void OnImGui<CameraComponent>(Registry& registry, const Entity entity)
    {
        CameraComponent& camera = registry.get<CameraComponent>(entity);
        ImGui::DragFloat("Fov", &camera.Fov, 0.01f, 20.f, 90.f);
        ImGui::DragFloat("Near", &camera.NearZ, 0.001f);
        ImGui::DragFloat("Far", &camera.FarZ, 0.001f);

        ImGui::Text("Viewport");
        ImGui::InputFloat("X", &camera.CameraViewport.x);
        ImGui::InputFloat("Y", &camera.CameraViewport.y);
        ImGui::InputFloat("Width", &camera.CameraViewport.width);
        ImGui::InputFloat("Height", &camera.CameraViewport.height);
    }
}    // namespace ig
