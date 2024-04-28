#pragma once
#include <Input/InputManager.h>

namespace ig
{
    class Timer;
}    // namespace ig

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

        Handle<Action, InputManager> moveLeftActionHandle;
        Handle<Action, InputManager> moveRightActionHandle;
        Handle<Action, InputManager> moveForwardActionHandle;
        Handle<Action, InputManager> moveBackwardActionHandle;
        Handle<Action, InputManager> moveUpActionHandle;
        Handle<Action, InputManager> moveDownActionHandle;

        Handle<Axis, InputManager> turnYawAxisHandle;
        Handle<Axis, InputManager> turnPitchAxisHandle;

        Handle<Action, InputManager> sprintActionHandle;
    };
}    // namespace fe