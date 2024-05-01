#pragma once
#include "Igniter/Input/InputManager.h"
#include "Igniter/Gameplay/GameSystem.h"

namespace ig
{
    class Timer;
    class World;
}    // namespace ig

namespace fe
{
    class FpsCameraControllSystem : public ig::GameSystem
    {
    public:
        FpsCameraControllSystem();

        void Update(const float deltaTime, ig::World& world) override;
        void SetIgnoreInput(const bool bEnable) { this->bIgnoreInput = bEnable; }

    private:
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