#pragma once
#include <Core/InputManager.h>
#include <Gameplay/Common.h>

namespace ig
{
    class Timer;
} // namespace ig

class FpsCameraControllSystem
{
public:
    FpsCameraControllSystem();

    void Update(ig::Registry& registry);

    void SetIgnoreInput(const bool bEnable) { this->bIgnoreInput = bEnable; }

private:
    const ig::Timer& timer;

    bool bIgnoreInput = false;

    ig::RefHandle<const ig::Action> moveLeftAction;
    ig::RefHandle<const ig::Action> moveRightAction;
    ig::RefHandle<const ig::Action> moveForwardAction;
    ig::RefHandle<const ig::Action> moveBackwardAction;
    ig::RefHandle<const ig::Action> moveUpAction;
    ig::RefHandle<const ig::Action> moveDownAction;

    ig::RefHandle<const ig::Axis> turnYawAxis;
    ig::RefHandle<const ig::Axis> turnPitchAxis;

    ig::RefHandle<const ig::Action> sprintAction;
};