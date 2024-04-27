#pragma once
#include <Core/Math.h>
#include <Core/Meta.h>

namespace fe
{
    struct FpsCameraController
    {
    public:
        ig::Vector3 GetCurrentVelocity() const
        {
            return ig::Lerp(LatestImpulse, ig::Vector3::Zero, ig::SmoothStep(0.f, MovementPowerAttenuationTime, ElapsedTimeAfterLatestImpulse));
        }

        ig::Quaternion GetCurrentRotation() const
        {
            return ig::Quaternion::CreateFromYawPitchRoll(ig::Deg2Rad(CurrentYaw), ig::Deg2Rad(CurrentPitch), 0.f);
        }

        Json& Serialize(Json& archive) const;
        const Json& Deserialize(const Json& archive);
        static void OnInspector(Registry* registry, const Entity entity);

    public:
        float MovementPower = 25.f;
        float MovementPowerAttenuationTime = 0.65f; /* Seconds */
        float MouseYawSentisitivity = 0.03f;        /* Degrees */
        float MousePitchSentisitivity = 0.03f;      /* Degrees */
        float SprintFactor = 4.f;

        ig::Vector3 LatestImpulse{};
        float ElapsedTimeAfterLatestImpulse = 0.f;

        float CurrentYaw = 0.f;   /* Degrees */
        float CurrentPitch = 0.f; /* Degrees */
        constexpr static float MinPitchDegrees = -45.f;
        constexpr static float MaxPitchDegrees = 45.f;
    };

    IG_DECLARE_TYPE_META(FpsCameraController);
}    // namespace fe