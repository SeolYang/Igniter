#pragma once
#include "Igniter/Core/Math.h"
#include "Igniter/Core/Meta.h"

namespace fe
{
    struct FpsCameraController
    {
    public:
        ig::Json& Serialize(ig::Json& archive) const;
        const ig::Json& Deserialize(const ig::Json& archive);
        static void OnInspector(ig::Registry* registry, const ig::Entity entity);

        float MovementPower = 8.f;
        float MovementPowerAttenuationTime = 0.65f; /* Seconds */
        float MouseYawSentisitivity = 0.03f;        /* Degrees */
        float MousePitchSentisitivity = 0.03f;      /* Degrees */
        float SprintFactor = 8.f;

        ig::Vector3 LatestImpulse{};
        float ElapsedTimeAfterLatestImpulse = 0.f;

        float CurrentYaw = 0.f;   /* Degrees */
        float CurrentPitch = 0.f; /* Degrees */
        constexpr static float MinPitchDegrees = -90.f + 0.01f;
        constexpr static float MaxPitchDegrees = 90.f - 0.01f;
    };

    class FPSCameraControllerUtility
    {
    public:
        static ig::Vector3 CalculateCurrentVelocity(const FpsCameraController& controller)
        {
            return ig::Lerp(controller.LatestImpulse, ig::Vector3::Zero, ig::SmoothStep(0.f, controller.MovementPowerAttenuationTime, controller.ElapsedTimeAfterLatestImpulse));
        }

        static ig::Quaternion CalculateCurrentRotation(const FpsCameraController& controller)
        {
            return ig::Quaternion::CreateFromYawPitchRoll(ig::Deg2Rad(controller.CurrentYaw), ig::Deg2Rad(controller.CurrentPitch), 0.f);
        }
    };

    IG_META_DECLARE(FpsCameraController);
} // namespace fe

namespace ig
{
    template <>
    Json& Serialize(Json& archive, const fe::FpsCameraController& controller);

    template <>
    const Json& Deserialize(const Json& archive, fe::FpsCameraController& controller);

    template <>
    void OnInspector<fe::FpsCameraController>(Registry* registry, Entity entity);
}
