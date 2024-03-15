#include <CameraPossessSystem.h>
#include <CameraMovementSystem.h>
#include <Core/Window.h>
#include <Core/Log.h>
#include <Engine.h>

CameraPossessSystem::CameraPossessSystem(CameraMovementSystem& camMovementSystem)
	: window(fe::Engine::GetWindow()),
	  cameraMovementSystem(camMovementSystem)
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
		cameraMovementSystem.SetIgnoreInput(false);
		window.SetCursorVisibility(false);
		window.ClipCursor();
	}
	else
	{
		FE_LOG(fe::LogDebug, "Unposess from Camera.");
		cameraMovementSystem.SetIgnoreInput(true);
		window.SetCursorVisibility(true);
		window.UnclipCursor();
	}
}
