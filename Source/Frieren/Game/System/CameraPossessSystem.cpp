#include <Frieren.h>
#include <Game/System/CameraPossessSystem.h>
#include <Game/System/FpsCameraControllSystem.h>
#include <Core/Window.h>
#include <Core/Log.h>
#include <Core/Engine.h>
#include <ImGui/ImGuiCanvas.h>

IG_DEFINE_LOG_CATEGORY(CameraPossessSystem);

namespace fe
{
    CameraPossessSystem::CameraPossessSystem(FpsCameraControllSystem& fpsCamControllSystem)
        : window(ig::Igniter::GetWindow()), fpsCamControllSystem(fpsCamControllSystem)
    {
        const auto& inputManager = ig::Igniter::GetInputManager();
        togglePossessToCameraHandle = inputManager.QueryAction(ig::String("TogglePossessCamera"));

        Configure();
    }

    void CameraPossessSystem::Update()
    {
        const Action togglePossessToCamera = Igniter::GetInputManager().GetAction(togglePossessToCameraHandle);
        if (togglePossessToCamera.State == ig::EInputState::Pressed)
        {
            bEnabled = !bEnabled;
            Configure();
        }
    }

    void CameraPossessSystem::Configure()
    {
        ImGuiIO& io = ImGui::GetIO();
        if (bEnabled)
        {
            IG_LOG(CameraPossessSystem, Debug, "Possess to Camera.");
            fpsCamControllSystem.SetIgnoreInput(false);
            window.SetCursorVisibility(false);
            window.ClipCursor();
            io.WantCaptureKeyboard = false;
            io.WantCaptureMouse = false;
        }
        else
        {
            IG_LOG(CameraPossessSystem, Debug, "Unposess from Camera.");
            fpsCamControllSystem.SetIgnoreInput(true);
            window.SetCursorVisibility(true);
            window.UnclipCursor();
            io.WantCaptureKeyboard = true;
            io.WantCaptureMouse = true;
        }
    }
}    // namespace fe
