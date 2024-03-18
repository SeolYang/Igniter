#include <CameraPossessSystem.h>
#include <FpsCameraControllSystem.h>
#include <Core/Window.h>
#include <Core/Log.h>
#include <Core/Igniter.h>

namespace fe
{
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
        if (bEnabled)
        {
            IG_LOG(ig::LogDebug, "Possess to Camera.");
            fpsCamControllSystem.SetIgnoreInput(false);
            window.SetCursorVisibility(false);
            window.ClipCursor();
        }
        else
        {
            IG_LOG(ig::LogDebug, "Unposess from Camera.");
            fpsCamControllSystem.SetIgnoreInput(true);
            window.SetCursorVisibility(true);
            window.UnclipCursor();
        }
    }
} // namespace fe