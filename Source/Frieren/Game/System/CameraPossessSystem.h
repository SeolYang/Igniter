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
        ig::Window& window;
        ig::RefHandle<const ig::Action> togglePossessToCamera;
        FpsCameraControllSystem& fpsCamControllSystem;

        bool bEnabled = false;
    };
} // namespace fe