#pragma once
#include <Core/InputManager.h>
#include <Gameplay/Common.h>

namespace fe
{
    class Timer;
} // namespace fe

class FpsCameraControllSystem
{
public:
    FpsCameraControllSystem();

    void Update(fe::Registry& registry);

    void SetIgnoreInput(const bool bEnable) { this->bIgnoreInput = bEnable; }

private:
    const fe::Timer& timer;

    bool bIgnoreInput = false;

    fe::RefHandle<const fe::Action> moveLeftAction;
    fe::RefHandle<const fe::Action> moveRightAction;
    fe::RefHandle<const fe::Action> moveForwardAction;
    fe::RefHandle<const fe::Action> moveBackwardAction;
    fe::RefHandle<const fe::Action> moveUpAction;
    fe::RefHandle<const fe::Action> moveDownAction;

    fe::RefHandle<const fe::Axis> turnYawAxis;
    fe::RefHandle<const fe::Axis> turnPitchAxis;

    fe::RefHandle<const fe::Action> sprintAction;
};