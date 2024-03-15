#pragma once
#include <Math/Common.h>

struct FpsCameraController
{
public:
	fe::Vector3 GetCurrentVelocity() const
	{
		return fe::Lerp(LatestImpulse, fe::Vector3::Zero,
						fe::SmoothStep(0.f, MovementPowerAttenuationTime, ElapsedTimeAfterLatestImpulse));
	}

	fe::Quaternion GetCurrentRotation() const
	{
		return fe::Quaternion::CreateFromYawPitchRoll(
			fe::Deg2Rad(CurrentYaw),
			fe::Deg2Rad(CurrentPitch), 0.f);
	}

public:
	float MovementPower = 25.f;
	float MovementPowerAttenuationTime = 1.f; /* Seconds */
	float MouseYawSentisitivity = 0.03f;		  /* Degrees */
	float MousePitchSentisitivity = 0.03f;	  /* Degrees */
	float SprintFactor = 4.f;

	fe::Vector3 LatestImpulse{};
	float ElapsedTimeAfterLatestImpulse = 0.f;

	float CurrentYaw = 0.f;
	float CurrentPitch = 0.f;
};