#pragma once
#include "Igniter/Input/InputManager.h"
#include "Igniter/Gameplay/GameSystem.h"

namespace ig
{
    class Timer;
    class World;
} // namespace ig

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

        ig::Handle<ig::Action, ig::InputManager> moveLeftActionHandle;
        ig::Handle<ig::Action, ig::InputManager> moveRightActionHandle;
        ig::Handle<ig::Action, ig::InputManager> moveForwardActionHandle;
        ig::Handle<ig::Action, ig::InputManager> moveBackwardActionHandle;
        ig::Handle<ig::Action, ig::InputManager> moveUpActionHandle;
        ig::Handle<ig::Action, ig::InputManager> moveDownActionHandle;

        ig::Handle<ig::Axis, ig::InputManager> turnYawAxisHandle;
        ig::Handle<ig::Axis, ig::InputManager> turnPitchAxisHandle;

        ig::Handle<ig::Action, ig::InputManager> sprintActionHandle;
    };
} // namespace fe
