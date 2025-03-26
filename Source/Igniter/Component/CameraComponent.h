#pragma once
#include "Igniter/Core/Meta.h"
#include "Igniter/Core/Serialization.h"

namespace ig
{
    struct TransformComponent;

    struct CameraComponent
    {
        Viewport CameraViewport{0.f, 0.f, 1280.f, 720.f};
        F32 NearZ = 0.1f;
        F32 FarZ = 1000.f;
        /* Degrees Field Of View */
        F32 Fov = 45.f;

        bool bEnableFrustumCull = true;
        bool bIsMainCamera = false;
    };

    class CameraUtility
    {
    public:
        [[nodiscard]] static Matrix CreatePerspective(const CameraComponent& camera);
        [[nodiscard]] static Matrix CreatePerspectiveForReverseZ(const CameraComponent& camera);
    };

    template <>
    Json& Serialize<Json, CameraComponent>(Json& archive, const CameraComponent& camera);

    template <>
    const Json& Deserialize(const Json& archive, CameraComponent& camera);

    template <>
    void OnInspector<CameraComponent>(Registry* registry, const Entity entity);

    IG_DECLARE_META(CameraComponent);
} // namespace ig
