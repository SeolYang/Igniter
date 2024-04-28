#include <Igniter.h>
#include <Core/Json.h>
#include <Core/Serialization.h>
#include <Component/CameraComponent.h>

namespace ig
{
    template <>
    void DefineMeta<CameraComponent>()
    {
        IG_SET_META_ON_INSPECTOR_FUNC(CameraComponent, CameraComponent::OnInspector);
        IG_SET_META_JSON_SERIALIZABLE_COMPONENT(CameraComponent);
    }

    Json& CameraComponent::Serialize(Json& archive) const
    {
        /* Viewport */
        IG_SERIALIZE_JSON_SIMPLE(CameraComponent, archive, CameraViewport.x);
        IG_SERIALIZE_JSON_SIMPLE(CameraComponent, archive, CameraViewport.y);
        IG_SERIALIZE_JSON_SIMPLE(CameraComponent, archive, CameraViewport.width);
        IG_SERIALIZE_JSON_SIMPLE(CameraComponent, archive, CameraViewport.height);
        IG_SERIALIZE_JSON_SIMPLE(CameraComponent, archive, CameraViewport.minDepth);
        IG_SERIALIZE_JSON_SIMPLE(CameraComponent, archive, CameraViewport.maxDepth);

        /* Perspective */
        IG_SERIALIZE_JSON_SIMPLE(CameraComponent, archive, NearZ);
        IG_SERIALIZE_JSON_SIMPLE(CameraComponent, archive, FarZ);
        IG_SERIALIZE_JSON_SIMPLE(CameraComponent, archive, Fov);

        /* Miscs */
        IG_SERIALIZE_JSON_SIMPLE(CameraComponent, archive, bIsMainCamera);

        return archive;
    }

    const Json& CameraComponent::Deserialize(const Json& archive)
    {
        /* Viewport */
        IG_DESERIALIZE_JSON_SIMPLE(CameraComponent, archive, CameraViewport.x, 0.f);
        IG_DESERIALIZE_JSON_SIMPLE(CameraComponent, archive, CameraViewport.y, 0.f);
        IG_DESERIALIZE_JSON_SIMPLE(CameraComponent, archive, CameraViewport.width, 0.f);
        IG_DESERIALIZE_JSON_SIMPLE(CameraComponent, archive, CameraViewport.height, 0.f);
        IG_DESERIALIZE_JSON_SIMPLE(CameraComponent, archive, CameraViewport.minDepth, 0.f);
        IG_DESERIALIZE_JSON_SIMPLE(CameraComponent, archive, CameraViewport.maxDepth, 1.f);

        /* Perspective */
        IG_DESERIALIZE_JSON_SIMPLE(CameraComponent, archive, NearZ, 0.1f);
        IG_DESERIALIZE_JSON_SIMPLE(CameraComponent, archive, FarZ, 1000.f);
        IG_DESERIALIZE_JSON_SIMPLE(CameraComponent, archive, Fov, 45.f);

        /* Miscs */
        IG_DESERIALIZE_JSON_SIMPLE(CameraComponent, archive, bIsMainCamera, false);

        return archive;
    }

    void CameraComponent::OnInspector(Registry* registry, const Entity entity)
    {
        IG_CHECK(registry != nullptr && entity != entt::null);
        CameraComponent& camera = registry->get<CameraComponent>(entity);
        ImGui::DragFloat("Fov", &camera.Fov, 0.01f, 20.f, 90.f);
        ImGui::DragFloat("Near", &camera.NearZ, 0.001f);
        ImGui::DragFloat("Far", &camera.FarZ, 0.001f);

        ImGui::Text("Viewport");
        ImGui::InputFloat("X", &camera.CameraViewport.x);
        ImGui::InputFloat("Y", &camera.CameraViewport.y);
        ImGui::InputFloat("Width", &camera.CameraViewport.width);
        ImGui::InputFloat("Height", &camera.CameraViewport.height);

        ImGui::Checkbox("Is Main Camera", &camera.bIsMainCamera);
    }
    IG_DEFINE_TYPE_META_AS_COMPONENT(CameraComponent);
}    // namespace ig
