#pragma once
#include <Gameplay/ComponentRegistry.h>
#include <Core/Math.h>

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

    public:
        Viewport CameraViewport{};
        float NearZ = 0.1f;
        float FarZ = 1000.f;
        /* Degrees Field Of View */
        float Fov = 45.f;

        /* #sy_improvement Support Orthographic? */
    };

    struct MainCameraTag
    {
    };

    IG_DECLARE_COMPONENT(CameraComponent);

    IG_DECLARE_COMPONENT(MainCameraTag);
}    // namespace ig
