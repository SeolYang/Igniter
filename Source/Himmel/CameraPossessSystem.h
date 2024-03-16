#pragma once
#include <Core/InputManager.h>
#include <Gameplay/Common.h>

namespace fe
{
    class Window;
} // namespace fe

class FpsCameraControllSystem;
class CameraPossessSystem
{
public:
    CameraPossessSystem(FpsCameraControllSystem& fpsCamControllSystem);

    void Update();

private:
    void Configure();

private:
    fe::Window& window;
    fe::RefHandle<const fe::Action> togglePossessToCamera;
    FpsCameraControllSystem& fpsCamControllSystem;

    bool bEnabled = true;
};