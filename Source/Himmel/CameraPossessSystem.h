#pragma once
#include <Core/InputManager.h>
#include <Gameplay/Common.h>

namespace ig
{
    class Window;
} // namespace ig

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

    bool bEnabled = true;
};