#pragma once

struct CameraController
{
public:
	float MovementPower = 10.f;
	float MovementPowerAttenuationTime = 0.5f;
	float MouseYawSentisitivity = 5.f; /* Degrees */
	float MousePitchSentisitivity = 5.f; /* Degrees */
	float RotatePowerAttenuationTime = 0.3f;
	float SprintFactor = 4.f;
};