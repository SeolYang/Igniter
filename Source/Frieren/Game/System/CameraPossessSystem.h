#pragma once
#include "Igniter/Input/InputManager.h"

namespace ig
{
    class Window;
} // namespace ig

namespace fe
{
    class FpsCameraControllSystem;

    class CameraPossessSystem
    {
      public:
        CameraPossessSystem(FpsCameraControllSystem& fpsCamControllSystem);

        void Update();

      private:
        void Configure();

      private:
        ig::Window& window;
        ig::Handle<ig::Action> togglePossessToCameraHandle;
        FpsCameraControllSystem& fpsCamControllSystem;

        bool bEnabled = false;
    };
} // namespace fe
