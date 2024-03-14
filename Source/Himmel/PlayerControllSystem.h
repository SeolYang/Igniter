#pragma once
#include <Core/InputManager.h>

namespace fe
{
	class World;
	class Timer;
} // namespace fe

class PlayerControllSystem
{
public:
	PlayerControllSystem();

	void Update(fe::World& world);

private:
	void HandleMoveAction(fe::World& world);
	void HandleUseHealthRecoveryAction(fe::World& world);
	void HandleDisplayPlayerInfoAction(fe::World& world);

private:
	const fe::Timer& timer;

	fe::RefHandle<const fe::Action> moveLeftAction;
	fe::RefHandle<const fe::Action> moveRightAction;
	fe::RefHandle<const fe::Action> moveForwardAction;
	fe::RefHandle<const fe::Action> moveBackwardAction;
	fe::RefHandle<const fe::Action> moveUpAction;
	fe::RefHandle<const fe::Action> moveDownAction;
	fe::RefHandle<const fe::Action> healthRecoveryAction;
	fe::RefHandle<const fe::Action> displayPlayerInfoAction;
};
