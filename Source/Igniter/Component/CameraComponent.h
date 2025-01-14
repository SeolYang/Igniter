#pragma once
#include "Igniter/Core/Math.h"
#include "Igniter/Core/BoundingVolume.h"
#include "Igniter/Core/Meta.h"

namespace ig
{
    struct TransformComponent;
    struct CameraComponent
    {
      public:
        [[nodiscard]] Matrix CreatePerspective() const
        {
            const float aspectRatio = CameraViewport.AspectRatio();
            IG_CHECK(aspectRatio >= 0.f);
            const float fovRads = Deg2Rad(Fov);
            return DirectX::XMMatrixPerspectiveFovLH(fovRads, aspectRatio, NearZ, FarZ);
        }

        Json& Serialize(Json& archive) const;
        const Json& Deserialize(const Json& archive);
        static void OnInspector(Registry* registry, const Entity entity);

      public:
        Viewport CameraViewport{0.f, 0.f, 1280.f, 720.f};
        float NearZ = 0.1f;
        float FarZ = 300.f;
        /* Degrees Field Of View */
        float Fov = 45.f;

        bool bEnableFrustumCull = true;
        bool bIsMainCamera = false;
        /* #sy_improvements Support Orthographic? */
    };

    IG_DECLARE_TYPE_META(CameraComponent);
} // namespace ig
