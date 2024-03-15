#pragma once
#include <Core/InputManager.h>

namespace fe
{
	class World;
	class Window;
} // namespace fe

class CameraMovementSystem;
class CameraPossessSystem
{
public:
	CameraPossessSystem(CameraMovementSystem& camMovementSystem);

	void Update(fe::World& world);

private:
	void Configure();

private:
	fe::Window& window;
	fe::RefHandle<const fe::Action> togglePossessToCamera;
	CameraMovementSystem& cameraMovementSystem;

	bool bEnabled = true;
};