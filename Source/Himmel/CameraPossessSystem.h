#pragma once
#include <Core/InputManager.h>

namespace fe
{
    class World;
    class Window;
} // namespace fe

class FpsCameraControllSystem;
class CameraPossessSystem
{
public:
    CameraPossessSystem(FpsCameraControllSystem& fpsCamControllSystem);

    void Update(fe::World& world);

private:
    void Configure();

private:
    fe::Window& window;
    fe::RefHandle<const fe::Action> togglePossessToCamera;
    FpsCameraControllSystem& fpsCamControllSystem;

    bool bEnabled = true;
};