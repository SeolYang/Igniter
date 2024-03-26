#include <Frieren.h>
#include <CameraPossessSystem.h>
#include <FpsCameraControllSystem.h>
#include <Core/Window.h>
#include <Core/Log.h>
#include <Core/Engine.h>
#include <ImGui/ImGuiCanvas.h>

namespace fe
{
    IG_DEFINE_LOG_CATEGORY(CameraPossessSystem);

    CameraPossessSystem::CameraPossessSystem(FpsCameraControllSystem& fpsCamControllSystem)
        : window(ig::Igniter::GetWindow()),
          fpsCamControllSystem(fpsCamControllSystem)
    {
        const auto& inputManager = ig::Igniter::GetInputManager();
        togglePossessToCamera = inputManager.QueryAction(ig::String("TogglePossessCamera"));
        IG_CHECK(togglePossessToCamera);

        Configure();
    }

    void CameraPossessSystem::Update()
    {
        if (togglePossessToCamera)
        {
            if (togglePossessToCamera->State == ig::EInputState::Pressed)
            {
                bEnabled = !bEnabled;
                Configure();
            }
        }
    }

    void CameraPossessSystem::Configure()
    {
        auto& imguiCanvas = ig::Igniter::GetImGuiCanvas();
        if (bEnabled)
        {
            IG_LOG(CameraPossessSystem, ig::ELogVerbosity::Debug, "Possess to Camera.");
            fpsCamControllSystem.SetIgnoreInput(false);
            window.SetCursorVisibility(false);
            window.ClipCursor();
            imguiCanvas.SetIgnoreInput(true);
        }
        else
        {
            IG_LOG(CameraPossessSystem, ig::ELogVerbosity::Debug, "Unposess from Camera.");
            fpsCamControllSystem.SetIgnoreInput(true);
            window.SetCursorVisibility(true);
            window.UnclipCursor();
            imguiCanvas.SetIgnoreInput(false);
        }
    }
} // namespace fe