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

        Handle<Action> moveLeftActionHandle;
        Handle<Action> moveRightActionHandle;
        Handle<Action> moveForwardActionHandle;
        Handle<Action> moveBackwardActionHandle;
        Handle<Action> moveUpActionHandle;
        Handle<Action> moveDownActionHandle;

        Handle<Axis> turnYawAxisHandle;
        Handle<Axis> turnPitchAxisHandle;

        Handle<Action> sprintActionHandle;
    };
}    // namespace fe