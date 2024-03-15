#include <CameraPossessSystem.h>
#include <FpsCameraControllSystem.h>
#include <Core/Window.h>
#include <Core/Log.h>
#include <Engine.h>

CameraPossessSystem::CameraPossessSystem(FpsCameraControllSystem& fpsCamControllSystem)
	: window(fe::Engine::GetWindow()),
	  fpsCamControllSystem(fpsCamControllSystem)
{
	const auto& inputManager = fe::Engine::GetInputManager();
	togglePossessToCamera = inputManager.QueryAction(fe::String("TogglePossessCamera"));
	check(togglePossessToCamera);

	Configure();
}

void CameraPossessSystem::Update(fe::World&)
{
	if (togglePossessToCamera)
	{
		if (togglePossessToCamera->State == fe::EInputState::Pressed)
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
		FE_LOG(fe::LogDebug, "Possess to Camera.");
		fpsCamControllSystem.SetIgnoreInput(false);
		window.SetCursorVisibility(false);
		window.ClipCursor();
	}
	else
	{
		FE_LOG(fe::LogDebug, "Unposess from Camera.");
		fpsCamControllSystem.SetIgnoreInput(true);
		window.SetCursorVisibility(true);
		window.UnclipCursor();
	}
}
