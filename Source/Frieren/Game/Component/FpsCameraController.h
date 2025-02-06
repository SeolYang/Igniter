#pragma once
#include "Igniter/Core/Math.h"
#include "Igniter/Core/Meta.h"

namespace fe
{
    struct FpsCameraController
    {
      public:
        ~FpsCameraController() = default;

        ig::Vector3 GetCurrentVelocity() const
        {
            return ig::Lerp(LatestImpulse, ig::Vector3::Zero, ig::SmoothStep(0.f, MovementPowerAttenuationTime, ElapsedTimeAfterLatestImpulse));
        }

        ig::Quaternion GetCurrentRotation() const
        {
            return ig::Quaternion::CreateFromYawPitchRoll(ig::Deg2Rad(CurrentYaw), ig::Deg2Rad(CurrentPitch), 0.f);
        }

        ig::Json& Serialize(ig::Json& archive) const;
        const ig::Json& Deserialize(const ig::Json& archive);
        static void OnInspector(ig::Registry* registry, const ig::Entity entity);

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
        constexpr static float MinPitchDegrees = -90.f + 0.01f;
        constexpr static float MaxPitchDegrees = 90.f - 0.01f;
    };

    IG_DECLARE_META(FpsCameraController);
} // namespace fe
