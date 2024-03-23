#include <PCH.h>
#include <Component/TransformComponent.h>
#include <Math/Utils.h>

namespace ig
{
    IG_DEFINE_COMPONENT(TransformComponent);

    template <>
    void OnImGui<TransformComponent>(Registry& registry, const Entity entity)
    {
        TransformComponent& transformComponent = registry.get<TransformComponent>(entity);
        ImGui::DragFloat3("Position", &transformComponent.Position.x);
        ImGui::DragFloat3("Scale", &transformComponent.Scale.x, 0.001f);

        Vector3 eulerAngles = transformComponent.Rotation.ToEuler();
        eulerAngles.x = Rad2Deg(eulerAngles.x);
        eulerAngles.y = Rad2Deg(eulerAngles.y); 
        eulerAngles.z = Rad2Deg(eulerAngles.z); 
        if (ImGui::DragFloat3("Rotation", &eulerAngles.x, 0.5f))
        {
            transformComponent.Rotation = Quaternion::CreateFromYawPitchRoll(Vector3{Deg2Rad(eulerAngles.x), Deg2Rad(eulerAngles.y), Deg2Rad(eulerAngles.z)});
        }
    }
} // namespace ig