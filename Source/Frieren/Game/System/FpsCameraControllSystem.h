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

        ig::Handle<ig::Action> moveLeftActionHandle;
        ig::Handle<ig::Action> moveRightActionHandle;
        ig::Handle<ig::Action> moveForwardActionHandle;
        ig::Handle<ig::Action> moveBackwardActionHandle;
        ig::Handle<ig::Action> moveUpActionHandle;
        ig::Handle<ig::Action> moveDownActionHandle;

        ig::Handle<ig::Axis> turnYawAxisHandle;
        ig::Handle<ig::Axis> turnPitchAxisHandle;

        ig::Handle<ig::Action> sprintActionHandle;
    };
} // namespace fe
