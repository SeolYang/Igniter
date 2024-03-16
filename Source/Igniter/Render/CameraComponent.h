#pragma once
#include <Math/Common.h>
#include <Math/Utils.h>
#include <Core/Assert.h>

namespace ig
{
    /* #sy_wip_todo Camera->CameraComponent */
    struct CameraComponent
    {
    public:
        [[nodiscard]] Matrix CreatePerspective() const
        {
            const float aspectRatio = CameraViewport.AspectRatio();
            check(aspectRatio >= 0.f);
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
} // namespace ig