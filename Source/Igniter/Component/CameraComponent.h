#pragma once
#include <Core/Math.h>
#include <Core/Meta.h>

namespace ig
{
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
        Viewport CameraViewport{};
        float NearZ = 0.1f;
        float FarZ = 1000.f;
        /* Degrees Field Of View */
        float Fov = 45.f;

        bool bIsMainCamera = false;
        /* #sy_improvements Support Orthographic? */
    };

    IG_DECLARE_TYPE_META(CameraComponent);
}    // namespace ig
