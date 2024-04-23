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

        Handle_New<Action> moveLeftActionHandle;
        Handle_New<Action> moveRightActionHandle;
        Handle_New<Action> moveForwardActionHandle;
        Handle_New<Action> moveBackwardActionHandle;
        Handle_New<Action> moveUpActionHandle;
        Handle_New<Action> moveDownActionHandle;

        Handle_New<Axis> turnYawAxisHandle;
        Handle_New<Axis> turnPitchAxisHandle;

        Handle_New<Action> sprintActionHandle;
    };
}    // namespace fe