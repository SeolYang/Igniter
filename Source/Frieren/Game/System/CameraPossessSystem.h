#pragma once
#include <Input/InputManager.h>

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
        ig::Window&              window;
        Handle_New<Action>       togglePossessToCameraHandle;
        FpsCameraControllSystem& fpsCamControllSystem;

        bool bEnabled = false;
    };
} // namespace fe
