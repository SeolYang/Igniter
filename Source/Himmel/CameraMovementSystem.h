#pragma once
#include <Core/InputManager.h>

namespace fe
{
	class World;
	class Timer;
} // namespace fe

class CameraMovementSystem
{
public:
	CameraMovementSystem();

	void Update(fe::World& world);

private:
	const fe::Timer& timer;

	fe::RefHandle<const fe::Action> moveLeftAction;
	fe::RefHandle<const fe::Action> moveRightAction;
	fe::RefHandle<const fe::Action> moveForwardAction;
	fe::RefHandle<const fe::Action> moveBackwardAction;
	fe::RefHandle<const fe::Action> moveUpAction;
	fe::RefHandle<const fe::Action> moveDownAction;

	fe::RefHandle<const fe::Action> sprintAction;
	bool bSprint = false;

	fe::RefHandle<const fe::Axis> turnYawAxis;
	fe::RefHandle<const fe::Axis> turnPitchAxis;

	const float mouseYawSentisitivity = 1.f;
	const float mousePitchSentisitivity = 1.f;

	float yawDegrees = 0.f;
	float pitchDegrees = 0.f;
	bool bTurnDirty = false;

};