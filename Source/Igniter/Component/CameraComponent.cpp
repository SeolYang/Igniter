#include "Igniter/Igniter.h"
#include "Igniter/Core/Math.h"
#include "Igniter/Core/Json.h"
#include "Igniter/Component/TransformComponent.h"
#include "Igniter/Component/CameraComponent.h"

namespace ig
{
    template <>
    void DefineMeta<CameraComponent>()
    {
        IG_META_SET_ON_INSPECTOR(CameraComponent);
        IG_META_SET_JSON_SERIALIZABLE_COMPONENT(CameraComponent);
    }

    Matrix CameraUtility::CreatePerspective(const CameraComponent& camera)
    {
        const F32 aspectRatio = camera.CameraViewport.AspectRatio();
        IG_CHECK(aspectRatio >= 0.f);
        const F32 fovRads = Deg2Rad(camera.Fov);
        return DirectX::XMMatrixPerspectiveFovLH(fovRads, aspectRatio, camera.NearZ, camera.FarZ);
    }

    Matrix CameraUtility::CreatePerspectiveForReverseZ(const CameraComponent& camera)
    {
        const F32 aspectRatio = camera.CameraViewport.AspectRatio();
        IG_CHECK(aspectRatio >= 0.f);
        const F32 fovRads = Deg2Rad(camera.Fov);
        return DirectX::XMMatrixPerspectiveFovLH(fovRads, aspectRatio, camera.FarZ, camera.NearZ);
    }

    template <>
    Json& Serialize<nlohmann::basic_json<>, CameraComponent>(Json& archive, const CameraComponent& camera)
    {
        const auto& [CameraViewport, NearZ, FarZ, Fov, bEnableFrustumCull, bIsMainCamera] = camera;
        /* Viewport */
        IG_SERIALIZE_TO_JSON(CameraComponent, archive, CameraViewport.x);
        IG_SERIALIZE_TO_JSON(CameraComponent, archive, CameraViewport.y);
        IG_SERIALIZE_TO_JSON(CameraComponent, archive, CameraViewport.width);
        IG_SERIALIZE_TO_JSON(CameraComponent, archive, CameraViewport.height);
        IG_SERIALIZE_TO_JSON(CameraComponent, archive, CameraViewport.minDepth);
        IG_SERIALIZE_TO_JSON(CameraComponent, archive, CameraViewport.maxDepth);

        /* Perspective */
        IG_SERIALIZE_TO_JSON(CameraComponent, archive, NearZ);
        IG_SERIALIZE_TO_JSON(CameraComponent, archive, FarZ);
        IG_SERIALIZE_TO_JSON(CameraComponent, archive, Fov);

        /* Miscs */
        IG_SERIALIZE_TO_JSON(CameraComponent, archive, bIsMainCamera);

        return archive;
    }

    template <>
    const Json& Deserialize<nlohmann::basic_json<>, CameraComponent>(const Json& archive, CameraComponent& camera)
    {
        auto& [CameraViewport, NearZ, FarZ, Fov, bEnableFrustumCull, bIsMainCamera] = camera;
        /* Viewport */
        IG_DESERIALIZE_FROM_JSON_FALLBACK(CameraComponent, archive, CameraViewport.x, 0.f);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(CameraComponent, archive, CameraViewport.y, 0.f);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(CameraComponent, archive, CameraViewport.width, 0.f);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(CameraComponent, archive, CameraViewport.height, 0.f);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(CameraComponent, archive, CameraViewport.minDepth, 0.f);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(CameraComponent, archive, CameraViewport.maxDepth, 1.f);

        /* Perspective */
        IG_DESERIALIZE_FROM_JSON_FALLBACK(CameraComponent, archive, NearZ, 0.1f);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(CameraComponent, archive, FarZ, 1000.f);
        IG_DESERIALIZE_FROM_JSON_FALLBACK(CameraComponent, archive, Fov, 45.f);

        /* Miscs */
        IG_DESERIALIZE_FROM_JSON_FALLBACK(CameraComponent, archive, bIsMainCamera, false);
        return archive;
    }

    template <>
    void OnInspector<CameraComponent>(Registry* registry, const Entity entity)
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
        ImGui::Checkbox("Enable Frustum Culling", &camera.bEnableFrustumCull);
    }

    IG_META_DEFINE_AS_COMPONENT(CameraComponent);
} // namespace ig
