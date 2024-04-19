#pragma once
#include <Input/InputManager.h>

namespace ig
{
    class Timer;
} // namespace ig

namespace fe
{
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
} // namespace fe