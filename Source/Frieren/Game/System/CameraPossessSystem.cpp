#include "Frieren/Frieren.h"
#include "Igniter/Core/Window.h"
#include "Igniter/Core/Log.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/ImGui/ImGuiCanvas.h"
#include "Igniter/ImGui/ImGuiContext.h"
#include "Frieren/Game/System/CameraPossessSystem.h"
#include "Frieren/Game/System/FpsCameraControllSystem.h"

IG_DEFINE_LOG_CATEGORY(CameraPossessSystem);

namespace fe
{
    CameraPossessSystem::CameraPossessSystem(FpsCameraControllSystem& fpsCamControllSystem)
        : window(ig::Engine::GetWindow()), fpsCamControllSystem(fpsCamControllSystem)
    {
        const auto& inputManager = ig::Engine::GetInputManager();
        togglePossessToCameraHandle = inputManager.QueryAction(ig::String("TogglePossessCamera"));

        Configure();
    }

    void CameraPossessSystem::Update()
    {
        const ig::Action togglePossessToCamera = ig::Engine::GetInputManager().GetAction(togglePossessToCameraHandle);
        if (togglePossessToCamera.State == ig::EInputState::Pressed)
        {
            bEnabled = !bEnabled;
            Configure();
        }
    }

    void CameraPossessSystem::Configure()
    {
        ig::ImGuiContext& imguiContext = ig::Engine::GetImGuiContext();
        if (bEnabled)
        {
            IG_LOG(CameraPossessSystem, Debug, "Possess to Camera.");
            fpsCamControllSystem.SetIgnoreInput(false);
            window.SetCursorVisibility(false);
            window.ClipCursor();
            imguiContext.SetInputEnabled(false);
        }
        else
        {
            IG_LOG(CameraPossessSystem, Debug, "Unposess from Camera.");
            fpsCamControllSystem.SetIgnoreInput(true);
            window.SetCursorVisibility(true);
            window.UnclipCursor();
            imguiContext.SetInputEnabled(true);
        }
    }
}    // namespace fe
